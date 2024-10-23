/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * Second generation work queue implementation
 */
#define FUSE_USE_VERSION 31
 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fuse.h>

#include "glib.h"

#define DEBUG  1
#include "fs_super.h"


static GHashTable *fs_super_htable = NULL;

static void do_fs_initcall(void)
{
	int len, ret = 0;
	char path[128] = {0};
	struct fs_info *info;
	struct fs_initcall *init;

	for (init = ll_entry_start(fs_initcall); 
		 init < ll_entry_end(fs_initcall); init++) {
		info = (struct fs_info *)init->super;
		
		len = 0;
		memset(path, 0, sizeof(path));
		
		if (info->parent != NULL) {
			len = strlen(info->parent);
			
			if (len > 0) {
				strcpy(path, info->parent);

				if (path[len - 1] != '/') {
					path[len] = '/';
					len++;
				}
			}
		}
		
		strcpy(path + len, info->name);
		ret = g_hash_table_insert(fs_super_htable, g_strdup(path), info);

		if (ret == 0) {
			pr_err("init->name = %s, info->name = %s\n", init->name, info->name);
		}
	}
}

static void *fs_super_init(struct fuse_conn_info *conn, struct fuse_config *cfg)
{
	(void) conn;
	pr_debug("%s: path = %s\n", __func__, "");
	
	// fs_super_htable = g_hash_table_new(g_str_hash, g_str_equal);
	fs_super_htable = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

	if (fs_super_htable != NULL) {
		do_fs_initcall();
	}
	
	cfg->kernel_cache = 1;

	return NULL;
}

static void fs_super_destroy(void *userdata)
{
	pr_debug("%s: path = %s\n", __func__, "");

	g_hash_table_destroy(fs_super_htable);
}

static int fs_super_getattr(const char *path, struct stat *st,
			 struct fuse_file_info *fi)
{
	(void) fi;
	int ret = 0;
	mode_t mode = 0;
	struct fs_info *info;
	struct fs_super *fs_super;
	
	info = g_hash_table_lookup(fs_super_htable, path);
	
	pr_debug("%s: path = %s, info = %p\n", __func__, path, info);
	if (info == NULL) {
		return -ENOENT;
	}
	
	if (info->mode == S_IFDIR) {
		st->st_mode = S_IFDIR | 0755;
		st->st_nlink = 2;
	} else {	
		// fs_super->info.mode = S_IFREG
		fs_super = (struct fs_super *)info;
	
		if (fs_super->fop.read) {
			mode |= 0444;
		}
		
		if (fs_super->fop.write) {
			mode |= 0222;
		}
		
		st->st_mode = mode | fs_super->info.mode;
		st->st_nlink = 1;
		st->st_size = 1024;
	}
	
	st->st_atime = time(NULL);
	st->st_mtime = time(NULL);
	st->st_ctime = time(NULL);
	
	return ret;
}

static int fs_super_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags)
{
	(void) offset;
	(void) fi;
	(void) flags;
	GHashTableIter iter;
	gpointer key, value;
	struct fs_info *info;
	
	info = g_hash_table_lookup(fs_super_htable, path);
	
	pr_debug("%s: path = %s, info = %p\n", __func__, path, info);
	if (info == NULL) {
		return 0;
	}

	filler(buf, ".", NULL, 0, 0);
	filler(buf, "..", NULL, 0, 0);

	g_hash_table_iter_init(&iter, fs_super_htable);
	
	while (g_hash_table_iter_next(&iter, &key, &value)) {
		info = (struct fs_info *)value;
		
		if (info->parent == NULL) {
			continue;
		}
	
		if (strcmp(path, info->parent) == 0) {
			filler(buf, info->name, NULL, 0, 0);
		}	
	}

	return 0;
}


static int fs_super_open(const char *path, struct fuse_file_info *fi)
{
	int ret = 0;
	struct fs_super *fs_super;

	pr_debug("%s: path = %s\n", __func__, path);

	fs_super = g_hash_table_lookup(fs_super_htable, path);
	
	if (fs_super) {
		if (fs_super->fop.open) {
			ret = fs_super->fop.open(fs_super);
		}
	} else {
		ret = -ENOENT;
	}

/*
	if ((fi->flags & O_ACCMODE) != O_RDONLY)
		return -EACCES;
*/

	return ret;
}

static int fs_super_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	size_t len;
	(void) fi;
	int ret = 0;
	struct fs_super *fs_super;
	
	pr_debug("%s: path = %s, size = %ld, offset = %ld\n", __func__, path, size, offset);

	fs_super = g_hash_table_lookup(fs_super_htable, path);
	
	if (fs_super && fs_super->fop.read) {
		fs_super->read.buf = buf;
		fs_super->read.size = size;
		fs_super->read.offset = offset;
		ret = fs_super->fop.read(fs_super);
	} else {
		ret = -ENOENT;
	}

	return ret;
}

static int fs_super_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	int ret = 0;
	(void) path;
	struct fs_super *fs_super;
	
	pr_debug("%s: path = %s, size = %ld, offset = %ld, buf = %s\n", __func__, path, size, offset, buf);
	
	fs_super = g_hash_table_lookup(fs_super_htable, path);
	
	if (fs_super && fs_super->fop.write) {
		fs_super->write.buf = buf;
		fs_super->write.size = size;
		fs_super->write.offset = offset;
		ret = fs_super->fop.write(fs_super);
	} else {
		ret = -ENOENT;
	}

	return ret;
}

static int fs_super_release(const char *path, struct fuse_file_info *fi)
{
	int idx = fi->fh;
	(void) path;
	int ret = 0;
	struct fs_super *fs_super;

	pr_debug("%s: path = %s\n", __func__, path);

	fs_super = g_hash_table_lookup(fs_super_htable, path);
	
	if (fs_super) {
		if (fs_super->fop.close) {
			ret = fs_super->fop.close(fs_super);
		}
	} else {
		ret = -ENOENT;
	}

	return ret;
}

const struct fuse_operations fs_super_oper = {
	.init       = fs_super_init,
	.destroy    = fs_super_destroy,
	.getattr	= fs_super_getattr,
	.readdir	= fs_super_readdir,
	.open		= fs_super_open,
	.read		= fs_super_read,
	.write		= fs_super_write,
	.release	= fs_super_release,
};


static struct fs_super fs_root = {
	.info = {
		.name = "/",
		.parent = NULL,
		.mode = S_IFDIR,
	},
};
fs_initcall(fs_root);


int fs_nop_open(struct fs_super *fs)
{
	pr_debug("%s: path = %s\n", __func__, "");
	return 0;
}

int fs_nop_read(struct fs_super *fs)
{
	int len = 0;
	pr_debug("%s: path = %s\n", __func__, "");
	
	pr_debug("%s: fs->read.size = %ld\n", __func__, fs->read.size);
	pr_debug("%s: fs->read.offset = %ld\n", __func__, fs->read.offset);
	
	len = sprintf(fs->read.buf, "%s", "12345\n");
	
	return len;
}

int fs_nop_write(struct fs_super *fs)
{
	pr_debug("%s: path = %s\n", __func__, "");
	
	pr_debug("%s: fs->write.buf = %s\n", __func__, fs->write.buf);
	pr_debug("%s: fs->write.size = %ld\n", __func__, fs->write.size);
	pr_debug("%s: fs->write.offset = %ld\n", __func__, fs->write.offset);
	
	return fs->write.size;
}

int fs_nop_close(struct fs_super *fs)
{
	pr_debug("%s: path = %s\n", __func__, "");
	return 0;
}


