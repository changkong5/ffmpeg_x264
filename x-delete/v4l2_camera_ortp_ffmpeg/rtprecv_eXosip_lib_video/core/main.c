#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dbus/dbus.h>
#include <stdint.h>

#include "initcall.h"
#include "src/shared/mainloop.h"

int fuse_fs_main(int argc, char *argv[]);
int fuse_ll_main(int argc, char *argv[]);
int fuse_cuse_main(int argc, char *argv[]);

static void signal_callback(int signum, void *user_data)
{
	switch (signum) {
	case SIGINT:
	case SIGTERM:
		mainloop_quit();
		break;
	}
}

static __preinit void loop_init(void)
{
	mainloop_init();
	printf("--------1---10---\n");
}

static __exit_0 void loop_quit(void)
{
	mainloop_quit();
	printf("--------1---11---\n");
}

static __finish void finish(void)
{
	usleep(1000);
	printf("--------2------\n");
}

static void *loop_thread(void *arg)
{
	printf("--------3-1-----\n");

	mainloop_sd_notify("STATUS=Starting up");
	mainloop_sd_notify("STATUS=Running");
	mainloop_sd_notify("READY=1");
	
#if defined(FS_LL) || defined(FS_FS) || defined(FS_CUSE)
	printf("--------5-1-----\n");
	mainloop_run();
#else
	printf("--------5-2-----\n");
	mainloop_run_with_signal(signal_callback, NULL);
#endif	
	mainloop_sd_notify("STATUS=Quitting");
	
	printf("--------3-2-----\n");
	
	return NULL;
}

#if defined(FS_LL) || defined(FS_FS) || defined(FS_CUSE)
thread_initcall(loop_thread);
#endif


int main(int argc, char *argv[])
{
	do_fn_initcall();
	do_thread_initcall();
	
#ifdef FS_LL
printf("--------4-1-----\n");
	fuse_ll_main(argc, argv);
#elif FS_FS
printf("--------4-2-----\n");
	fuse_fs_main(argc, argv);
#elif FS_CUSE
printf("--------4-3-----\n");
	fuse_cuse_main(argc, argv);
#else
printf("--------4-4-----\n");
	loop_thread(NULL);
#endif
	
	do_fn_exitcall();

	return 0;
}

