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
#include <tinyalsa/pcm.h>

#include "tinyplay_cap.h"

#include "info.h"
#include "initcall.h"

#include "gdbus/gdbus.h"
#include "src/error.h"
#include "src/dbus-common.h"

#define ID_RIFF 0x46464952
#define ID_WAVE 0x45564157
#define ID_FMT  0x20746d66
#define ID_DATA 0x61746164

struct wav_header {
    uint32_t riff_id;
    uint32_t riff_sz;
    uint32_t riff_fmt;
    uint32_t fmt_id;
    uint32_t fmt_sz;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    uint32_t data_id;
    uint32_t data_sz;
};

struct riff_wave_header {
    uint32_t riff_id;
    uint32_t riff_sz;
    uint32_t wave_id;
};

struct chunk_header {
    uint32_t id;
    uint32_t sz;
};

struct chunk_fmt {
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
};

struct play_wav {
	sem_t sem;
	int run;
	int loop;
	int framerate;
	char file[128];
};

static struct play_wav play_wav[1];

static int play_wav_sem_open(void)
{
	int value = 0;

	return sem_init(&play_wav->sem, 0, value);
}

static int play_wav_sem_wait(void)
{
	return sem_wait(&play_wav->sem);
}

static int play_wav_sem_post(void)
{
	return sem_post(&play_wav->sem);
}

static int play_wav_sem_close(void)
{
	return sem_destroy(&play_wav->sem);
}

int tinyplay_write(void *data, int len);
bool audio_play_start(void);
bool audio_play_stop(void);

static void *play_wav_thread(void *arg)
{
    FILE *file;
    struct riff_wave_header riff_wave_header;
    struct chunk_header chunk_header;
    struct chunk_fmt chunk_fmt;
    uint8_t frames[1024];
    int frame_len, frame_count, data_sz, num_read;
    int ret, more_chunks = 1;
    char *filename = "./record-8k-16bit-le-new.wav";
    int frame_size, frames_per_sec;
	
	ret = play_wav_sem_open();
	pr_debug("%s: <sem> ret = %d\n", __func__, ret);
    if (ret) {
    	play_wav->run = 0;
    	play_wav->loop = 0;
		pr_err("%s: sem ERROR (%s)\n", __func__, strerror(errno));
	} else {
		play_wav->run = 1;
		play_wav->loop = 1;
	}
	pthread_detach(pthread_self());
	
	while (play_wav->run) {
		play_wav_sem_wait();

		audio_play_start();
		usleep(1000);
		more_chunks = 1;
		filename = play_wav->file;
		pr_debug("%s: filename = %s\n", __func__, filename);

		file = fopen(filename, "rb");
		if (!file) {
			pr_err("%s: Unable to open file '%s'\n", __func__, filename);
		    continue;
		}

		fread(&riff_wave_header, sizeof(riff_wave_header), 1, file);
		if ((riff_wave_header.riff_id != ID_RIFF) ||
		    (riff_wave_header.wave_id != ID_WAVE)) {
		    pr_err("%s: Error: '%s' is not a riff/wave file\n", __func__, filename);
		    goto err_c;
		}
		
		do {
		    fread(&chunk_header, sizeof(chunk_header), 1, file);

		    switch (chunk_header.id) {
		    case ID_FMT:
		        fread(&chunk_fmt, sizeof(chunk_fmt), 1, file);
		        /* If the format header is larger, skip the rest */
		        if (chunk_header.sz > sizeof(chunk_fmt))
		            fseek(file, chunk_header.sz - sizeof(chunk_fmt), SEEK_CUR);
		        break;
		    case ID_DATA:
		        /* Stop looking for chunks */
		        more_chunks = 0;
		        chunk_header.sz = le32toh(chunk_header.sz);
		        break;
		    default:
		        /* Unknown chunk, skip bytes */
		        fseek(file, chunk_header.sz, SEEK_CUR);
		    }
		} while (more_chunks);

		frame_len = chunk_fmt.block_align;
		frame_count = chunk_fmt.sample_rate / 50;
		frame_size = frame_len * frame_count;

		data_sz = chunk_header.sz;
		pr_debug("%s: data_sz = %d, frame_len = %d, frame_count = %d\n", __func__, data_sz, frame_len, frame_count);
		pr_debug("%s: frame_size = %d, file = %p\n", __func__, frame_size, file);
		
		do {
			usleep(20*1000);
		    num_read = fread(frames, 1, frame_size, file);
		    
		    if (num_read > 0) {
		        data_sz -= num_read;
		        pr_debug("%s: num_read = %d\n", __func__, num_read);
		        tinyplay_write(frames, num_read);
		    }
		} while (num_read > 0 && data_sz > 0 && play_wav->loop);
		pr_debug("%s: data_sz = %d, num_read = %d\n", __func__, data_sz, num_read);
		
		audio_play_stop();
err_c:
		fclose(file);
    }
    
    play_wav_sem_close();

	return NULL;
}
thread_initcall(play_wav_thread);

static void play_wav_stop(void)
{
	play_wav->loop = 0;
}

static __exit_0 void play_wav_exit(void)
{
	play_wav->run = 0;
	play_wav_stop();
}

static int play_wav_start(char *file)
{
	if (!file) {
		return 1;
	}
	
	play_wav->loop = 1;
	memset(play_wav->file, 0, sizeof(play_wav->file));
	strcpy(play_wav->file, file);
	
	return play_wav_sem_post();
}



static DBusMessage *mothod_start_play(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
	DBusMessage *message;
	
	char *file = NULL;

	if (dbus_message_get_args(msg, NULL, 
						DBUS_TYPE_STRING, &file,
						DBUS_TYPE_INVALID) == FALSE) {
		return btd_error_invalid_args(msg);
	}

	pr_info("%s: file = %s\n", __func__, file);
	
	if (file) {
		play_wav_start(file);
	}

	return dbus_message_new_method_return(msg);
}

static DBusMessage *mothod_stop_play(DBusConnection *conn,
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
	play_wav_stop();
	pr_debug("%s: <play_wav_stop> capture \n", __func__);
	
	return dbus_message_new_method_return(msg);
}

static const GDBusMethodTable play_wav_methods[] = {
	{ GDBUS_METHOD("Start",
			GDBUS_ARGS({ "play", "s" }), NULL, mothod_start_play) },
	{ GDBUS_METHOD("Stop",
			GDBUS_ARGS({ "play", "u" }), NULL, mothod_stop_play) },
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

static const GDBusPropertyTable play_wav_properties[] = {
	{ "Switch1", "b", property_get_switch, property_set_switch },
	{ }
};

static int play_wav_init(void)
{
	DBusConnection *conn = btd_get_dbus_connection();

	/* Registering interface after querying properties */
	if (!g_dbus_register_interface(conn,
				       "/org/wav",
				       "org.wav.play", play_wav_methods,
				       NULL, play_wav_properties, NULL, NULL)) {
				       
		pr_err("Failed to register " "/org/wav \n");
	}	

	return 0;	
}
fn_initcall(play_wav_init);


bool play_wav_file_start(void)
{
 	DBusMessage *message;
 	char *service_name = (char *)dbus_service_name();
 	DBusConnection *connection = btd_get_dbus_connection();

	char *file = "/root/record-8k-16bit-le-new.wav";
 	
 	message = dbus_message_new_method_call(
 					service_name, 
 					"/org/wav",
 					"org.wav.play", "Start");
 					
 	if (message == NULL) {
		return FALSE;
	}

	dbus_message_append_args(message, 
						DBUS_TYPE_STRING, &file,
						DBUS_TYPE_INVALID);

 	return g_dbus_send_message(connection, message);
} 

bool play_wav_file_stop(void)
{
 	DBusMessage *message;
 	char *service_name = (char *)dbus_service_name();
 	DBusConnection *connection = btd_get_dbus_connection();

 	uint32_t value = 8;
 	
 	message = dbus_message_new_method_call(
 					service_name,
 					"/org/wav",
 					"org.wav.play", "Stop");
 					
 	if (message == NULL) {
		return FALSE;
	}

	dbus_message_append_args(message, 
								DBUS_TYPE_UINT32, &value, 
								DBUS_TYPE_INVALID);

 	return g_dbus_send_message(connection, message);
}




























