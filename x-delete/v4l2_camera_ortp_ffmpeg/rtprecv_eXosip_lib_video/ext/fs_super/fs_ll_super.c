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
#define FUSE_USE_VERSION 34
 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fuse_lowlevel.h>

#include "glib.h"

#define DEBUG  1
#include "fs_ll_super.h"


static GHashTable *fs_ll_super_htable = NULL;

static void do_ll_fs_initcall(void)
{
	int ret = 0;
	struct fs_ll_info *info;
	struct fs_ll_initcall *init;

	for (init = ll_entry_start(fs_ll_initcall); 
		 init < ll_entry_end(fs_ll_initcall); init++) {
		info = (struct fs_ll_info *)init->super;
		
		pr_err("init->name = %s, info = %p, info->name = %s, info->ino = %ld\n", init->name, info, info->name, info->ino);
		
		if (info->ino > 0) {
			ret = g_hash_table_insert(fs_ll_super_htable, (int64_t *)&info->ino, info);
		}

		if (ret == 0 || info->ino == 0) {
			pr_err("init->name = %s, info->name = %s\n", init->name, info->name);
		}
	}
}

static void fs_ll_super_init(void *userdata, struct fuse_conn_info *conn)
{
	(void) conn;
	pr_debug("%s: path = %s\n", __func__, "");
	
	fs_ll_super_htable = g_hash_table_new(g_int64_hash, g_int64_equal);
	// fs_ll_super_htable = g_hash_table_new_full(g_int64_hash, g_int64_equal, NULL, NULL);

	if (fs_ll_super_htable != NULL) {
		do_ll_fs_initcall();
	}
}

static void fs_ll_super_destroy(void *userdata)
{
	pr_debug("%s: path = %s\n", __func__, "");

	g_hash_table_destroy(fs_ll_super_htable);
}

static void fs_ll_super_getattr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
	(void) fi;
	struct stat st[1];

	int err = 0;
	mode_t mode = 0;
	struct fs_ll_info *info;
	struct fs_ll_super *fs_super;

	info = g_hash_table_lookup(fs_ll_super_htable, &ino);
	pr_debug("%s: ino = %ld (0x%08lx), info = %p, info->ino = %ld, info->name = %s\n", __func__, ino, ino, info, info->ino, info->name);

	memset(&st, 0, sizeof(st));
	
	if (info == NULL) {
		fuse_reply_err(req, ENOENT);
		return;
	}
	
	st->st_ino = info->ino;
	
	if (info->mode == S_IFDIR) {
		st->st_mode = S_IFDIR | 0755;
		st->st_nlink = 2;
	} else {	
		// fs_super->info.mode = S_IFREG
		fs_super = (struct fs_ll_super *)info;
	
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
	
	fuse_reply_attr(req, st, 1.0);
	
//	if (hello_stat(ino, &stbuf) == -1)
//		fuse_reply_err(req, ENOENT);
//	else
//		fuse_reply_attr(req, &stbuf, 1.0);
}

static void fs_ll_super_lookup(fuse_req_t req, fuse_ino_t parent, const char *name)
{
	int find = 0;
	mode_t mode = 0;
	GHashTableIter iter;
	gpointer key, value;
	struct stat *st;
	struct fs_ll_info *info;
	struct fs_ll_super *fs_super;
	struct fuse_entry_param e;
	
	g_hash_table_iter_init(&iter, fs_ll_super_htable);
	
	while (g_hash_table_iter_next(&iter, &key, &value)) {
		info = (struct fs_ll_info *)value;
		
		if (info->parent != parent) {
			continue;
		}
	
		if (strcmp(info->name, name) == 0) {
			find = 1;
			break;
		}	
	}
	
	if (find == 0) {
		fuse_reply_err(req, ENOENT);
		return;
	}
	
	pr_debug("%s: ino = %ld, info = %p, name = %s, info->ino = %ld, info->parent = %ld \n", __func__, parent, info, name, info->ino, info->parent);

	memset(&e, 0, sizeof(e));
	
	st = &e.attr;
	e.ino = info->ino;
	e.attr_timeout = 1.0;
	e.entry_timeout = 1.0;
	
	st->st_ino = info->ino;
	
	if (info->mode == S_IFDIR) {
		st->st_mode = S_IFDIR | 0755;
		st->st_nlink = 2;
	} else {	
		// fs_super->info.mode = S_IFREG
		fs_super = (struct fs_ll_super *)info;
	
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
	
	fuse_reply_entry(req, &e);
}

static void fs_ll_super_readdir(fuse_req_t req, fuse_ino_t ino, size_t size,
			     				off_t off, struct fuse_file_info *fi)
{
	(void) fi;
	
	char *buf;
	char *p;
	size_t rem = size;
	
	size_t entsize;
	off_t nextoff = 0;
	const char *name;
	
	char *path[2] = {".", ".."};
	
	int err = 0, find = 0;
	mode_t mode = 0;
	GHashTableIter iter;
	gpointer key, value;
	struct stat st[1];
	struct fs_ll_info *info;
	struct fs_ll_super *fs_super;
	
	fuse_ino_t parent = ino;
	
	pr_debug("%s: ino = %ld (0x%08lx), size = %ld, off = %ld\n", __func__, ino, ino, size, off);

	buf = calloc(1, size);
	
	if (!buf) {
		err = ENOMEM;
		goto error;
	}
	p = buf;
	
	nextoff = 0;
	memset(&st, 0, sizeof(st));

	for (int i = 0; i < 2; i++) {
		st->st_ino = 1;
		st->st_mode = S_IFDIR | 0755;
		st->st_nlink = 2;
		
		entsize = fuse_add_direntry(req, NULL, 0, path[i], NULL, 0);
		nextoff += entsize;

		fuse_add_direntry(req, p, entsize, path[i], st, nextoff);
		p += entsize;
	}

	g_hash_table_iter_init(&iter, fs_ll_super_htable);
	
	while (g_hash_table_iter_next(&iter, &key, &value)) {
		info = (struct fs_ll_info *)value;
		
		pr_debug("%s: info->parent = %ld, info->ino = %ld (0x%08lx), info->name = %s\n", __func__, info->parent, info->ino, info->ino, info->name);
		
		if (info->parent != parent) {
			continue;
		}
		
		st->st_ino = info->ino;
	
		if (info->mode == S_IFDIR) {
			st->st_mode = S_IFDIR | 0755;
			st->st_nlink = 2;
		} else {	
			// fs_super->info.mode = S_IFREG
			fs_super = (struct fs_ll_super *)info;
		
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
		
		entsize = fuse_add_direntry(req, NULL, 0, info->name, NULL, 0);
		nextoff += entsize;
		
		fuse_add_direntry(req, p, entsize, info->name, st, nextoff);
		// fuse_add_direntry(req, p, rem, info->name, st, nextoff);
		
		p += entsize;
		// rem -= entsize;
		
		
		pr_debug("%s: entsize = %ld, size = %ld, rem = %ld, nextoff = %ld\n", __func__, entsize, size, rem, nextoff);
	}

    err = 0;
error:

	pr_debug("%s: err = %d, size = %ld, rem = %ld, off = %ld, nextoff = %ld\n", __func__, err, size, rem, off, nextoff);
	
    // If there's an error, we can only signal it if we haven't stored
    // any entries yet - otherwise we'd end up with wrong lookup
    // counts for the entries that are already in the buffer. So we
    // return what we've collected until that point.
/*   
    if (err && rem == size) 
	    fuse_reply_err(req, err);
    else
	    fuse_reply_buf(req, buf + off, size - rem);
*/

	if (off < nextoff)
		fuse_reply_buf(req, buf + off, nextoff);
	else
		fuse_reply_buf(req, NULL, 0);

    free(buf);
}

static void fs_ll_super_open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
	struct fs_ll_super *fs_super;
	pr_debug("%s: ino = %ld (0x%08lx)\n", __func__, ino, ino);
	
	fs_super = g_hash_table_lookup(fs_ll_super_htable, &ino);
	
	if (fs_super) {
		if (fs_super->fop.open) {
			fs_super->comm.req = req;
			fs_super->comm.ino = ino;
			fs_super->comm.fi = fi;
			fs_super->fop.open(fs_super);
		} else {
			fuse_reply_err(req, ENOSYS);
		}
	} else {
		fuse_reply_err(req, ENOENT);
	}
}

static void fs_ll_super_read(fuse_req_t req, fuse_ino_t ino, size_t size,
			  off_t off, struct fuse_file_info *fi)
{
	(void) fi;
	struct fs_ll_super *fs_super;
	pr_debug("%s: ino = %ld (0x%08lx)\n", __func__, ino, ino);
	
	fs_super = g_hash_table_lookup(fs_ll_super_htable, &ino);
	
	if (fs_super) {
		if (fs_super->fop.read) {
			fs_super->comm.req = req;
			fs_super->comm.ino = ino;
			fs_super->comm.fi = fi;
			fs_super->read.size = size;
			fs_super->read.offset = off;
			fs_super->fop.read(fs_super);
		} else {
			fuse_reply_err(req, ENOSYS);
		}
	} else {
		fuse_reply_err(req, ENOENT);
	}
}

static void fs_ll_super_write(fuse_req_t req, fuse_ino_t ino, const char *buf, 
							  size_t size, off_t off, struct fuse_file_info *fi)
{
	(void) fi;
	struct fs_ll_super *fs_super;
	pr_debug("%s: ino = %ld (0x%08lx)\n", __func__, ino, ino);
	
	fs_super = g_hash_table_lookup(fs_ll_super_htable, &ino);
	
	if (fs_super) {
		if (fs_super->fop.write) {
			fs_super->comm.req = req;
			fs_super->comm.ino = ino;
			fs_super->comm.fi = fi;
			fs_super->write.buf = buf;
			fs_super->write.size = size;
			fs_super->write.offset = off;
			fs_super->fop.write(fs_super);
		} else {
			fuse_reply_err(req, ENOSYS);
		}
	} else {
		fuse_reply_err(req, ENOENT);
	}
}

static void fs_ll_super_release(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
	struct fs_ll_super *fs_super;
	pr_debug("%s: ino = %ld (0x%08lx)\n", __func__, ino, ino);
	
	fs_super = g_hash_table_lookup(fs_ll_super_htable, &ino);
	
	if (fs_super) {
		if (fs_super->fop.close) {
			fs_super->comm.req = req;
			fs_super->comm.ino = ino;
			fs_super->comm.fi = fi;
			fs_super->fop.close(fs_super);
		} else {
			fuse_reply_err(req, ENOSYS);
		}
	} else {
		fuse_reply_err(req, ENOENT);
	}
}

const struct fuse_lowlevel_ops fs_ll_super_oper = {
	.init    = fs_ll_super_init,
	.destroy = fs_ll_super_destroy,
	.lookup  = fs_ll_super_lookup,
	.getattr = fs_ll_super_getattr,
	.readdir = fs_ll_super_readdir,
	.open    = fs_ll_super_open,
	.read    = fs_ll_super_read,
	.write	 = fs_ll_super_write,
	.release = fs_ll_super_release,
};

static struct fs_ll_super fs_ll_root = {
	.info = {
		.name = "/",
		.ino = FUSE_ROOT_ID,
		.parent = 0,
		.mode = S_IFDIR,
	},
};
fs_ll_initcall(fs_ll_root);


void fs_ll_nop_open(struct fs_ll_super *fs)
{
	fuse_req_t req = fs->comm.req;
	fuse_ino_t ino = fs->comm.ino;
	struct fuse_file_info *fi = fs->comm.fi;

	pr_debug("%s: path = %s, req = %p, ino = %ld\n", __func__, "", req, ino);
	
	fuse_reply_open(req, fi);
}

void fs_ll_nop_close(struct fs_ll_super *fs)
{
	fuse_req_t req = fs->comm.req;
	fuse_ino_t ino = fs->comm.ino;
	
	pr_debug("%s: path = %s, req = %p, ino = %ld\n", __func__, "", req, ino);
	
	fuse_reply_err(req, 0);
}

void fs_ll_nop_read(struct fs_ll_super *fs)
{
	int len;
	fuse_req_t req = fs->comm.req;
	fuse_ino_t ino = fs->comm.ino;
	size_t size = fs->read.size;
	off_t offset = fs->read.offset;
	
	pr_debug("%s: path = %s, req = %p, ino = %ld\n", __func__, "", req, ino);
	
	pr_debug("%s: fs->read.size = %ld\n", __func__, fs->read.size);
	pr_debug("%s: fs->read.offset = %ld\n", __func__, fs->read.offset);

	fuse_reply_buf(req, NULL, 0);
}

void fs_ll_nop_write(struct fs_ll_super *fs)
{
	int len;
	fuse_req_t req = fs->comm.req;
	fuse_ino_t ino = fs->comm.ino;
	size_t size = fs->write.size;
	off_t offset = fs->write.offset;
	const char *buf = fs->write.buf;

	pr_debug("%s: path = %s, req = %p, ino = %ld\n", __func__, "", req, ino);
	
	for (int i = 0; i < size; i++) {
		pr_debug("%s: fs->write.buf[%d] = 0x%02x (%c)\n", __func__, fs->write.buf[i], fs->write.buf[i]);
	}
	
	pr_debug("%s: fs->write.buf = %s\n", __func__, fs->write.buf);
	pr_debug("%s: fs->write.size = %ld\n", __func__, fs->write.size);
	pr_debug("%s: fs->write.offset = %ld\n", __func__, fs->write.offset);
	
	fuse_reply_write(req, size);
}



