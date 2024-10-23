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

static int sys_reboot(void)
{
	return 0;
}

static void subsys_reboot(int argc, char *argv[])
{
	pr_info("%s: argc = %d\n", __func__, argc);

	for (int i = 0; i < argc; i++) {
		pr_info("%s: argv[%d] = %s\n", __func__, i, argv[i]);
	}

	sys_reboot();
}
cmd_initcall(reboot, subsys_reboot);
