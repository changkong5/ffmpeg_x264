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

struct emulplay {
	sem_t sem;
	int pfd[2];
	int run;
	int loop;
	int frame_len;
	int frame_count;
};

static struct emulplay emulplay[1];

static int emulplay_pipe_open(void)
{
	return pipe(emulplay->pfd);
}

static int emulplay_pipe_write(void *data, int len)
{
	uint8_t *buf = data;

	return write(emulplay->pfd[1], buf, len);
}

static int emulplay_pipe_read(void *data, int len)
{
	uint8_t *buf = data;

	return read(emulplay->pfd[0], buf, len);
}

static int emulplay_pipe_close(void)
{
	close(emulplay->pfd[0]);
	close(emulplay->pfd[1]);

	return 0;
}

static int emulplay_pipe_rd_nonblocking(int nonblock)
{
	int flag, ret;

	flag = fcntl(emulplay->pfd[0], F_GETFL);

	if (nonblock) {
		flag |= O_NONBLOCK;
	} else {
		flag &= ~O_NONBLOCK;
	}

	return fcntl(emulplay->pfd[0], F_SETFL, flag);
}

static int emulplay_pipe_wr_nonblocking(int nonblock)
{
	int flag, ret;

	flag = fcntl(emulplay->pfd[1], F_GETFL);

	if (nonblock) {
		flag |= O_NONBLOCK;
	} else {
		flag &= ~O_NONBLOCK;
	}

	return fcntl(emulplay->pfd[1], F_SETFL, flag);
}

static int emulplay_pipe_remain_nbyte(void)
{
	int ret, nbyte;

	ret = ioctl(emulplay->pfd[0], FIONREAD, &nbyte);

	if (ret < 0) {
		nbyte = -1;
	}

	return nbyte;
}

static int emulplay_sem_open(void)
{
	int value = 0;

	return sem_init(&emulplay->sem, 0, value);
}

static int emulplay_sem_wait(void)
{
	return sem_wait(&emulplay->sem);
}

static int emulplay_sem_post(void)
{
	return sem_post(&emulplay->sem);
}

static int emulplay_sem_close(void)
{
	return sem_destroy(&emulplay->sem);
}

static void *emulplay_thread(void *arg)
{
	int i, ret, len;
	int16_t frames[160];
    int card = 0, device = 0;
    int frame_len, frame_count;
    int frame_size, frame_byte;

	ret = emulplay_pipe_open();
	pr_debug("%s: <pipe> ret = %d\n", __func__, ret);
    if (ret) {
    	emulplay->run = 0;
    	emulplay->loop = 0;
		pr_err("%s: pipe ERROR (%s)\n", __func__, strerror(errno));
	} else {
		ret = emulplay_sem_open();
		pr_debug("%s: <sem> ret = %d\n", __func__, ret);

		emulplay->run = 1;
		emulplay->loop = 1;
	}

	emulplay->frame_count = 160;	// rate / frame = 160 frame

	pthread_detach(pthread_self());

	while (emulplay->run) {
		pr_info("%s: =====================================================> wait ...\n", __func__);
		emulplay_sem_wait();

		pr_debug("%s: <emulplay_sem_wait> play start \n", __func__);
		pr_info("%s: =====================================================> start ...\n", __func__);

		if (emulplay->frame_count <= 0) {
			pr_err("%s: <error> emulplay->frame_count = %d\n", __func__, emulplay->frame_count);
			continue;
		}
		frame_size = emulplay->frame_count * 2;
		pr_debug("%s: emulplay->frame_count = %d, frame_size = %d\n", __func__, emulplay->frame_count, frame_size);

		while (emulplay->loop) {
			frame_len = emulplay_pipe_read(frames, frame_size);
			// frame_size = read(emulplay->pfd[0], frames, emulplay->frame_len);
			
			// pr_info("%s run ... \n", __func__);
			for (i = 0; i < frame_len; i++) {
				//pr_debug("%s: buf[%02d] = %#02x (%c)\n", __func__, i, frames[i], frames[i]);
			}

			frame_count = frame_len / 2;
			// pr_debug("%s: frame_count = %d, frame_len = %d\n", __func__, frame_count, frame_len);
			
			for (i = 0; i < frame_count; i++) {
				// pr_debug("%s: frames[%d] = %d\n", __func__, i, frames[i]);
			}
		}

		pr_debug("%s: <emulplay_sem_wait> play finish \n", __func__);
		pr_info("%s: =====================================================> stop ...\n", __func__);
	}
	
	emulplay_sem_close();
	emulplay_pipe_close();

	return NULL;
}
thread_initcall_prio(emulplay_thread, NULL, 3);
// thread_initcall(emulplay_thread);

void emulplay_stop(void)
{
	int i, len = 160;
	int16_t frames[160];
	emulplay->loop = 0;

	for (i = 0; i < len; i++) {
		frames[i] = 0xd5d5;
	}

	// write data to exit pipe wait
	emulplay_pipe_write(frames, sizeof(frames));
}

void emulplay_exit(void)
{
	emulplay->run = 0;
	emulplay_stop();
}

int emulplay_start(void)
{
	emulplay->loop = 1;
	return emulplay_sem_post();
}

int emulplay_write(void *data, int len)
{
	return emulplay_pipe_write(data, len);
}

int emulplay_set_nonblocking(int nonblock)
{
	return emulplay_pipe_rd_nonblocking(nonblock);
}

int emulplay_set_rw_nonblocking(int nonblock)
{
	int ret = emulplay_pipe_rd_nonblocking(nonblock);

	if (ret < 0) {
		pr_err("%s: <error> ret = %d\n", __func__, ret);
	}

	return emulplay_pipe_wr_nonblocking(nonblock);
}

int emulplay_remain_nbyte(void)
{
	return emulplay_pipe_remain_nbyte();
}


static DBusMessage *method_emulate_start_play(DBusConnection *conn,
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

	emulplay->frame_count = period_size;

    // start play thread
    emulplay_start();
    pr_debug("%s: <emulplay_start> play \n", __func__);

	return dbus_message_new_method_return(msg);
}

static DBusMessage *method_emulate_stop_play(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
	uint32_t which;
	DBusMessage *message;
	
	if (dbus_message_get_args(msg, NULL, DBUS_TYPE_UINT32, &which,
						DBUS_TYPE_INVALID) == FALSE)
		return btd_error_invalid_args(msg);
		
	pr_info("%s: which = %d\n", __func__, which);

	// stop play thread
	emulplay_stop();
	pr_debug("%s: <emulplay_stop> play \n", __func__);

	return dbus_message_new_method_return(msg);
}

static const GDBusMethodTable emulate_audio_play_methods[] = {
	{ GDBUS_METHOD("Start",
			GDBUS_ARGS({ "play", "uuuuuuuuuu" }), NULL, method_emulate_start_play) },
	{ GDBUS_METHOD("Stop",
			GDBUS_ARGS({ "play", "u" }), NULL, method_emulate_stop_play) },
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

static const GDBusPropertyTable emulate_audio_play_properties[] = {
	{ "Switch1", "b", property_get_switch, property_set_switch },
	{ }
};

static int emulate_audio_play_init(void)
{
	DBusConnection *conn = btd_get_dbus_connection();

	/* Registering interface after querying properties */
	if (!g_dbus_register_interface(conn,
				       "/org/audio",
				       "org.audio.emulate.play", emulate_audio_play_methods,
				       NULL, emulate_audio_play_properties, NULL, NULL)) {
				       
		pr_err("Failed to register " "/org/audio \n");
	}	

	return 0;	
}
fn_initcall(emulate_audio_play_init);


bool emulate_audio_play_start(void)
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
 					"org.audio.emulate.play", "Start");
 					
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

bool emulate_audio_play_stop(void)
{
 	DBusMessage *message;
 	char *service_name = (char *)dbus_service_name();
 	DBusConnection *connection = btd_get_dbus_connection();

 	uint32_t value = 8;
 	
 	message = dbus_message_new_method_call(
 					service_name, 
 					"/org/audio",
 					"org.audio.emulate.play", "Stop");
 					
 	if (message == NULL) {
		return FALSE;
	}

	dbus_message_append_args(message, 
								DBUS_TYPE_UINT32, &value, 
								DBUS_TYPE_INVALID);

 	return g_dbus_send_message(connection, message);
}


