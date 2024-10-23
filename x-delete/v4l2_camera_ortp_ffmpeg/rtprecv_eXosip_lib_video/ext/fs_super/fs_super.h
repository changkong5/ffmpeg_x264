/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * include/linker_lists.h
 *
 * Implementation of linker-generated arrays
 *
 * Copyright (C) 2012 Marek Vasut <marex@denx.de>
 */

#ifndef __FS_SUPER_H__
#define __FS_SUPER_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "initcall.h"

struct fs_info {
	char *name;
	char *parent;
	mode_t mode;
	void *private;
};

struct fs_read {
	char *buf;
	size_t size;
	off_t offset;
};

struct fs_write {
	const char *buf;
	size_t size;
	off_t offset;
};

struct fs_super;
struct fs_operations {
	int (*open)(struct fs_super *fs);
	int (*read)(struct fs_super *fs);
	int (*write)(struct fs_super *fs);
	int (*close)(struct fs_super *fs);
};

struct fs_super {
	struct fs_info info;
	union {
		struct fs_read read;
		struct fs_write write;
	};
	struct fs_operations fop;
};

struct fs_dir_super {
	struct fs_info info;
};

/* function section define and declare */
struct fs_initcall {
	const char *name;
	const void *super;
};

/* static initcall function macro declare */
#define fs_initcall_sort(_super, _sort, _prio) \
	static ll_entry_declare(fs_initcall, _super, _sort, _prio) = { \
		.name = #_super,	\
		.super = &_super	\
	}
	
#define fs_initcall_prio(_super, _prio) \
		fs_initcall_sort(_super, 9, _prio)
		
#define fs_initcall(_super) fs_initcall_prio(_super, 9)


/* extern initcall function macro declare */
#define ext_fs_initcall_sort(_super, _sort, _prio) \
	ll_entry_declare(fs_initcall, _super, _sort, _prio) = { \
		.name = #_super,	\
		.super = &_super	\
	}

#define ext_fs_initcall_prio(_super, _prio) \
		ext_fs_initcall_sort(_super, 9, _prio)
		
#define ext_fs_initcall(_super) ext_fs_initcall_prio(_super, 9)


/* extern fs operate function declare */
int fs_nop_open(struct fs_super *fs);
int fs_nop_close(struct fs_super *fs);
int fs_nop_read(struct fs_super *fs);
int fs_nop_write(struct fs_super *fs);



#if 0
/* static fs super file define and declare */
#define fs_super_create_file(_name, _parent, _open, _read, _write, _close) \
	static struct fs_super _fs_super_file_##_name = {	\
		.info = {								\
			.name = #_name,						\
			.parent = #_parent,					\
			.mode = S_IFREG,					\
		},										\
		.fop = {								\
			.open = _open,						\
			.read = _read,						\
			.write = _write,					\
			.close = _close						\
		},										\
	};											\
	fs_initcall(_fs_super_file_##_name)
	
	
	
/* static fs super dirent define and declare */
#define fs_super_create_dir(_name, _parent)	\
	static struct fs_dir_super _fs_super_dir_##_name = {	\
		.info = {								\
			.name = #_name,						\
			.parent = #_parent,					\
			.mode = S_IFDIR,					\
		},										\
	};											\
	fs_initcall(_fs_super_dir_##_name)	


/* static fs super root file define and declare */
#define fs_super_root_file(_name, _open, _read, _write, _close)	\
		fs_super_create_file(_name, /, _open, _read, _write, _close)

/* static fs super root dirent define and declare */
#define fs_super_root_dir(_name) fs_super_create_dir(_name, /)
	
	
	
#define fs_super_root_read_file(_name, _open, _read, _close)	\
		fs_super_root_file(_name, _open, _read, NULL, _close)
		
#define fs_super_root_write_file(_name, _open, _write, _close) \
		fs_super_root_file(_name, _open, NULL, _write, _close)


#define fs_super_root_read_write(_name, _read, _write)	\
		fs_super_root_file(_name, NULL, _read, _write, NULL)	

#define fs_super_root_read_only(_name, _read)	\
		fs_super_root_file(_name, NULL, _read, NULL, NULL)
		
#define fs_super_root_write_only(_name, _write)	\
		fs_super_root_file(_name, NULL, NULL, _write, NULL)
		

		
#define FS_SUPER_ROOT_CREAT_FILE(_name, _open, _read, _write, _close)	\
		fs_super_create_file(_name, /, _open, _read, _write, _close)
		
#define FS_SUPER_ROOT_CREAT_DIR(_name) 	fs_super_create_dir(_name, /)
#endif

#endif /* __FS_SUPER_H__ */


