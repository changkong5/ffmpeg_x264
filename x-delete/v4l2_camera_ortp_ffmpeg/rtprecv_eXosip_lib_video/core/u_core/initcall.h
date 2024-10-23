/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * include/linker_lists.h
 *
 * Implementation of linker-generated arrays
 *
 * Copyright (C) 2012 Marek Vasut <marex@denx.de>
 */

#ifndef __INITCALL_H__
#define __INITCALL_H__

#include <stdint.h>
#include <stdio.h> 
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "linker_lists.h"

//#define DEBUG  1

#ifdef DEBUG
#define pr_debug(...) 	printf(__VA_ARGS__)
#else
#define pr_debug(...) 
#endif

#define pr_info(...) 	printf(__VA_ARGS__)
#define pr_err(...) 	printf(__VA_ARGS__)


/* function initcall section define and declare */
struct fn_initcall {
	const char *name;
	int (*fn)(void);
};

/* static initcall function macro declare */
#define fn_initcall_sort(_fn, _sort, _prio) \
	static ll_entry_declare(fn_initcall, _fn, _sort, _prio) = { \
		.name = #_fn,	\
		.fn = _fn		\
	}

#define fn_initcall_prio(_fn, _prio) \
		fn_initcall_sort(_fn, 9, _prio)
		
#define fn_initcall(_fn) fn_initcall_prio(_fn, 9)

/* extern initcall function macro declare */
#define ext_fn_initcall_sort(_fn, _sort, _prio) \
	ll_entry_declare(fn_initcall, _fn, _sort, _prio) = { \
		.name = #_fn,	\
		.fn = _fn		\
	}

#define ext_fn_initcall_prio(_fn, _prio) \
		ext_fn_initcall_sort(_fn, 9, _prio)
		
#define ext_fn_initcall(_fn) ext_fn_initcall_prio(_fn, 9)

/* function exitcall section define and declare */
struct fn_exitcall {
	const char *name;
	int (*fn)(void);
};

/* static exitcall function macro declare */
#define fn_exitcall_sort(_fn, _sort, _prio) \
	static ll_entry_declare(fn_exitcall, _fn, _sort, _prio) = { \
		.name = #_fn,	\
		.fn = _fn		\
	}

#define fn_exitcall_prio(_fn, _prio) \
		fn_exitcall_sort(_fn, 9, _prio)

#define fn_exitcall(_fn) fn_exitcall_prio(_fn, 9)

/* extern exitcall function macro declare */
#define ext_fn_exitcall_sort(_fn, _sort, _prio) \
	ll_entry_declare(fn_exitcall, _fn, _sort, _prio) = { \
		.name = #_fn,	\
		.fn = _fn		\
	}

#define ext_fn_exitcall_prio(_fn, _prio) \
		ext_fn_exitcall_sort(_fn, 9, _prio)

#define ext_fn_exitcall(_fn) ext_fn_exitcall_prio(_fn, 9)

/* thread section define and declare */
struct thread_initcall {
	const char *name;
	void *arg;
	pthread_t tid;
	void *(*entry)(void *arg);
};

/* static initcall thread macro declare */
#define thread_initcall_sort(_thread, _arg, _sort, _prio) \
	static ll_entry_declare(thread_initcall, _thread, _sort, _prio) = { \
		.name = #_thread,	\
		.tid = 0,			\
		.arg = _arg,		\
		.entry = _thread	\
	}

#define thread_initcall_prio(_thread, _arg, _prio) \
		thread_initcall_sort(_thread, _arg, 9, _prio)
		
#define thread_initcall_arg(_thread, _arg) \
		thread_initcall_prio(_thread, _arg, 9)
	
#define thread_initcall(_thread)	\
		thread_initcall_arg(_thread, NULL)
		
/* extern initcall thread macro declare */		
#define ext_thread_initcall_sort(_thread, _arg, _sort, _prio) \
	ll_entry_declare(thread_initcall, _thread, _sort, _prio) = { \
		.name = #_thread,	\
		.tid = 0,			\
		.arg = _arg,		\
		.entry = _thread	\
	}

#define ext_thread_initcall_prio(_thread, _arg, _prio) \
		ext_thread_initcall_sort(_thread, _arg, 9, _prio)
		
#define ext_thread_initcall_arg(_thread, _arg) \
		ext_thread_initcall_prio(_thread, _arg, 9)
	
#define ext_thread_initcall(_thread)	\
		ext_thread_initcall_arg(_thread, NULL)

/* extern init function declare */
void do_fn_initcall(void);
int fn_initcall_display(void);
int fn_initcall_foreach(int (*callback)(const struct fn_initcall *init));

/* extern exit function declare */
void do_fn_exitcall(void);
int fn_exitcall_display(void);
int fn_exitcall_foreach(int (*callback)(const struct fn_exitcall *call));

/* extern init thread declare */
void do_thread_initcall(void);
int thread_initcall_display(void);
int thread_initcall_foreach(int (*callback)(const struct thread_initcall *entry));

#endif	/* __INITCALL_H__ */
















