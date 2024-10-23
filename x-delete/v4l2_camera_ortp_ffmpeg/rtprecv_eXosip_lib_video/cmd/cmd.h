/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * include/linker_lists.h
 *
 * Implementation of linker-generated arrays
 *
 * Copyright (C) 2012 Marek Vasut <marex@denx.de>
 */

#ifndef __CMD_H__
#define __CMD_H__

#include <ctype.h>

#define K_CMD_MAXARGS		64

struct k_cmd {
	const char *name;
	void (*fn)(int argc, char *argv[]);
};

/* static initcall thread macro declare */
#define cmd_initcall_sort(_name, _fn, _sort, _prio) \
	static ll_entry_declare(k_cmd, _name, _sort, _prio) = { \
		.name = #_name,	\
		.fn = _fn,		\
	}

#define cmd_initcall_prio(_name, _fn, _prio) \
		cmd_initcall_sort(_name, _fn, 9, _prio)
	
#define cmd_initcall(_name, _fn)	\
		cmd_initcall_prio(_name, _fn, 9)

int k_cmd_parse_line(char *line, char *argv[]);
int k_cmd_run_command(int argc, char *argv[]);

void k_cmd_display_foreach(void);
int k_cmd_foreach(int (*callback)(const struct k_cmd *cmd));

#endif /* __CMD_H__ */


