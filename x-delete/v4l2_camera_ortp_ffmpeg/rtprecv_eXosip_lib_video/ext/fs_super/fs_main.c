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
#include <fuse.h>

#define DEBUG  1
#include "fs_super.h"

/* fs_super_oper variable declare */	
extern const struct fuse_operations fs_super_oper;

static __exit_0 void fuse_fs_quit(void)
{

}

int fuse_fs_main(int argc, char *argv[])
{
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	
#if 1
	args.argv[args.argc++] = strdup("-f");
	// args.argv[args.argc++] = strdup("/opt");
	args.argv[args.argc++] = strdup("/media");
	
//	args.argv[args.argc++] = strdup("-h");
//	args.argv[args.argc++] = strdup("-V");

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

	return fuse_main(args.argc, args.argv, &fs_super_oper, NULL);
}

/*
static void *fuse_fs_thread(void *arg)
{
	fuse_fs_main(0, NULL);
	return NULL;
}
thread_initcall(fuse_fs_thread);
*/

