/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * include/linker_lists.h
 *
 * Implementation of linker-generated arrays
 *
 * Copyright (C) 2012 Marek Vasut <marex@denx.de>
 */
#include "initcall.h"

/* extern init function declare */

void do_fn_initcall(void)
{
	struct fn_initcall *init;

	for (init = ll_entry_start(fn_initcall); 
		 init < ll_entry_end(fn_initcall); init++) {
		 
		int ret = init->fn();

		if (ret != 0) {
			pr_err("ret = %d, init->name = %s", ret, init->name);
		}
	}
}

int fn_initcall_display(void)
{
	const struct fn_initcall *init;

	for (init = ll_entry_start(fn_initcall); 
		 init < ll_entry_end(fn_initcall); init++) {
		 
		pr_info("fn->name = %s\n", init->name);
	}

	return 0;
}

int fn_initcall_foreach(int (*callback)(const struct fn_initcall *init))
{
	const struct fn_initcall *init;

	for (init = ll_entry_start(fn_initcall); 
		 init < ll_entry_end(fn_initcall); init++) {
		 
		int ret = callback(init);

		if (ret != 0) {
			pr_err("ret = %d, init->name = %s\n", ret, init->name);
			return ret;
		}
	}

	return 0;
}

/* extern exit function declare */

void do_fn_exitcall(void)
{
	struct fn_exitcall *call;

	for (call = ll_entry_start(fn_exitcall);
		 call < ll_entry_end(fn_exitcall); call++) {

		int ret = call->fn();

		if (ret != 0) {
			pr_err("ret = %d, call->name = %s", ret, call->name);
		}
	}
}

int fn_exitcall_display(void)
{
	const struct fn_exitcall *call;

	for (call = ll_entry_start(fn_exitcall);
		 call < ll_entry_end(fn_exitcall); call++) {

		pr_info("call->name = %s\n", call->name);
	}

	return 0;
}

int fn_exitcall_foreach(int (*callback)(const struct fn_exitcall *call))
{
	const struct fn_exitcall *call;

	for (call = ll_entry_start(fn_exitcall);
		 call < ll_entry_end(fn_exitcall); call++) {

		int ret = callback(call);

		if (ret != 0) {
			pr_err("ret = %d, call->name = %s\n", ret, call->name);
			return ret;
		}
	}

	return 0;
}

/* extern init thread declare */

void do_thread_initcall(void)
{
	struct thread_initcall *thread;

	for (thread = ll_entry_start(thread_initcall); 
		 thread < ll_entry_end(thread_initcall); thread++) {
		 
		int ret = pthread_create(&thread->tid, NULL, thread->entry, thread->arg);

		if (ret != 0) {
			pr_err("pthread_create error thread->name = %s", thread->name);
		} else {
			ret = pthread_detach(thread->tid);

			if (ret != 0) {
				pr_err("pthread_detach error thread->name = %s", thread->name);
			}
		}
	}
}

int thread_initcall_display(void)
{
	struct thread_initcall *thread;

	for (thread = ll_entry_start(thread_initcall); 
		 thread < ll_entry_end(thread_initcall); thread++) {
		 
		pr_info("thread->name = %s\n", thread->name);
	}

	return 0;
}

int thread_initcall_foreach(int (*callback)(const struct thread_initcall *entry))
{
	struct thread_initcall *thread;

	for (thread = ll_entry_start(thread_initcall); 
		 thread < ll_entry_end(thread_initcall); thread++) {
		 
		int ret = callback(thread);

		if (ret != 0) {
			pr_err("ret = %d, thread->name = %s\n", ret, thread->name);
			return ret;
		}
	}

	return 0;
}

