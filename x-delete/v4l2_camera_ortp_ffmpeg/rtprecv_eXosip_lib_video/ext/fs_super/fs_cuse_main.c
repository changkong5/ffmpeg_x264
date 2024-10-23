/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPLv2.
  See the file COPYING.
*/

/** @file
 *
 * minimal example filesystem using high-level API
 *
 * Compile with:
 *
 *     gcc -Wall hello.c `pkg-config fuse3 --cflags --libs` -o hello
 *
 * ## Source code ##
 * \include hello.c
 */
#define FUSE_USE_VERSION 34

#include <stdio.h>
#include <unistd.h>
#include <cuse_lowlevel.h>

#define DEBUG  1
#include "fs_cuse_super.h"

/* fs_super_oper variable declare */	
extern const struct cuse_lowlevel_ops fs_cuse_super_op;

static char dev_name[128] = "DEVNAME=fs_cuse";

static const char *fs_cuse_dev_info_argv[] = { 
	dev_name
};

static struct cuse_info fs_cuse_ci_info = {
	.dev_major = 200,
	.dev_minor = 1,
	.dev_info_argc = 1,
	.dev_info_argv = fs_cuse_dev_info_argv,
	.flags = CUSE_UNRESTRICTED_IOCTL,
};

static struct cuse_info *fs_cuse_ci = &fs_cuse_ci_info;


static void fuse_cuse_dev_name(char *name)
{
	if (!name || strlen(name) == 0) {
		pr_err("%s: <error> name\n");
		return;
	}

	memset(dev_name, 0, sizeof(dev_name));
	sprintf(dev_name, "DEVNAME=%s", name);
}

static void fuse_cuse_dev_major(int major)
{
	fs_cuse_ci_info.dev_major = major;
}

static void fuse_cuse_dev_minor(int minor)
{
	fs_cuse_ci_info.dev_minor = minor;
}

void fuse_cuse_dev_name_minor(char *name, int minor)
{
	fuse_cuse_dev_name(name);
	fuse_cuse_dev_minor(minor);
}

void fuse_cuse_dev_name_major_minor(char *name, int major, int minor)
{
	fuse_cuse_dev_name(name);
	fuse_cuse_dev_major(major);
	fuse_cuse_dev_minor(minor);
}

static __exit_0 void fuse_cuse_quit(void)
{

}

int fuse_cuse_main(int argc, char *argv[])
{
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	
#if 1
	args.argv[args.argc++] = strdup("-f");
	// args.argv[args.argc++] = strdup("/opt");
	args.argv[args.argc++] = strdup("/media");

#if 1
	printf("------------1-----------------\n");	
	printf("args.argc = %d\n", args.argc);
	printf("args.argv = %p\n", args.argv);
	printf("args.allocated = %d\n", args.allocated);
	
	for (int i = 0; i < args.argc; i++) {
		printf("args.argv[%d] = %s\n", i, args.argv[i]);
	}
#endif
#endif

	return cuse_lowlevel_main(args.argc, args.argv, fs_cuse_ci, &fs_cuse_super_op, NULL);
}


