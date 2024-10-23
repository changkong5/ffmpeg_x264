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

static int shell_prompt(void)
{
	pr_info(">> ");
    fflush(stdout);

	return 0;
}

static void *shell_entry(void *arg)
{
    char input[1024];
    char *command = input;

    while (1) {
    	shell_prompt();

    	int n = read(STDIN_FILENO, input, sizeof(input));
	    //fgets(input, 1024, stdin);
	    //fread(input, 1, 1024, stdin);
	    
    	char *str = strchr(command, '\n');

		if (str != NULL) {
			*str = '\0';
		}

		int len = strlen(command);

		if (len == 0) {
			continue;
		}

		int i, argc = 0;
		char *argv[K_CMD_MAXARGS];

		argc = k_cmd_parse_line(command, argv);

		if (k_cmd_run_command(argc, argv) < 0) {
			pr_info("command not found\n");
		}

		fflush(stdin);
    }

    return NULL;
}
thread_initcall(shell_entry);



