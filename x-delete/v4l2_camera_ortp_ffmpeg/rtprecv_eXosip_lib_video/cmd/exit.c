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
#include "cmd.h"
#include "initcall.h"


static void cmd_exit(int argc, char *argv[])
{
	pr_info("%s: argc = %d\n", __func__, argc);

	for (int i = 0; i < argc; i++) {
		pr_info("%s: argv[%d] = %s\n", __func__, i, argv[i]);
	}
	
	pid_t pid = 0;
	
	//int result = kill(pid, SIGTERM);
	int result = kill(pid, SIGINT);
	
	

	// mainloop_quit();
}
cmd_initcall(exit, cmd_exit);
