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

struct tinyplay {
	sem_t sem;
	int pfd[2];
	int run;
	int loop;
	int frame_len;
	int frame_count;
	struct pcm *pcm;
	pthread_t tid;
};

static struct tinyplay tinyplay[1];

static int tinyplay_pipe_open(void)
{
	return pipe(tinyplay->pfd);
}

static int tinyplay_pipe_write(void *data, int len)
{
	uint8_t *buf = data;

	return write(tinyplay->pfd[1], buf, len);
}

static int tinyplay_pipe_read(void *data, int len)
{
	uint8_t *buf = data;

	return read(tinyplay->pfd[0], buf, len);
}

static int tinyplay_pipe_close(void)
{
	close(tinyplay->pfd[0]);
	close(tinyplay->pfd[1]);

	return 0;
}

static int tinyplay_pipe_rd_nonblocking(int nonblock)
{
	int flag, ret;

	flag = fcntl(tinyplay->pfd[0], F_GETFL);

	if (nonblock) {
		flag |= O_NONBLOCK;
	} else {
		flag &= ~O_NONBLOCK;
	}

	return fcntl(tinyplay->pfd[0], F_SETFL, flag);
}

static int tinyplay_pipe_wr_nonblocking(int nonblock)
{
	int flag, ret;

	flag = fcntl(tinyplay->pfd[1], F_GETFL);

	if (nonblock) {
		flag |= O_NONBLOCK;
	} else {
		flag &= ~O_NONBLOCK;
	}

	return fcntl(tinyplay->pfd[1], F_SETFL, flag);
}

static int tinyplay_pipe_remain_nbyte(void)
{
	int ret, nbyte;

	ret = ioctl(tinyplay->pfd[0], FIONREAD, &nbyte);

	if (ret < 0) {
		nbyte = -1;
	}

	return nbyte;
}

static int tinyplay_sem_open(void)
{
	int value = 0;

	return sem_init(&tinyplay->sem, 0, value);
}

static int tinyplay_sem_wait(void)
{
	return sem_wait(&tinyplay->sem);
}

static int tinyplay_sem_post(void)
{
	return sem_post(&tinyplay->sem);
}

static int tinyplay_sem_close(void)
{
	return sem_destroy(&tinyplay->sem);
}

#if 0
static void *tinyplay_thread(void *arg)
{
	int i, ret, len;
	uint8_t frames[320];
    int card = 0, device = 0;
    int frame_len, frame_count;
    int flags = PCM_OUT;
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

	ret = tinyplay_pipe_open();
	pr_debug("%s: <pipe> ret = %d\n", __func__, ret);
    if (ret) {
    	tinyplay->run = 0;
    	tinyplay->loop = 0;
		pr_err("%s: pipe ERROR (%s)\n", __func__, strerror(errno));
	} else {
		ret = tinyplay_sem_open();
		pr_debug("%s: <sem> ret = %d\n", __func__, ret);

		tinyplay->run = 1;
		tinyplay->loop = 1;
	}

    frame_byte = pcm_frames_to_bytes(pcm, 1);
	frame_size = frame_byte * config.period_size;

	pthread_detach(pthread_self());

	while (tinyplay->run) {
		tinyplay_sem_wait();

		pr_debug("%s: <tinyplay_sem_wait> \n", __func__);

		while (tinyplay->loop) {
			frame_len = tinyplay_pipe_read(frames, frame_size);
			// frame_size = read(tinyplay->pfd[0], frames, tinyplay->frame_len);
			
			// pr_info("%s run ... \n", __func__);
			for (i = 0; i < frame_len; i++) {
				//pr_debug("%s: buf[%02d] = %#02x (%c)\n", __func__, i, frames[i], frames[i]);
			}
			frame_count = pcm_bytes_to_frames(pcm, frame_len);

			// pr_debug("%s: frame_count = %d, frame_len = %d\n", __func__, frame_count, frame_len);
			
			ret = pcm_writei(pcm, frames, frame_count);
			if (ret < 0) {
				//pr_debug("%s: error: %s\n", __func__, pcm_get_error(pcm));
			}

			if (frame_count != ret) {
				pr_debug("%s: <error> frame_count = %d, ret = %d\n", __func__, frame_count, ret);
			}
			// usleep(10*1000);
		}
	}
	
	pcm_close(pcm);
	tinyplay_sem_close();
	tinyplay_pipe_close();

	return NULL;
}
#endif

static void *tinyplay_thread(void *arg)
{
	int i, ret, len;
	uint8_t frames[320];
    int card = 0, device = 0;
    int frame_len, frame_count;
    int frame_size, frame_byte;

	ret = tinyplay_pipe_open();
	pr_debug("%s: <pipe> ret = %d\n", __func__, ret);
    if (ret) {
    	tinyplay->run = 0;
    	tinyplay->loop = 0;
		pr_err("%s: pipe ERROR (%s)\n", __func__, strerror(errno));
	} else {
		ret = tinyplay_sem_open();
		pr_debug("%s: <sem> ret = %d\n", __func__, ret);

		tinyplay->run = 1;
		tinyplay->loop = 1;
	}

	tinyplay->frame_count = 160;	// rate / frame = 160 frame

    // frame_byte = pcm_frames_to_bytes(pcm, 1);
	// frame_size = frame_byte * config.period_size;

	pthread_detach(pthread_self());

	while (tinyplay->run) {
		pr_info("%s: ===========================================================================> wait ...\n", __func__);
		tinyplay_sem_wait();

		pr_debug("%s: <tinyplay_sem_wait> play start \n", __func__);

		if (!tinyplay->pcm) {
			pr_err("%s: <error> tinyplay->pcm = NULL\n", __func__);
			continue;
		}

		if (tinyplay->frame_count <= 0) {
			pr_err("%s: <error> tinyplay->frame_count = %d\n", __func__, tinyplay->frame_count);
			continue;
		}
		frame_size = tinyplay->frame_count * 2;
		pr_debug("%s: tinyplay->frame_count = %d, frame_size = %d\n", __func__, tinyplay->frame_count, frame_size);
		pr_info("%s: ===========================================================================> start ...\n", __func__);

		while (tinyplay->loop) {
			frame_len = tinyplay_pipe_read(frames, frame_size);
			
			// pr_info("%s run ... \n", __func__);
			for (i = 0; i < frame_len; i++) {
				//pr_debug("%s: buf[%02d] = %#02x (%c)\n", __func__, i, frames[i], frames[i]);
			}

			frame_count = pcm_bytes_to_frames(tinyplay->pcm, frame_len);

			// pr_debug("%s: frame_count = %d, frame_len = %d\n", __func__, frame_count, frame_len);
			
			ret = pcm_writei(tinyplay->pcm, frames, frame_count);
			if (ret < 0) {
				//pr_debug("%s: error: %s\n", __func__, pcm_get_error(pcm));
			}

			if (frame_count != ret) {
				pr_debug("%s: <error> frame_count = %d, ret = %d\n", __func__, frame_count, ret);
			}
		}

		if (tinyplay->pcm) {
			pcm_close(tinyplay->pcm);
			tinyplay->pcm = NULL;
			pr_debug("%s: <tinyplay_sem_wait> play close \n", __func__);
		}

		pr_info("%s: ===========================================================================> stop ...\n", __func__);
	}
	
	tinyplay_sem_close();
	tinyplay_pipe_close();

	return NULL;
}
thread_initcall_prio(tinyplay_thread, NULL, 3);
// thread_initcall(tinyplay_thread);

void tinyplay_stop(void)
{
	int i, len = 160;
	int16_t frames[160];
	tinyplay->loop = 0;

	for (i = 0; i < len; i++) {
		frames[i] = 0xd5d5;
	}

	// write data to exit pipe wait
	tinyplay_pipe_write(frames, sizeof(frames));
}

void tinyplay_exit(void)
{
	tinyplay->run = 0;
	tinyplay_stop();
}

int tinyplay_start(void)
{
	tinyplay->loop = 1;
	return tinyplay_sem_post();
}

int tinyplay_write(void *data, int len)
{
	return tinyplay_pipe_write(data, len);
}

int tinyplay_set_nonblocking(int nonblock)
{
	return tinyplay_pipe_rd_nonblocking(nonblock);
}

int tinyplay_set_rw_nonblocking(int nonblock)
{
	int ret = tinyplay_pipe_rd_nonblocking(nonblock);

	if (ret < 0) {
		pr_err("%s: <error> ret = %d\n", __func__, ret);
	}

	return tinyplay_pipe_wr_nonblocking(nonblock);
}

int tinyplay_remain_nbyte(void)
{
	return tinyplay_pipe_remain_nbyte();
}


static DBusMessage *start_play(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
	struct pcm *pcm;
	DBusMessage *message;

	int flags = PCM_OUT;
	int card = 0, device = 0;
	int channels = 1, rate = 8000;
	int format = PCM_FORMAT_S16_LE;
	int period_size = 8000 / 50;
	int period_count = 2;
	int start_threshold = 0;
	int silence_threshold = 0;
	int stop_threshold = 0;

	
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

    tinyplay->pcm = NULL;
    pcm = pcm_open(card, device, flags, &config);

    if (pcm == NULL) {
        pr_err("%s: failed to allocate memory for PCM\n", __func__);
        return btd_error_invalid_args(msg);
    } else if (!pcm_is_ready(pcm)) {
        pcm_close(pcm);
        pr_err("%s: failed to open PCM\n", __func__);
        return btd_error_invalid_args(msg);
    } else {
    	tinyplay->pcm = pcm;
    	tinyplay->frame_count = period_size;

    	// start play thread
    	tinyplay_start();
    	pr_debug("%s: <tinyplay_start> play \n", __func__);
    }


	return dbus_message_new_method_return(msg);
}

static DBusMessage *stop_play(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
	uint32_t which;
	DBusMessage *message;
	
	if (dbus_message_get_args(msg, NULL, DBUS_TYPE_UINT32, &which,
						DBUS_TYPE_INVALID) == FALSE)
		return btd_error_invalid_args(msg);
		
	pr_info("%s: which = %d\n", __func__, which);

	// stop play thread
	tinyplay_stop();
	pr_debug("%s: <tinyplay_stop> play \n", __func__);

	return dbus_message_new_method_return(msg);
}

static const GDBusMethodTable audio_play_methods[] = {
	{ GDBUS_METHOD("Start",
			GDBUS_ARGS({ "play", "uuuuuuuuuu" }), NULL, start_play) },
	{ GDBUS_METHOD("Stop",
			GDBUS_ARGS({ "play", "u" }), NULL, stop_play) },
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

static const GDBusPropertyTable audio_play_properties[] = {
	{ "Switch1", "b", property_get_switch, property_set_switch },
	{ }
};

static int audio_play_init(void)
{
	DBusConnection *conn = btd_get_dbus_connection();

	/* Registering interface after querying properties */
	if (!g_dbus_register_interface(conn,
				       "/org/audio",
				       "org.audio.play", audio_play_methods,
				       NULL, audio_play_properties, NULL, NULL)) {
				       
		pr_err("Failed to register " "/org/audio \n");
	}	

	return 0;	
}
fn_initcall(audio_play_init);


bool audio_play_start(void)
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
 	
 	message = dbus_message_new_method_call(
 					service_name, 
 					"/org/audio",
 					"org.audio.play", "Start");
 					
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
						DBUS_TYPE_INVALID);

 	return g_dbus_send_message(connection, message);
} 

bool audio_play_stop(void)
{
 	DBusMessage *message;
 	char *service_name = (char *)dbus_service_name();
 	DBusConnection *connection = btd_get_dbus_connection();

 	uint32_t value = 8;
 	
 	message = dbus_message_new_method_call(
 					service_name, 
 					"/org/audio",
 					"org.audio.play", "Stop");
 					
 	if (message == NULL) {
		return FALSE;
	}

	dbus_message_append_args(message, 
								DBUS_TYPE_UINT32, &value, 
								DBUS_TYPE_INVALID);

 	return g_dbus_send_message(connection, message);
}


