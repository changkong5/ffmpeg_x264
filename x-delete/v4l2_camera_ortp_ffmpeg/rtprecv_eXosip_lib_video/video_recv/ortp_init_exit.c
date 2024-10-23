#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <linux/fb.h>
#include <time.h>
#include <linux/rtc.h>
#include <sys/time.h>

#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include <bctoolbox/vfs.h>
#include <ortp/ortp.h>
#include <signal.h>
#include <stdlib.h>

#include "initcall.h"
#include "gdbus/gdbus.h"


static int ortp_init_init(void)
{

	ortp_init();
	ortp_scheduler_init();
	// ortp_set_log_level_mask(NULL, ORTP_DEBUG | ORTP_MESSAGE | ORTP_WARNING | ORTP_ERROR);
	
	pr_info("%s: ...\n", __func__);
    
    return 0;
}
fn_initcall(ortp_init_init);

static int ortp_exit_exit(void)
{
	ortp_exit();
	ortp_global_stats_display();
	
	pr_info("%s: ...\n", __func__);
	
    return 0;
}
fn_exitcall(ortp_exit_exit);


