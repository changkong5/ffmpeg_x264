/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPLv2.
  See the file COPYING.
*/

/** @file
 *
 * minimal example filesystem using low-level API
 *
 * Compile with:
 *
 *     gcc -Wall hello_ll.c `pkg-config fuse3 --cflags --libs` -o hello_ll
 *
 * ## Source code ##
 * \include hello_ll.c
 */
#define FUSE_USE_VERSION 34
#include <fuse_lowlevel.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DEBUG  1
#include "fs_ll_super.h"

static struct fuse_session *se = NULL;

static __exit_0 void fuse_ll_quit(void)
{
	if (!se) {
		return;
	}
	printf("--------1---13---\n");
	fuse_session_exit(se);
}

int fuse_ll_main(int argc, char *argv[])
{
	//struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	// struct fuse_session *se;
	struct fuse_cmdline_opts opts;
	struct fuse_loop_config config;
	int ret = -1;

#if 1
	struct fuse_args args = FUSE_ARGS_INIT(0, NULL);
	args.argv = malloc(16 * sizeof(char *));
	args.argv[args.argc++] = strdup("./main");

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

	if (fuse_parse_cmdline(&args, &opts) != 0)
		return 1;
		
	if (opts.show_help) {
		printf("usage: %s [options] <mountpoint>\n\n", argv[0]);
		fuse_cmdline_help();
		fuse_lowlevel_help();
		ret = 0;
		goto err_out1;
	} else if (opts.show_version) {
		printf("FUSE library version %s\n", fuse_pkgversion());
		fuse_lowlevel_version();
		ret = 0;
		goto err_out1;
	}

	if(opts.mountpoint == NULL) {
		printf("usage: %s [options] <mountpoint>\n", argv[0]);
		printf("       %s --help\n", argv[0]);
		ret = 1;
		goto err_out1;
	}

	se = fuse_session_new(&args, &fs_ll_super_oper,
			      sizeof(fs_ll_super_oper), NULL);
	if (se == NULL)
	    goto err_out1;

	if (fuse_set_signal_handlers(se) != 0)
	    goto err_out2;

	if (fuse_session_mount(se, opts.mountpoint) != 0)
	    goto err_out3;

	fuse_daemonize(opts.foreground);

	/* Block until ctrl+c or fusermount -u */
	if (opts.singlethread)
		ret = fuse_session_loop(se);
	else {
		config.clone_fd = opts.clone_fd;
		config.max_idle_threads = opts.max_idle_threads;
		ret = fuse_session_loop_mt(se, &config);
	}

	fuse_session_unmount(se);
err_out3:
	fuse_remove_signal_handlers(se);
err_out2:
	fuse_session_destroy(se);
err_out1:
	free(opts.mountpoint);
	fuse_opt_free_args(&args);

	return ret ? 1 : 0;
}

/*
static void *fuse_ll_thread(void *arg)
{
	fuse_ll_main(0, NULL);
	return NULL;
}
thread_initcall(fuse_ll_thread);
*/


