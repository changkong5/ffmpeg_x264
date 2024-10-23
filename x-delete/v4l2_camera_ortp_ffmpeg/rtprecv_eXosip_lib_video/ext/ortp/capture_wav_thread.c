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

#define FORMAT_PCM 1

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

struct capture_wav {
	sem_t sem;
	int run;
	int loop;
	int framerate;
	char file[128];
};

static struct capture_wav capture_wav[1];

static int capture_wav_sem_open(void)
{
	int value = 0;

	return sem_init(&capture_wav->sem, 0, value);
}

static int capture_wav_sem_wait(void)
{
	return sem_wait(&capture_wav->sem);
}

static int capture_wav_sem_post(void)
{
	return sem_post(&capture_wav->sem);
}

static int capture_wav_sem_close(void)
{
	return sem_destroy(&capture_wav->sem);
}


static void *capture_wav_thread(void *arg)
{
    FILE *file;
    uint8_t buffer[320];
    struct wav_header header;
    unsigned int card = 0;
    unsigned int device = 0;
    unsigned int channels = 1;
    unsigned int rate = 8000;
    unsigned int bits = 16;
    unsigned int frames;
    unsigned int period_size = 1024;
    unsigned int period_count = 4;
    unsigned int cap_time = 0;
    enum pcm_format format;
    
    // char *filename = "./record-8k-16bit-le-new.wav";
    
    
    int frame_len, frame_count, data_sz, num_read;
    int ret, more_chunks = 1;
    char *filename = "/root/write_cap.wav";
    int frame_size, frames_per_sec;
	
	ret = capture_wav_sem_open();
	pr_debug("%s: <sem> ret = %d\n", __func__, ret);
    if (ret) {
    	capture_wav->run = 0;
    	capture_wav->loop = 0;
		pr_err("%s: sem ERROR (%s)\n", __func__, strerror(errno));
	} else {
		capture_wav->run = 1;
		capture_wav->loop = 1;
	}
	pthread_detach(pthread_self());
	
	while (capture_wav->run) {
		capture_wav_sem_wait();

		filename = capture_wav->file;
		pr_debug("%s: filename = %s\n", __func__, filename);

		file = fopen(filename, "wb");
		if (!file) {
			pr_err("%s: Unable to open file '%s'\n", __func__, filename);
		    continue;
		}

		header.riff_id = ID_RIFF;
		header.riff_sz = 0;
		header.riff_fmt = ID_WAVE;
		header.fmt_id = ID_FMT;
		header.fmt_sz = 16;
		header.audio_format = FORMAT_PCM;
		header.num_channels = channels;		// channels = 6
		header.sample_rate = rate;			// rate = 4800

		format = PCM_FORMAT_S16_LE;
		
		header.bits_per_sample = pcm_format_to_bits(format);				// format = PCM_FORMAT_S16_LE
		header.byte_rate = (header.bits_per_sample / 8) * channels * rate;
		header.block_align = channels * (header.bits_per_sample / 8);
		header.data_id = ID_DATA;

		/* leave enough room for header */
		fseek(file, sizeof(struct wav_header), SEEK_SET);

/*		
		while (play_wav->loop) {
			
			if (fwrite(buffer, 1, size, file) != size) {
            	fprintf(stderr,"Error capturing sample\n");
            	break;
        	}
		}
*/
		/* write header now all information is known */
		header.data_sz = frames * header.block_align;
		header.riff_sz = header.data_sz + sizeof(header) - 8;
		fseek(file, 0, SEEK_SET);
		fwrite(&header, sizeof(struct wav_header), 1, file);
	
err_c:
		fclose(file);
    }
    
    capture_wav_sem_close();

	return NULL;
}
thread_initcall(capture_wav_thread);



static void capture_wav_stop(void)
{
	capture_wav->loop = 0;
}

static __exit_0 void capture_wav_exit(void)
{
	capture_wav->run = 0;
	capture_wav_stop();
}

static int capture_wav_start(char *file)
{
	if (!file) {
		return 1;
	}
	
	capture_wav->loop = 1;
	memset(capture_wav->file, 0, sizeof(capture_wav->file));
	strcpy(capture_wav->file, file);
	
	return capture_wav_sem_post();
}



static DBusMessage *mothod_start_capture(DBusConnection *conn,
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
		capture_wav_start(file);
	}

	return dbus_message_new_method_return(msg);
}

static DBusMessage *mothod_stop_capture(DBusConnection *conn,
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
	capture_wav_stop();
	pr_debug("%s: <capture_wav_stop> capture \n", __func__);
	
	return dbus_message_new_method_return(msg);
}

static const GDBusMethodTable play_wav_methods[] = {
	{ GDBUS_METHOD("Start",
			GDBUS_ARGS({ "capture", "s" }), NULL, mothod_start_capture) },
	{ GDBUS_METHOD("Stop",
			GDBUS_ARGS({ "capture", "u" }), NULL, mothod_stop_capture) },
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

static int capture_wav_init(void)
{
	DBusConnection *conn = btd_get_dbus_connection();

	/* Registering interface after querying properties */
	if (!g_dbus_register_interface(conn,
				       "/org/wav",
				       "org.wav.capture", play_wav_methods,
				       NULL, play_wav_properties, NULL, NULL)) {
				       
		pr_err("Failed to register " "/org/wav \n");
	}	

	return 0;	
}
fn_initcall(capture_wav_init);


bool capture_wav_file_start(void)
{
 	DBusMessage *message;
 	char *service_name = (char *)dbus_service_name();
 	DBusConnection *connection = btd_get_dbus_connection();

	char *file = "/root/write_cap.wav";
 	
 	message = dbus_message_new_method_call(
 					service_name, 
 					"/org/wav",
 					"org.wav.capture", "Start");
 					
 	if (message == NULL) {
		return FALSE;
	}

	dbus_message_append_args(message, 
						DBUS_TYPE_STRING, &file,
						DBUS_TYPE_INVALID);

 	return g_dbus_send_message(connection, message);
} 

bool capture_wav_file_stop(void)
{
 	DBusMessage *message;
 	char *service_name = (char *)dbus_service_name();
 	DBusConnection *connection = btd_get_dbus_connection();

 	uint32_t value = 8;
 	
 	message = dbus_message_new_method_call(
 					service_name,
 					"/org/wav",
 					"org.wav.capture", "Stop");
 					
 	if (message == NULL) {
		return FALSE;
	}

	dbus_message_append_args(message, 
								DBUS_TYPE_UINT32, &value, 
								DBUS_TYPE_INVALID);

 	return g_dbus_send_message(connection, message);
}

















