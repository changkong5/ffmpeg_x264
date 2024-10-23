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

#if 1
const char *version = "zeplin 2021-07-30";

const char *sys_version(void)
{
	return version;
}

/*
int sys_version_fn(void)
{
	k_printk("system kernel version %s\n", sys_version());

	return 0;
}
sys_fn_initcall(sys_version_fn);
*/

static void subsys_version(int argc, char *argv[])
{
	pr_info("%s\n", sys_version());
}

cmd_initcall(version, subsys_version);

#endif

