/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * include/linker_lists.h
 *
 * Implementation of linker-generated arrays
 *
 * Copyright (C) 2012 Marek Vasut <marex@denx.de>
 */

#ifndef __FS_LL_SUPER_H__
#define __FS_LL_SUPER_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#ifndef FUSE_USE_VERSION
#define FUSE_USE_VERSION 34
#endif
#include <fuse_lowlevel.h>

#include "initcall.h"
	
struct fs_ll_super;

struct fs_ll_info {
	char *name;
	fuse_ino_t ino;
	fuse_ino_t parent;
	mode_t mode;
	void *private;
};

struct fs_ll_read {
	size_t size;
	off_t offset;
};

struct fs_ll_write {
	const char *buf;
	size_t size;
	off_t offset;
};

struct fs_ll_comm {
	fuse_req_t req;
	fuse_ino_t ino;
	struct fuse_file_info *fi;
};

// typedef uint64_t fuse_ino_t;
// 

struct fs_ll_operations {
	void (*open)(struct fs_ll_super *fs);
	void (*read)(struct fs_ll_super *fs);
	void (*write)(struct fs_ll_super *fs);
	void (*close)(struct fs_ll_super *fs);
};

struct fs_ll_super {
	struct fs_ll_info info;
	struct fs_ll_comm comm;
	union {
		struct fs_ll_read read;
		struct fs_ll_write write;
	};
	struct fs_ll_operations fop;
};

struct fs_ll_dir_super {
	struct fs_ll_info info;
};

/* fs_super_oper variable declare */	
extern const struct fuse_lowlevel_ops fs_ll_super_oper;

/* function section define and declare */
struct fs_ll_initcall {
	const char *name;
	const void *super;
};

/* static initcall function macro declare */
#define fs_ll_initcall_sort(_super, _sort, _prio) \
	static ll_entry_declare(fs_ll_initcall, _super, _sort, _prio) = { \
		.name = #_super,	\
		.super = &_super	\
	}
	
#define fs_ll_initcall_prio(_super, _prio) \
		fs_ll_initcall_sort(_super, 9, _prio)
		
#define fs_ll_initcall(_super) fs_ll_initcall_prio(_super, 9)


/* extern initcall function macro declare */
#define ext_fs_ll_initcall_sort(_super, _sort, _prio) \
	ll_entry_declare(fs_ll_initcall, _super, _sort, _prio) = { \
		.name = #_super,	\
		.super = &_super	\
	}

#define ext_fs_ll_initcall_prio(_super, _prio) \
		ext_fs_ll_initcall_sort(_super, 9, _prio)
		
#define ext_fs_ll_initcall(_super) ext_fs_ll_initcall_prio(_super, 9)


/* extern fs lowlevel operate function declare */
void fs_ll_nop_open(struct fs_ll_super *fs);
void fs_ll_nop_close(struct fs_ll_super *fs);
void fs_ll_nop_read(struct fs_ll_super *fs);
void fs_ll_nop_write(struct fs_ll_super *fs);


#if 0
/* static fs super low level file define and declare */
#define fs_ll_super_create_file(_name, _ino, _parent, _open, _read, _write, _close) \
	static struct fs_ll_super _fs_super_ll_file_##_name = {	\
		.info = {								\
			.name = #_name,						\
			.ino = _ino,						\
			.parent = _parent,					\
			.mode = S_IFREG,					\
		},										\
		.fop = {								\
			.open = _open,						\
			.read = _read,						\
			.write = _write,					\
			.close = _close						\
		},										\
	};											\
	fs_ll_initcall(_fs_super_ll_file_##_name)


/* static fs super low level dirent define and declare */
#define fs_ll_super_create_dir(_name, _ino, _parent)	\
	static struct fs_ll_dir_super _fs_super_ll_dir_##_name = {	\
		.info = {								\
			.name = #_name,						\
			.ino = _ino,						\
			.parent = _parent,					\
			.mode = S_IFDIR,					\
		},										\
	};											\
	fs_ll_initcall(_fs_super_ll_dir_##_name)


/* static fs super low level root file define and declare */
#define fs_ll_super_root_file(_name, _ino, _open, _read, _write, _close) \
		fs_ll_super_create_file(_name, _ino, FUSE_ROOT_ID, _open, _read, _write, _close)

/* static fs super low level root dirent define and declare */
#define fs_ll_super_root_dir(_name, _ino) \
			fs_ll_super_create_dir(_name, _ino, FUSE_ROOT_ID)
		
		
		
#define fs_ll_super_root_read_file(_name, _ino, _open, _read, _close)	\
			fs_ll_super_root_file(_name, _ino, _open, _read, NULL, _close)	
		
#define fs_ll_super_root_write_file(_name, _ino, _open, _write, _close) \
			fs_ll_super_root_file(_name, _ino, _open, NULL, _write, _close)
		
		
#define fs_ll_super_root_read_write(_name, _ino, _read, _write)	\
			fs_ll_super_root_file(_name, _ino,		\
								  fs_ll_nop_open, 	\
								  _read, _write, 	\
								  fs_ll_nop_close)
		
#define fs_ll_super_root_read_only(_name, _ino, _read)	\
			fs_ll_super_root_file(_name, _ino,		\
								  fs_ll_nop_open, 	\
								  _read, NULL, 	\
								  fs_ll_nop_close)
								  
#define fs_ll_super_root_write_only(_name, _ino, _write) \
			fs_ll_super_root_file(_name, _ino,		\
								  fs_ll_nop_open, 	\
								  NULL, _write, 	\
								  fs_ll_nop_close)							  
								  
#endif

#endif /* __FS_LL_SUPER_H__ */


