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
#include <cuse_lowlevel.h>

#include "glib.h"

#define DEBUG  1
#include "fs_cuse_super.h"

static GHashTable *cuse_super_htable = NULL;

static void do_cuse_fs_initcall(void)
{
	int ret = 0;
	struct fs_cuse_super *info;
	struct fs_cuse_initcall *init;

	for (init = ll_entry_start(fs_cuse_initcall); 
		 init < ll_entry_end(fs_cuse_initcall); init++) {
		info = (struct fs_cuse_super *)init->super;
		
		pr_err("init->name = %s, info = %p, info->name = %s\n", init->name, info, info->name);
		
		if (info->name) {
			ret = g_hash_table_insert(cuse_super_htable, info->name, info);
		}

		if (ret == 0) {
			pr_err("init->name = %s, info->name = %s\n", init->name, info->name);
		}
	}
}

static void fs_cuse_super_init(void *userdata, struct fuse_conn_info *conn)
{
	(void) conn;
	pr_debug("%s: path = %s\n", __func__, "");

	cuse_super_htable = g_hash_table_new(g_str_hash, g_str_equal);

	if (cuse_super_htable != NULL) {
		do_cuse_fs_initcall();
	}

}

static void fs_cuse_super_init_done(void *userdata)
{
	pr_debug("%s: path = %s\n", __func__, "");
}

static void fs_cuse_super_destroy(void *userdata)
{
	pr_debug("%s: path = %s\n", __func__, "");

	if (cuse_super_htable != NULL) {
		g_hash_table_destroy(cuse_super_htable);
	}
	cuse_super_htable = NULL;
}

static void fs_cuse_super_open(fuse_req_t req, struct fuse_file_info *fi)
{
	// pr_debug("%s: \n", __func__);
	
	fuse_reply_open(req, fi);
}

/*
static void fs_cuse_super_read(fuse_req_t req, size_t size, off_t off,
			 struct fuse_file_info *fi)
{
	(void)fi;
	int len;
	static int done = 0;
	char buf[512];
	GHashTableIter iter;
	gpointer key, value;

	if (done) {
		done = 0;
		fuse_reply_buf(req, buf + off, 0);
		return;
	}
	
	if (!cuse_super_htable) {
		fuse_reply_write(req, ENOMEM);
		return;
	}
	
	pr_debug("%s: off = %ld, size = %ld\n", __func__, off, size);
	
	len = 0;
	memset(buf, 0, sizeof(buf));
	g_hash_table_iter_init(&iter, cuse_super_htable);
	
	while (g_hash_table_iter_next(&iter, &key, &value)) {
		len += sprintf(&buf[len], "%s\n", (char *)key);
		pr_info("%s: len = %d, key = %s, value = %p\n", __func__, len, key, value);
	}
	
	fuse_reply_buf(req, buf + off, len);
	
	if (len < sizeof(buf)) {
		done = 1;
	}
}
*/

static gint compare_strings(gconstpointer a, gconstpointer b)
{
  char **str_a = (char **)a;
  char **str_b = (char **)b;

  return strcmp(*str_a, *str_b);
}

static void fs_cuse_super_read(fuse_req_t req, size_t size, off_t off,
			 struct fuse_file_info *fi)
{
	(void)fi;
	int i, len;
	static int done = 0;
	
	char buf[1024];
	GPtrArray *parr;
	GHashTableIter iter;
	gpointer key, value;

	if (done) {
		done = 0;
		fuse_reply_buf(req, buf + off, 0);
		return;
	}
	
	if (!cuse_super_htable) {
		fuse_reply_write(req, ENOMEM);
		return;
	}
	
	pr_debug("%s: off = %ld, size = %ld\n", __func__, off, size);
	
	len = 0;
	memset(buf, 0, sizeof(buf));
	g_hash_table_iter_init(&iter, cuse_super_htable);
	
	parr = g_ptr_array_new(); 
	if (!parr) {
		fuse_reply_write(req, ENOMEM);
		return;
	}
	
	while (g_hash_table_iter_next(&iter, &key, &value)) {
		g_ptr_array_add(parr, key);
	}
	g_ptr_array_sort(parr, compare_strings);
	
	for(i = 0; i < parr->len; i++) {
		key = g_ptr_array_index(parr, i);
		len += sprintf(&buf[len], "%s\n", (char *)key);
    }
    pr_debug("%s: len = %ld, size = %ld\n", __func__, len, size);
	
	g_ptr_array_free(parr, TRUE);
	fuse_reply_buf(req, buf + off, len);
	
	if (len < sizeof(buf)) {
		done = 1;
	}
}

static void fs_cuse_super_write(fuse_req_t req, const char *buf, size_t size,
			  off_t off, struct fuse_file_info *fi)
{
	(void)fi;
	int ret, len = size;
	char *pchar, *newline;
	const char *method = buf;
	struct fs_cuse_super *fs_super;
	
	// pr_debug("%s: off = %ld, size = %ld, buf = %s\n", __func__, off, size, buf);
	
	for (int i = 0; i < size; i++) {
		// pr_debug("%s: buf[%02d] = 0x%02x, %c\n", __func__, i, buf[i], buf[i]);
	}
	
	if (size <= 0 || !cuse_super_htable) {
		fuse_reply_err(req, ENOMEM);
		return;
	}
	
	pchar = strchr(buf, ' ');
	if (pchar) {
		pchar[0] = 0;
		pchar++;
		len -= pchar - buf;
	} else {
		pchar = (char *)buf;
	}
	
	newline = strchr(pchar, '\n');
	if (newline) {
		newline[0] = 0;
		len -= 1;
	}
	
	if (pchar == buf) {
		len = 0;
		pchar = newline;
	}

	for (int i = 0; i < size; i++) {
		// pr_debug("%s: buf[%02d] = 0x%02x, %c\n", __func__, i, buf[i], buf[i]);
	}
	
	ret = ENOSYS;
	fs_super = g_hash_table_lookup(cuse_super_htable, method);
	
	if (fs_super && fs_super->write) {
		ret = size;
		fs_super->write(pchar, len);
	}
	
	fuse_reply_write(req, ret);
}

static void fs_cuse_super_release(fuse_req_t req, struct fuse_file_info *fi)
{
	// pr_debug("%s: \n", __func__);
	
	fuse_reply_err(req, 0);
}

const struct cuse_lowlevel_ops fs_cuse_super_op = {
	.init       = fs_cuse_super_init,
	.init_done  = fs_cuse_super_init_done,
	.destroy    = fs_cuse_super_destroy,
	.open		= fs_cuse_super_open,
	.read		= fs_cuse_super_read,
	.write		= fs_cuse_super_write,
	.release    = fs_cuse_super_release,
};


/*
struct cuse_lowlevel_ops {
	void (*init) (void *userdata, struct fuse_conn_info *conn);
	void (*init_done) (void *userdata);
	void (*destroy) (void *userdata);
	void (*open) (fuse_req_t req, struct fuse_file_info *fi);
	void (*read) (fuse_req_t req, size_t size, off_t off,
		      struct fuse_file_info *fi);
	void (*write) (fuse_req_t req, const char *buf, size_t size, off_t off,
		       struct fuse_file_info *fi);
	void (*flush) (fuse_req_t req, struct fuse_file_info *fi);
	void (*release) (fuse_req_t req, struct fuse_file_info *fi);
	void (*fsync) (fuse_req_t req, int datasync, struct fuse_file_info *fi);
	void (*ioctl) (fuse_req_t req, int cmd, void *arg,
		       struct fuse_file_info *fi, unsigned int flags,
		       const void *in_buf, size_t in_bufsz, size_t out_bufsz);
	void (*poll) (fuse_req_t req, struct fuse_file_info *fi,
		      struct fuse_pollhandle *ph);
};
*/



