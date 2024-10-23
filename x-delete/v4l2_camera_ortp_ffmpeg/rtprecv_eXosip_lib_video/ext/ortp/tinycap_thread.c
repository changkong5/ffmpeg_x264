#define DEBUG  1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <sys/stat.h>
#include <pthread.h>
#include <semaphore.h>

#include <stdint.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <tinyalsa/pcm.h>

#include "tinyplay_cap.h"

#include "gdbus/gdbus.h"
#include "src/error.h"
#include "src/dbus-common.h"

struct tinycap {
	sem_t sem;
	int pfd[2];
	int run;
	int loop;
	int delay;
	int frame_len;
	int frame_count;
	struct pcm *pcm;
	pthread_t tid;
};

static struct tinycap tinycap[1];

static int tinycap_pipe_open(void)
{
	return pipe(tinycap->pfd);
}

static int tinycap_pipe_write(void *data, int len)
{
	uint8_t *buf = data;

	// pr_debug("%s: <tinycap->pfd[1] = %d, len = %d> \n", __func__, tinycap->pfd[1], len);

	return write(tinycap->pfd[1], buf, len);
}

static int tinycap_pipe_read(void *data, int len)
{
	uint8_t *buf = data;

	return read(tinycap->pfd[0], buf, len);
}

static int tinycap_pipe_close(void)
{
	close(tinycap->pfd[0]);
	close(tinycap->pfd[1]);

	return 0;
}

static int tinycap_pipe_rd_nonblocking(int nonblock)
{
	int flag, ret;

	flag = fcntl(tinycap->pfd[0], F_GETFL);

	if (nonblock) {
		flag |= O_NONBLOCK;
	} else {
		flag &= ~O_NONBLOCK;
	}

	return fcntl(tinycap->pfd[0], F_SETFL, flag);
}

static int tinycap_pipe_wr_nonblocking(int nonblock)
{
	int flag, ret;

	flag = fcntl(tinycap->pfd[1], F_GETFL);

	if (nonblock) {
		flag |= O_NONBLOCK;
	} else {
		flag &= ~O_NONBLOCK;
	}

	return fcntl(tinycap->pfd[1], F_SETFL, flag);
}

static int tinycap_pipe_remain_nbyte(void)
{
	int ret, nbyte;

	ret = ioctl(tinycap->pfd[0], FIONREAD, &nbyte);

	if (ret < 0) {
		nbyte = -1;
	}

	return nbyte;
}

static int tinycap_sem_open(void)
{
	int value = 0;

	return sem_init(&tinycap->sem, 0, value);
}

static int tinycap_sem_wait(void)
{
	return sem_wait(&tinycap->sem);
}

static int tinycap_sem_post(void)
{
	return sem_post(&tinycap->sem);
}

static int tinycap_sem_close(void)
{
	return sem_destroy(&tinycap->sem);
}

#if 0
static void *tinycap_thread(void *arg)
{
	int i, ret, len;
	uint16_t frames[160*160];
    int card = 0, device = 0;
    int frame_len, frame_count;
    int flags = PCM_IN;
    int frame_size, frame_byte;
    
    const struct pcm_config config = {
        .channels = 1,
        .rate = 8000,
        .format = PCM_FORMAT_S16_LE,
        /**
        	frame = channel * bits_per_sample / 8
        	buffer_size = period_size * period_count
        
        	  8000               		  1000
        	-------- = 160 (frame) =  ---------- = 20 (ms)
        	   50                 	       50 
        */
        .period_size = 8000 / 50,
        .period_count = 2,
        .start_threshold = 0,
        .silence_threshold = 0,
        .stop_threshold = 0
    };

    struct pcm *pcm = pcm_open(card, device, flags, &config);
    if (pcm == NULL) {
        pr_err("%s: failed to allocate memory for PCM\n", __func__);
    } else if (!pcm_is_ready(pcm)){
        pcm_close(pcm);
        pr_err("%s: failed to open PCM\n", __func__);
    }

    ret = tinycap_pipe_open();
    pr_debug("%s: <pipe> ret = %d\n", __func__, ret);
    if (ret) {
    	tinycap->run = 0;
    	tinycap->loop = 0;
		pr_err("%s: pipe ERROR (%s)\n", __func__, strerror(errno));
	} else {
		ret = tinycap_sem_open();
		pr_debug("%s: <sem> ret = %d\n", __func__, ret);
		
		tinycap->run = 1;
		tinycap->loop = 1;
	}
	
	frame_byte = pcm_frames_to_bytes(pcm, 1);
	frame_size = config.period_size;

	// frames_per_sec = config.period_size;
	// frames_per_sec = pcm_get_rate(pcm);
	// frame_size = frame_byte * config.period_size;

	pthread_detach(pthread_self());

	while (tinycap->run) {
		tinycap_sem_wait();

		pr_debug("%s: <tinycap_sem_wait> \n", __func__);

		while (tinycap->loop) {
			frame_count = pcm_readi(pcm, frames, frame_size);
			frame_len = pcm_frames_to_bytes(pcm, frame_count);
			
			// frame_count = pcm_bytes_to_frames(pcm, frame_len);
			
			for (i = 0; i < frame_len; i++) {
				// pr_debug("%s: buf[%02d] = %#02x (%c)\n", __func__, i, frames[i], frames[i]);
			}

			if (frame_count != frame_size) {
				pr_debug("%s: <error> frame_count = %d, frame_size = %d\n", __func__, frame_count, frame_size);
			}
			// pr_debug("%s: frame_count = %d, frame_len = %d\n", __func__, frame_count, frame_len);
			
			tinycap_pipe_write(frames, frame_len);

			usleep(20*1000);
			// usleep(10*1000);
			// usleep(20*1000);
		}	
	}
	
	pcm_close(pcm);
	tinycap_sem_close();
	tinycap_pipe_close();

	return NULL;
}
#endif

static void *tinycap_thread(void *arg)
{
	int i, ret, len;
	uint16_t frames[160*160];

    int frame_len, frame_count;
    // int flags = PCM_IN;
    int delay;
    int frame_size, frame_byte;

    ret = tinycap_pipe_open();
    pr_debug("%s: <pipe> ret = %d\n", __func__, ret);
    if (ret) {
    	tinycap->run = 0;
    	tinycap->loop = 0;
		pr_err("%s: pipe ERROR (%s)\n", __func__, strerror(errno));
	} else {
		ret = tinycap_sem_open();
		pr_debug("%s: <sem> ret = %d\n", __func__, ret);
		
		tinycap->run = 1;
		tinycap->loop = 1;
	}

	tinycap->delay = 20 * 1000; // 20 * 10000 = 20 ms
	tinycap->frame_count = 160;	// rate / frame = 160 frame
	
	// frame_byte = pcm_frames_to_bytes(pcm, 1);
	// frame_size = config.period_size;

	// frames_per_sec = config.period_size;
	// frames_per_sec = pcm_get_rate(pcm);
	// frame_size = frame_byte * config.period_size;

	pthread_detach(pthread_self());

	while (tinycap->run) {
		pr_info("%s: ===========================================================================> wait ...\n", __func__);
		tinycap_sem_wait();

		if (!tinycap->pcm) {
			pr_err("%s: <error> tinycap->pcm = NULL\n", __func__);
			continue;
		}

		if (tinycap->frame_count <= 0) {
			pr_err("%s: <error> tinycap->frame_count = %d\n", __func__, tinycap->frame_count);
			continue;
		}

		delay = tinycap->delay / 2;
		frame_size = tinycap->frame_count;
		pr_debug("%s: delay = %d, frame_size = %d\n", __func__, delay, frame_size);
		pr_info("%s: ===========================================================================> start ...\n", __func__);

		while (tinycap->loop) {
			usleep(delay);

			frame_count = pcm_readi(tinycap->pcm, frames, frame_size);
			frame_len = pcm_frames_to_bytes(tinycap->pcm, frame_count);
			
			// frame_count = pcm_bytes_to_frames(pcm, frame_len);
			
			for (i = 0; i < frame_len; i++) {
				// pr_debug("%s: buf[%02d] = %#02x (%c)\n", __func__, i, frames[i], frames[i]);
			}

			if (frame_count != frame_size) {
				pr_debug("%s: <error> frame_count = %d, frame_size = %d\n", __func__, frame_count, frame_size);
			}
			// pr_debug("%s: frame_count = %d, frame_len = %d\n", __func__, frame_count, frame_len);
			
			tinycap_pipe_write(frames, frame_len);
		}

		if (tinycap->pcm) {
			pcm_close(tinycap->pcm);
			tinycap->pcm = NULL;
			pr_debug("%s: <pcm_close> capture close \n", __func__);
		}

		pr_info("%s: ===========================================================================> stop ...\n", __func__);
	}

	tinycap_sem_close();
	tinycap_pipe_close();

	return NULL;
}
thread_initcall_prio(tinycap_thread, NULL, 3);
// thread_initcall(tinycap_thread);

void tinycap_stop(void)
{
	tinycap->loop = 0;
}

void tinycap_exit(void)
{
	tinycap->run = 0;
	tinycap_stop();
}

int tinycap_start(void)
{
	tinycap->loop = 1;
	return tinycap_sem_post();
}

int tinycap_read(void *data, int len)
{
	return tinycap_pipe_read(data, len);
}

int tinycap_set_nonblocking(int nonblock)
{
	return tinycap_pipe_rd_nonblocking(nonblock);
}

int tinycap_set_rw_nonblocking(int nonblock)
{
	int ret = tinycap_pipe_rd_nonblocking(nonblock);

	if (ret < 0) {
		pr_err("%s: <error> ret = %d\n", __func__, ret);
	}

	return tinycap_pipe_wr_nonblocking(nonblock);
}

int tinycap_remain_nbyte(void)
{
	return tinycap_pipe_remain_nbyte();
}

static DBusMessage *start_capture(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
	struct pcm *pcm;
	DBusMessage *message;

	int flags = PCM_IN;
	int card = 0, device = 0;
	int channels = 1, rate = 8000;
	int format = PCM_FORMAT_S16_LE;
	int period_size = 8000 / 50;
	int period_count = 2;
	int start_threshold = 0;
	int silence_threshold = 0;
	int stop_threshold = 0;
	int delay = 20 * 1000;	// unit: us

	
	if (dbus_message_get_args(msg, NULL, 
						DBUS_TYPE_UINT32, &card,
						DBUS_TYPE_UINT32, &device, 
						DBUS_TYPE_UINT32, &channels, 
						DBUS_TYPE_UINT32, &rate, 
						DBUS_TYPE_UINT32, &format,
						DBUS_TYPE_UINT32, &period_size, 
						DBUS_TYPE_UINT32, &period_count, 
						DBUS_TYPE_UINT32, &start_threshold, 
						DBUS_TYPE_UINT32, &silence_threshold,
						DBUS_TYPE_UINT32, &stop_threshold,
						DBUS_TYPE_UINT32, &delay,
						DBUS_TYPE_INVALID) == FALSE) {
		return btd_error_invalid_args(msg);
	}

	pr_info("%s: card     = %d\n", __func__, card);
	pr_info("%s: device   = %d\n", __func__, device);
	pr_info("%s: channels = %d\n", __func__, channels);
	pr_info("%s: rate     = %d\n", __func__, rate);
	pr_info("%s: format   = %d\n", __func__, format);

	pr_info("%s: period_size       = %d\n", __func__, period_size);
	pr_info("%s: period_count      = %d\n", __func__, period_count);
	pr_info("%s: start_threshold   = %d\n", __func__, start_threshold);
	pr_info("%s: silence_threshold = %d\n", __func__, silence_threshold);
	pr_info("%s: stop_threshold    = %d\n", __func__, stop_threshold);

	pr_info("%s: delay             = %d\n", __func__, delay);

	const struct pcm_config config = {
        .channels = channels,
        .rate = rate,
        .format = format,
        /**
        	frame = channel * bits_per_sample / 8
        	buffer_size = period_size * period_count
        
        	  8000               		  1000
        	-------- = 160 (frame) =  ---------- = 20 (ms)
        	   50                 	       50 
        */
        .period_size = period_size,
        .period_count = period_count,
        .start_threshold = start_threshold,
        .silence_threshold = silence_threshold,
        .stop_threshold = stop_threshold
    };

    tinycap->pcm = NULL;
    pcm = pcm_open(card, device, flags, &config);

    if (pcm == NULL) {
        pr_err("%s: failed to allocate memory for PCM\n", __func__);
        return btd_error_invalid_args(msg);
     
    } else if (!pcm_is_ready(pcm)) {
        pcm_close(pcm);
        pr_err("%s: failed to open PCM\n", __func__);
        return btd_error_invalid_args(msg);

    } else {
    	tinycap->pcm = pcm;
    	tinycap->delay = delay;
    	tinycap->frame_count = period_size;

    	// start capture thread
    	tinycap_start();
    	pr_debug("%s: <tinycap_start> capture \n", __func__);
    }


	return dbus_message_new_method_return(msg);
}

static DBusMessage *stop_capture(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
	uint32_t which;
	DBusMessage *message;
	
	if (dbus_message_get_args(msg, NULL, 
						DBUS_TYPE_UINT32, &which,
						DBUS_TYPE_INVALID) == FALSE) {
		return btd_error_invalid_args(msg);
	}
		
	pr_info("%s: which = %d\n", __func__, which);

	// stop capture thread
	tinycap_stop();
	pr_debug("%s: <tinycap_stop> capture \n", __func__);
	
	return dbus_message_new_method_return(msg);
}

static const GDBusMethodTable audio_capture_methods[] = {
	{ GDBUS_METHOD("Start",
			GDBUS_ARGS({ "capture", "uuuuuuuuuuu" }), NULL, start_capture) },
	{ GDBUS_METHOD("Stop",
			GDBUS_ARGS({ "capture", "u" }), NULL, stop_capture) },
	{ }
};

static gboolean property_get_switch(const GDBusPropertyTable *property,
					DBusMessageIter *iter, void *user_data)
{
	dbus_bool_t enable = TRUE; // FALSE
	
	dbus_message_iter_append_basic(iter, DBUS_TYPE_BOOLEAN, &enable);
	
	pr_info("%s: enable = %d\n", __func__, enable);

	return TRUE;
}

static void property_set_switch(const GDBusPropertyTable *property,
				DBusMessageIter *iter,
				GDBusPendingPropertySet id, void *user_data)
{
	dbus_bool_t enable;

	dbus_message_iter_get_basic(iter, &enable);
	
	pr_info("%s: id = %d, enable = %d\n", __func__, id, enable);

	g_dbus_pending_property_success(id);
}

static const GDBusPropertyTable audio_capture_properties[] = {
	{ "Switch1", "b", property_get_switch, property_set_switch },
	{ }
};

static int audio_capture_init(void)
{
	DBusConnection *conn = btd_get_dbus_connection();

	/* Registering interface after querying properties */
	if (!g_dbus_register_interface(conn,
				       "/org/audio",
				       "org.audio.capture", audio_capture_methods,
				       NULL, audio_capture_properties, NULL, NULL)) {
				       
		pr_err("Failed to register " "/org/audio \n");
	}	

	return 0;	
}
fn_initcall(audio_capture_init);


bool audio_capture_start(void)
{
 	DBusMessage *message;
 	char *service_name = (char *)dbus_service_name();
 	DBusConnection *connection = btd_get_dbus_connection();

	int card = 0, device = 0;
	int channels = 1, rate = 8000;
	int format = PCM_FORMAT_S16_LE;
	int period_size = 8000 / 50;
	int period_count = 4;
	int start_threshold = 0;
	int silence_threshold = 0;
	int stop_threshold = 0;
	int delay = 20 * 1000;	// unit: us
 	
 	message = dbus_message_new_method_call(
 					service_name, 
 					"/org/audio",
 					"org.audio.capture", "Start");
 					
 	if (message == NULL) {
		return FALSE;
	}

	dbus_message_append_args(message, 
						DBUS_TYPE_UINT32, &card,
						DBUS_TYPE_UINT32, &device, 
						DBUS_TYPE_UINT32, &channels, 
						DBUS_TYPE_UINT32, &rate, 
						DBUS_TYPE_UINT32, &format,
						DBUS_TYPE_UINT32, &period_size, 
						DBUS_TYPE_UINT32, &period_count, 
						DBUS_TYPE_UINT32, &start_threshold, 
						DBUS_TYPE_UINT32, &silence_threshold,
						DBUS_TYPE_UINT32, &stop_threshold,
						DBUS_TYPE_UINT32, &delay,
						DBUS_TYPE_INVALID);

 	return g_dbus_send_message(connection, message);
} 

bool audio_capture_stop(void)
{
 	DBusMessage *message;
 	char *service_name = (char *)dbus_service_name();
 	DBusConnection *connection = btd_get_dbus_connection();

 	uint32_t value = 8;
 	
 	message = dbus_message_new_method_call(
 					service_name, 
 					"/org/audio",
 					"org.audio.capture", "Stop");
 					
 	if (message == NULL) {
		return FALSE;
	}

	dbus_message_append_args(message, 
								DBUS_TYPE_UINT32, &value, 
								DBUS_TYPE_INVALID);

 	return g_dbus_send_message(connection, message);
} 

