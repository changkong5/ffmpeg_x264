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

struct k_cmd *k_cmd_find_foreach(char *name)
{
	struct k_cmd *cmd;

	for (cmd = ll_entry_start(k_cmd); 
		 cmd < ll_entry_end(k_cmd); cmd++) {
		 
		int len = strlen(name);
		 
		if (strncmp(name, cmd->name, len) != 0) {
			continue;
		}

		if (len != strlen(cmd->name)) {
			continue;
		}

		return cmd;
	}
	
	return NULL;
}

void k_cmd_display_foreach(void)
{
	struct k_cmd *cmd;

	for (cmd = ll_entry_start(k_cmd); 
		 cmd < ll_entry_end(k_cmd); cmd++) {
		 
		 pr_info("cmd->name = %s\n", cmd->name);
	}

}

int k_cmd_foreach(int (*callback)(const struct k_cmd *cmd))
{
	const struct k_cmd *cmd;

	for (cmd = ll_entry_start(k_cmd); 
		 cmd < ll_entry_end(k_cmd); cmd++) {
		 
		int ret = callback(cmd);

		if (ret != 0) {
			pr_info("ret = %d, init->name = %s", ret, cmd->name);
			return ret;
		}
	}

	return 0;
}

struct k_cmd *k_cmd_find_cmd(char *commad)
{
	return k_cmd_find_foreach(commad);
}

int k_cmd_run_command(int argc, char *argv[])
{
	struct k_cmd *cmd;

	cmd = k_cmd_find_cmd(argv[0]);

	if (cmd == NULL) {
		return -1;
	}
	
	cmd->fn(argc - 1, &argv[1]);

	return 0;
}

int k_cmd_parse_line(char *line, char *argv[])
{
	int nargs = 0;

	while (nargs < K_CMD_MAXARGS) {
		/* skip any white space */
		while (isblank(*line)) {
			++line;
		}

		if (*line == '\0') {	/* end of line, no more args	*/
			argv[nargs] = NULL;
			return nargs;
		}

		argv[nargs++] = line;	/* begin of argument string	*/

		/* find end of string */
		while (*line && !isblank(*line)){
			++line;
		}

		if (*line == '\0') {	/* end of line, no more args	*/
			argv[nargs] = NULL;
			return nargs;
		}

		*line++ = '\0';		/* terminate current arg	 */
	}

	return nargs;
}

