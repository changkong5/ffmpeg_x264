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

#include "emulplay_cap.h"

#include "gdbus/gdbus.h"
#include "src/error.h"
#include "src/dbus-common.h"

struct emulcap {
	sem_t sem;
	int pfd[2];
	int run;
	int loop;
	int delay;
	int frame_len;
	int frame_count;
};

static struct emulcap emulcap[1];

static int emulcap_pipe_open(void)
{
	return pipe(emulcap->pfd);
}

static int emulcap_pipe_write(void *data, int len)
{
	uint8_t *buf = data;

	// pr_debug("%s: <emulcap->pfd[1] = %d, len = %d> \n", __func__, emulcap->pfd[1], len);

	return write(emulcap->pfd[1], buf, len);
}

static int emulcap_pipe_read(void *data, int len)
{
	uint8_t *buf = data;

	return read(emulcap->pfd[0], buf, len);
}

static int emulcap_pipe_close(void)
{
	close(emulcap->pfd[0]);
	close(emulcap->pfd[1]);

	return 0;
}

static int emulcap_pipe_rd_nonblocking(int nonblock)
{
	int flag, ret;

	flag = fcntl(emulcap->pfd[0], F_GETFL);

	if (nonblock) {
		flag |= O_NONBLOCK;
	} else {
		flag &= ~O_NONBLOCK;
	}

	return fcntl(emulcap->pfd[0], F_SETFL, flag);
}

static int emulcap_pipe_wr_nonblocking(int nonblock)
{
	int flag, ret;

	flag = fcntl(emulcap->pfd[1], F_GETFL);

	if (nonblock) {
		flag |= O_NONBLOCK;
	} else {
		flag &= ~O_NONBLOCK;
	}

	return fcntl(emulcap->pfd[1], F_SETFL, flag);
}

static int emulcap_pipe_remain_nbyte(void)
{
	int ret, nbyte;

	ret = ioctl(emulcap->pfd[0], FIONREAD, &nbyte);

	if (ret < 0) {
		nbyte = -1;
	}

	return nbyte;
}

static int emulcap_sem_open(void)
{
	int value = 0;

	return sem_init(&emulcap->sem, 0, value);
}

static int emulcap_sem_wait(void)
{
	return sem_wait(&emulcap->sem);
}

static int emulcap_sem_post(void)
{
	return sem_post(&emulcap->sem);
}

static int emulcap_sem_close(void)
{
	return sem_destroy(&emulcap->sem);
}

static void *emulcap_thread(void *arg)
{
	int i, ret, len;
	uint16_t frames[160*160];

    int frame_len, frame_count;
    int delay;
    int frame_size, frame_byte;

    ret = emulcap_pipe_open();
    pr_debug("%s: <pipe> ret = %d\n", __func__, ret);
    if (ret) {
    	emulcap->run = 0;
    	emulcap->loop = 0;
		pr_err("%s: pipe ERROR (%s)\n", __func__, strerror(errno));
	} else {
		ret = emulcap_sem_open();
		pr_debug("%s: <sem> ret = %d\n", __func__, ret);
		
		emulcap->run = 1;
		emulcap->loop = 1;
	}

	emulcap->delay = 20 * 1000; // 20 * 10000 = 20 ms
	emulcap->frame_count = 160;	// rate / frame = 160 frame

	pthread_detach(pthread_self());

	while (emulcap->run) {
		pr_info("%s: =====================================================> wait ...\n", __func__);
		emulcap_sem_wait();

		pr_debug("%s: <emulcap_sem_wait> capture start \n", __func__);
		pr_info("%s: =====================================================> start ...\n", __func__);

		if (emulcap->frame_count <= 0) {
			pr_err("%s: <error> emulcap->frame_count = %d\n", __func__, emulcap->frame_count);
			continue;
		}

		delay = emulcap->delay;
		frame_size = emulcap->frame_count;
		pr_debug("%s: delay = %d, frame_size = %d\n", __func__, delay, frame_size);

		while (emulcap->loop) {
			usleep(delay);

			frame_count = 160;
			frame_len = frame_count * 2;
			for (i = 0; i < frame_count; i++) {
				frames[i] = 0x4545;	// 0x45 = 'E'
			}
			
			for (i = 0; i < frame_len; i++) {
				// pr_debug("%s: buf[%02d] = %#02x (%c)\n", __func__, i, frames[i], frames[i]);
			}

			if (frame_count != frame_size) {
				pr_debug("%s: <error> frame_count = %d, frame_size = %d\n", __func__, frame_count, frame_size);
			}
			// pr_debug("%s: frame_count = %d, frame_len = %d\n", __func__, frame_count, frame_len);
			
			emulcap_pipe_write(frames, frame_len);
		}

		pr_debug("%s: <emulcap_sem_wait> capture finish \n", __func__);
		pr_info("%s: =====================================================> stop ...\n", __func__);
	}

	emulcap_sem_close();
	emulcap_pipe_close();

	return NULL;
}
thread_initcall_prio(emulcap_thread, NULL, 3);
// thread_initcall(emulcap_thread);

void emulcap_stop(void)
{
	emulcap->loop = 0;
}

void emulcap_exit(void)
{
	emulcap->run = 0;
	emulcap_stop();
}

int emulcap_start(void)
{
	emulcap->loop = 1;
	return emulcap_sem_post();
}

int emulcap_read(void *data, int len)
{
	return emulcap_pipe_read(data, len);
}

int emulcap_set_nonblocking(int nonblock)
{
	return emulcap_pipe_rd_nonblocking(nonblock);
}

int emulcap_set_rw_nonblocking(int nonblock)
{
	int ret = emulcap_pipe_rd_nonblocking(nonblock);

	if (ret < 0) {
		pr_err("%s: <error> ret = %d\n", __func__, ret);
	}

	return emulcap_pipe_wr_nonblocking(nonblock);
}

int emulcap_remain_nbyte(void)
{
	return emulcap_pipe_remain_nbyte();
}

static DBusMessage *method_emulate_start_capture(DBusConnection *conn,
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

	emulcap->delay = delay;
    emulcap->frame_count = period_size;

    // start capture thread
    emulcap_start();
    pr_debug("%s: <emulcap_start> capture \n", __func__);


	return dbus_message_new_method_return(msg);
}

static DBusMessage *method_emulate_stop_capture(DBusConnection *conn,
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
	emulcap_stop();
	pr_debug("%s: <emulcap_stop> capture \n", __func__);
	
	return dbus_message_new_method_return(msg);
}

static const GDBusMethodTable emulate_audio_capture_methods[] = {
	{ GDBUS_METHOD("Start",
			GDBUS_ARGS({ "capture", "uuuuuuuuuuu" }), NULL, method_emulate_start_capture) },
	{ GDBUS_METHOD("Stop",
			GDBUS_ARGS({ "capture", "u" }), NULL, method_emulate_stop_capture) },
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

static const GDBusPropertyTable emulate_audio_capture_properties[] = {
	{ "Switch1", "b", property_get_switch, property_set_switch },
	{ }
};

static int emulate_audio_capture_init(void)
{
	DBusConnection *conn = btd_get_dbus_connection();

	/* Registering interface after querying properties */
	if (!g_dbus_register_interface(conn,
				       "/org/audio",
				       "org.audio.emulate.capture", emulate_audio_capture_methods,
				       NULL, emulate_audio_capture_properties, NULL, NULL)) {
				       
		pr_err("Failed to register " "/org/audio \n");
	}	

	return 0;	
}
fn_initcall(emulate_audio_capture_init);


bool emulate_audio_capture_start(void)
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
 					"org.audio.emulate.capture", "Start");
 					
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

bool emulate_audio_capture_stop(void)
{
 	DBusMessage *message;
 	char *service_name = (char *)dbus_service_name();
 	DBusConnection *connection = btd_get_dbus_connection();

 	uint32_t value = 8;
 	
 	message = dbus_message_new_method_call(
 					service_name, 
 					"/org/audio",
 					"org.audio.emulate.capture", "Stop");
 					
 	if (message == NULL) {
		return FALSE;
	}

	dbus_message_append_args(message, 
								DBUS_TYPE_UINT32, &value, 
								DBUS_TYPE_INVALID);

 	return g_dbus_send_message(connection, message);
} 

