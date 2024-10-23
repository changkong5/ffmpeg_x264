/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * include/linker_lists.h
 *
 * Implementation of linker-generated arrays
 *
 * Copyright (C) 2012 Marek Vasut <marex@denx.de>
 */

#ifndef __FS_CUSE_SUPER_H__
#define __FS_CUSE_SUPER_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "initcall.h"

struct fs_cuse_super {
	char *name;
	void (*write)(const char *buf, int size);
};

/* function section define and declare */
struct fs_cuse_initcall {
	const char *name;
	const void *super;
};

/* static initcall function macro declare */
#define fs_cuse_initcall_sort(_super, _sort, _prio) \
	static ll_entry_declare(fs_cuse_initcall, _super, _sort, _prio) = { \
		.name = #_super,	\
		.super = &_super	\
	}

#define fs_cuse_initcall_prio(_super, _prio) \
		fs_cuse_initcall_sort(_super, 9, _prio)
		
#define fs_cuse_initcall(_super) fs_cuse_initcall_prio(_super, 9)


#define fs_cuse_init_fn(_name, _fs_cuse_write) 	    	\
	static struct fs_cuse_super __fs_cuse_##_name = {	\
		.name = #_name,				\
		.write = _fs_cuse_write,	\
	};								\
	fs_cuse_initcall(__fs_cuse_##_name)


#define fs_cuse_function(_name) fs_cuse_init_fn(_name, fs_cuse_##_name)


/* extern initcall function macro declare */
#define ext_fs_cuse_initcall_sort(_super, _sort, _prio) \
	ll_entry_declare(fs_cuse_initcall, _super, _sort, _prio) = { \
		.name = #_super,	\
		.super = &_super	\
	}

#define ext_fs_cuse_initcall_prio(_super, _prio) \
		ext_fs_cuse_initcall_sort(_super, 9, _prio)
		
#define ext_fs_cuse_initcall(_super) ext_fs_cuse_initcall_prio(_super, 9)

/* extern fcuse function declare */
void fuse_cuse_dev_name_minor(char *name, int minor);
void fuse_cuse_dev_name_major_minor(char *name, int major, int minor);

/*
static void fs_cuse_version(const char *buf, int size)
{
	for (int i = 0; i < size; i++) {
		pr_debug("%s: buf[%d] = 0x%02x (%c)\n", __func__, i, buf[i], buf[i]);
	}
	
	pr_debug("%s: buf = %s\n", __func__, buf);
	pr_debug("%s: size = %ld\n", __func__, size);
}
fs_cuse_function(version);
// fs_cuse_init_fn(version, fs_cuse_version);


// static struct fs_cuse_super fs_cuse_version = {
// 	.name = "version",
// 	.write = fs_cuse_write,
// };
// fs_cuse_initcall(fs_cuse_version);


// fuse_cuse_dev_name_minor("cuse_dev", 2);
*/

#endif /* __FS_CUSE_SUPER_H__ */


