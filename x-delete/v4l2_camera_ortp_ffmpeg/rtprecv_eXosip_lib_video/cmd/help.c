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


static int count = 0;

static int cmd_callback(const struct k_cmd *cmd)
{
	pr_info("%s \t", cmd->name);
	
	count++;

	if (count == 6) {
		count = 0;
		pr_info("\n");
	}

	return 0;
}

static void subsys_help(int argc, char *argv[])
{
	count = 0;

	k_cmd_foreach(cmd_callback);
	
	if (count != 0) {
		pr_info("\n");
	}
}

cmd_initcall(help, subsys_help);


