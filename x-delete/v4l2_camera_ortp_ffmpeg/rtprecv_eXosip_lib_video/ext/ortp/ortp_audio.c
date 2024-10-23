/*
 * Copyright (c) 2010-2022 Belledonne Communications SARL.
 *
 * This file is part of oRTP
 * (see https://gitlab.linphone.org/BC/public/ortp).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <ortp/ortp.h>
#include <signal.h>
#include <stdlib.h>

#ifndef _WIN32
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#endif

#include <pthread.h>
#include <semaphore.h>

#include "info.h"
#include "tinyplay_cap.h"

#include <glib.h>
#include <tinyalsa/pcm.h>

#include "gdbus/gdbus.h"
#include "src/error.h"
#include "src/dbus-common.h"

struct ortp_audio {
	sem_t sem;
	volatile int run;
	volatile int loop;
	
	RtpSession *session;
	
	int session_mode;
	int scheduling_mode;
	int blocking_mode;
	int connected_mode;
	int payload_type;
	int session_ssrc;
	int log_level;
	
	int set_jitter;
	int enable_adaptive;
	
	uint32_t user_rx_ts;
	uint32_t user_tx_ts;

	int lport;
	char lipaddr[16];
	
	int rport;
	char ripaddr[16];
};

static struct ortp_audio ortp_audio[1];

uint8_t g711a_encode(int16_t pcm_val);
int16_t g711a_decode(uint8_t a_val);


static int ortp_audio_sem_open(void)
{
	int value = 0;

	return sem_init(&ortp_audio->sem, 0, value);
}

static int ortp_audio_sem_wait(void)
{
	return sem_wait(&ortp_audio->sem);
}

static int ortp_audio_sem_post(void)
{
	return sem_post(&ortp_audio->sem);
}

static int ortp_audio_sem_close(void)
{
	return sem_destroy(&ortp_audio->sem);
}


static uint32_t ortp_audio_recv(RtpSession *session, uint32_t ts)
{
	int16_t origin[160];
	uint8_t buffer[160];
	int havemore = 1;
	int received = 0;
	
	int frame_len;
	int frame_byte;
	int i, rbyte, wbyte;
			
	while (havemore) {
		frame_len = 160;
		frame_byte = frame_len * 2;
		rbyte = rtp_session_recv_with_ts(session, buffer, frame_len, ts, &havemore);

		if (rbyte != frame_len) {
			// pr_err("%s: <error> frame_len = %d, rbyte = %d\n", __func__, frame_len, rbyte);
		}
				
		if (rbyte > 0) {
			received = 1;
			
			for (i = 0; i < frame_len; i++) {
				origin[i] = g711a_decode(buffer[i]);
			}
		}
				
		if (received) {
			wbyte = tinyplay_write((uint8_t *)origin, frame_byte);
			
			if (wbyte != frame_byte) {
				pr_err("%s: <error> frame_byte = %d, wbyte = %d\n", __func__, frame_byte, wbyte);
			}
		}
		received = 0;

		ts += 160;
	}
	
	return ts;
}

static uint32_t ortp_audio_send(RtpSession *session, uint32_t ts)
{
	int16_t origin[160];
	uint8_t buffer[160];
	
	int frame_len;
	int frame_byte;
	int i, rbyte, wbyte;
	
	frame_len = 160;
	frame_byte = frame_len * 2;
	
	rbyte = tinycap_read(origin, frame_byte);
	
	if (rbyte != frame_byte) {
		pr_err("%s: <error> frame_byte = %d, rbyte = %d\n", __func__, frame_byte, rbyte);
	}
			
	for (i = 0; i < frame_len; i++) {
		buffer[i] = g711a_encode(origin[i]);
	}

	wbyte = rtp_session_send_with_ts(session, buffer, frame_len, ts);

	// remove ortp header length
	wbyte -= 12;
	
	if (wbyte != frame_len) {
		pr_err("%s: <error> frame_len = %d, wbyte = %d\n", __func__, frame_len, wbyte);
	}
	
	ts += frame_len;
	
	return ts;
}

static void *ortp_audio_thread(void *arg)
{
	int ret;
	int play_remain;
	int capture_remain;
	RtpSession *session;
	uint32_t user_rx_ts;
	uint32_t user_tx_ts;
	SessionSet *rdset = NULL;
	SessionSet *wrset = NULL;
	
	pthread_detach(pthread_self());
	
	ret = ortp_audio_sem_open();
  pr_debug("%s: <sem> ret = %d\n", __func__, ret);
    
  if (ret) {
    ortp_audio->run = 0;
    ortp_audio->loop = 0;
		pr_err("%s: pipe ERROR (%s)\n", __func__, strerror(errno));
	} else {
		ortp_audio->run = 1;
		ortp_audio->loop = 1;
	}

	user_rx_ts = 0;
	user_tx_ts = 0;

	ortp_init();
	ortp_scheduler_init();
	ortp_set_log_level_mask(NULL, 0);
	
	while (ortp_audio->run) {
		pr_info("%s: ===========================================================================> wait ...\n", __func__);
		ortp_audio_sem_wait();
		
		user_rx_ts = 0;
		user_tx_ts = 0;
		
		rdset = session_set_new();
		wrset = session_set_new();
		session = ortp_audio->session;

		// RTP_SESSION_RECVONLY = 0
		// RTP_SESSION_SENDONLY = 1
		// RTP_SESSION_SENDRECV = 2

		play_remain = tinyplay_remain_nbyte();
		capture_remain = tinycap_remain_nbyte();
		pr_info("=======================================================>%s: play_remain = %d, capture_remain = %d\n", __func__, play_remain, capture_remain);

		if (ortp_audio->session_mode == RTP_SESSION_SENDONLY) {
			audio_capture_start();
		} else if (ortp_audio->session_mode == RTP_SESSION_RECVONLY) {
			audio_play_start();
		} else if (ortp_audio->session_mode == RTP_SESSION_SENDRECV) {
			audio_play_start();
			audio_capture_start();
		} else {
			pr_err("%s: <error> ortp_audio->session_mode = %d\n", __func__, ortp_audio->session_mode);
			continue;
		}

#if 0
		tinycap_set_rw_nonblocking(1);
		tinyplay_set_rw_nonblocking(1);
#endif

		usleep(10*000);
		pr_info("%s: ===========================================================================> start <ortp_audio->session_mode = %d>...\n", __func__, ortp_audio->session_mode);
		
		while (ortp_audio->loop) {
			/* add the session to the set */

			if (ortp_audio->session_mode == RTP_SESSION_SENDONLY) {
				session_set_set(wrset, session);
			} else if (ortp_audio->session_mode == RTP_SESSION_RECVONLY) {
				session_set_set(rdset, session);
			} else if (ortp_audio->session_mode == RTP_SESSION_SENDRECV) {
				session_set_set(rdset, session);
				session_set_set(wrset, session);
			} else {
				pr_err("%s: <error>\n", __func__);
			}
		
			// ret = session_set_select(rdset, wrset, NULL);
			// if (ret == 0) pr_info("warning: session_set_select() is returning 0...\n");
			
			if (session_set_is_set(rdset, session)) {
				user_rx_ts = ortp_audio_recv(session, user_rx_ts);
				
				usleep(10*1000);
				// usleep(20*1000);
			}
			
			if (session_set_is_set(wrset, session)) {
				user_tx_ts = ortp_audio_send(session, user_tx_ts);
			}
		}

		if (ortp_audio->session_mode == RTP_SESSION_SENDONLY) {
			audio_capture_stop();
		} else if (ortp_audio->session_mode == RTP_SESSION_RECVONLY) {
			audio_play_stop();
		} else if (ortp_audio->session_mode == RTP_SESSION_SENDRECV) {
			audio_play_stop();
			audio_capture_stop();
		} else {
			pr_err("%s: <error> ortp_audio->session_mode = %d\n", __func__, ortp_audio->session_mode);
		}

		pr_info("%s: ===========================================================================> stop ...\n", __func__);
		
		session_set_destroy(rdset);
		session_set_destroy(wrset);
		
		if (session) {
			rtp_session_destroy(session);
		}
	}
	
	ortp_exit();
	ortp_audio_sem_close();

	return NULL;
}
thread_initcall_prio(ortp_audio_thread, NULL, 3);
// thread_initcall(ortp_audio_thread);


void ortp_audio_stop(void)
{
	ortp_audio->loop = 0;
}

void ortp_audio_exit(void)
{
	ortp_audio->run = 0;
	ortp_audio_stop();
}

int ortp_audio_start(void)
{
	ortp_audio->loop = 1;
	return ortp_audio_sem_post();
}


// #define G_SOURCE_REMOVE         FALSE
// #define G_SOURCE_CONTINUE       TRUE

/**
 * FALSE，该事件源将被删除
 * TRUE，该事件源在没有更高优先级事件时，再次运行
*/
static gboolean cancel_fire_5(gpointer data){
  g_print("cancel_fire_5() quit \n");

  // tinycap_start();

#if 0
  // audio_capture_stop();
  audio_capture_start();

  audio_play_start();
#endif

//  ortp_audio_start_i();

  return G_SOURCE_REMOVE;
}

static gboolean cancel_fire_6(gpointer data){
  g_print("cancel_fire_6() quit \n");

  // tinycap_start();

  // audio_capture_stop();
  // audio_capture_start();

//  ortp_audio_stop_i();

  return G_SOURCE_REMOVE;
}

// g_timeout_add(1000, count_down, NULL);

static int main_init(void)
{
	printf("-----main_init------ \n");

	g_timeout_add(1000, cancel_fire_5, NULL);
	g_timeout_add(3000, cancel_fire_6, NULL);
	
	return 0;
}
fn_initcall(main_init);


static DBusMessage *method_start_ortp_audio(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
	RtpSession *session;
	DBusMessage *message;

	int session_mode = RTP_SESSION_SENDRECV;
	int scheduling_mode = 1;
	int blocking_mode = 0;
	int connected_mode = 0;
	int payload_type = 8;
	int session_ssrc = 0;
	int log_level = 0;

	int lport = 8000;
	char lipaddrbuf[16] = "192.168.3.10";
	char *lipaddr = lipaddrbuf;

	int rport = 12222;
	char ripaddrbuf[16] = "192.168.3.4";
	char *ripaddr = ripaddrbuf;

	
	int set_jitter = 40;
	int enable_adaptive = 1;

	if (dbus_message_get_args(msg, NULL, 
						DBUS_TYPE_UINT32, &session_mode,
						DBUS_TYPE_UINT32, &scheduling_mode, 
						DBUS_TYPE_UINT32, &blocking_mode, 
						DBUS_TYPE_UINT32, &connected_mode, 
						DBUS_TYPE_UINT32, &lport,
						DBUS_TYPE_STRING, &lipaddr, 
						DBUS_TYPE_UINT32, &rport, 
						DBUS_TYPE_STRING, &ripaddr, 
						DBUS_TYPE_UINT32, &payload_type,
						DBUS_TYPE_UINT32, &session_ssrc,
						DBUS_TYPE_UINT32, &set_jitter,
						DBUS_TYPE_UINT32, &enable_adaptive,
						DBUS_TYPE_UINT32, &log_level,
						DBUS_TYPE_INVALID) == FALSE) {
		return btd_error_invalid_args(msg);
	}

	pr_info("%s: session_mode    = %d\n", __func__, session_mode);
	pr_info("%s: scheduling_mode = %d\n", __func__, scheduling_mode);
	pr_info("%s: blocking_mode   = %d\n", __func__, blocking_mode);
	pr_info("%s: connected_mode  = %d\n", __func__, connected_mode);
	
	pr_info("%s: lport           = %d\n", __func__, lport);
	pr_info("%s: lipaddr         = %s\n", __func__, lipaddr);
	pr_info("%s: rport           = %d\n", __func__, rport);
	pr_info("%s: ripaddr         = %s\n", __func__, ripaddr);

	pr_info("%s: payload_type    = %d\n", __func__, payload_type);
	pr_info("%s: session_ssrc    = %d\n", __func__, session_ssrc);
	pr_info("%s: set_jitter      = %d\n", __func__, set_jitter);
	pr_info("%s: enable_adaptive = %d\n", __func__, enable_adaptive);
	pr_info("%s: log_level       = %d\n", __func__, log_level);
	
#if 1
	session = rtp_session_new(session_mode);
	
	if (session == NULL) {
		ortp_audio->session = NULL;
		pr_err("%s: <error> rtp_session_new\n", __func__);
    return btd_error_invalid_args(msg);
	} 
	
	rtp_session_set_scheduling_mode(session, scheduling_mode);
	rtp_session_set_blocking_mode(session, blocking_mode);
	// rtp_session_set_connected_mode(session, connected_mode);

	rtp_session_set_local_addr(session, lipaddr, lport, lport + 1);
	rtp_session_set_remote_addr(session, ripaddr, rport);

	rtp_session_set_payload_type(session, payload_type);
	// rtp_session_set_ssrc(session, session_ssrc);

	rtp_session_enable_adaptive_jitter_compensation(session, enable_adaptive);
	// rtp_session_set_recv_buf_size(session, 256);	


	ortp_audio->session = session;
	
	ortp_audio->session_mode = session_mode;
	ortp_audio->scheduling_mode = scheduling_mode;
	ortp_audio->blocking_mode = blocking_mode;
	ortp_audio->connected_mode = connected_mode;
	ortp_audio->payload_type = payload_type;
	ortp_audio->session_ssrc = session_ssrc;
	ortp_audio->set_jitter = set_jitter;
	ortp_audio->enable_adaptive = enable_adaptive;
	
	ortp_audio->lport = lport;
	memcpy(ortp_audio->lipaddr, lipaddr, sizeof(ortp_audio->lipaddr));
	
	ortp_audio->rport = rport;
	memcpy(ortp_audio->ripaddr, ripaddr, sizeof(ortp_audio->ripaddr));
	
	// start ortp audio thread
	ortp_audio_start();
#endif

	return dbus_message_new_method_return(msg);
}

static DBusMessage *method_stop_ortp_audio(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
	uint32_t which;
	DBusMessage *message;
	
	if (dbus_message_get_args(msg, NULL, DBUS_TYPE_UINT32, &which,
						DBUS_TYPE_INVALID) == FALSE)
		return btd_error_invalid_args(msg);
		
	pr_info("%s: which = %d\n", __func__, which);

	// stop ortp audio thread
	ortp_audio_stop();
	pr_debug("%s: <ortp_audio_stop> capture \n", __func__);
	
	return dbus_message_new_method_return(msg);
}

static const GDBusMethodTable ortp_audio_methods[] = {
	{ GDBUS_METHOD("Start",
			GDBUS_ARGS({ "ortp_audio", "uuuuususuuuuu" }), NULL, method_start_ortp_audio) },
	{ GDBUS_METHOD("Stop",
			GDBUS_ARGS({ "ortp_audio", "u" }), NULL, method_stop_ortp_audio) },
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

static const GDBusPropertyTable ortp_audio_properties[] = {
	{ "Switch1", "b", property_get_switch, property_set_switch },
	{ }
};

static int ortp_audio_init(void)
{
	DBusConnection *conn = btd_get_dbus_connection();

	/* Registering interface after querying properties */
	if (!g_dbus_register_interface(conn,
				       "/org/audio",
				       "org.audio.ortp", ortp_audio_methods,
				       NULL, ortp_audio_properties, NULL, NULL)) {
				       
		pr_err("Failed to register " "/org/audio \n");
	}	

	return 0;	
}
fn_initcall(ortp_audio_init);


bool ortp_audio_start_i(void)
{
#if 1
 	DBusMessage *message;
 	char *service_name = (char *)dbus_service_name();
 	DBusConnection *connection = btd_get_dbus_connection();

	int session_mode = RTP_SESSION_SENDRECV;
	int scheduling_mode = 1;
	int blocking_mode = 0;
	int connected_mode = 0;
	int payload_type = 8;
	int session_ssrc = 0;
	int log_level = 0;

/*	
	int lport = 8000;
	// char *lipaddr = "192.168.3.10";
	char *lipaddr = info_default_ipaddr_get();

	int rport = 12222;
	char *ripaddr = "192.168.3.4";
	// char *ripaddr = info_loopback_ipaddr_get();
*/

	int lport = info_audio_local_port_get();
	char *lipaddr = info_audio_local_ipaddr_get();

	int rport = info_audio_remote_port_get();
	char *ripaddr = info_audio_remote_ipaddr_get();


	int set_jitter = 40;
	int enable_adaptive = 1;
 	
 	message = dbus_message_new_method_call(
 					service_name, 
 					"/org/audio",
 					"org.audio.ortp", "Start");
 					
 	if (message == NULL) {
		return FALSE;
	}

	dbus_message_append_args(message, 
						DBUS_TYPE_UINT32, &session_mode,
						DBUS_TYPE_UINT32, &scheduling_mode, 
						DBUS_TYPE_UINT32, &blocking_mode, 
						DBUS_TYPE_UINT32, &connected_mode, 
						DBUS_TYPE_UINT32, &lport,
						DBUS_TYPE_STRING, &lipaddr, 
						DBUS_TYPE_UINT32, &rport, 
						DBUS_TYPE_STRING, &ripaddr, 
						DBUS_TYPE_UINT32, &payload_type,
						DBUS_TYPE_UINT32, &session_ssrc,
						DBUS_TYPE_UINT32, &set_jitter,
						DBUS_TYPE_UINT32, &enable_adaptive,
						DBUS_TYPE_UINT32, &log_level,
						DBUS_TYPE_INVALID);

 	return g_dbus_send_message(connection, message);
#else
	return true;
#endif
}


bool ortp_audio_stop_i(void)
{
#if 1
 	DBusMessage *message;
 	char *service_name = (char *)dbus_service_name();
 	DBusConnection *connection = btd_get_dbus_connection();

 	uint32_t value = 8;
 	
 	message = dbus_message_new_method_call(
 					service_name, 
 					"/org/audio",
 					"org.audio.ortp", "Stop");
 					
 	if (message == NULL) {
		return FALSE;
	}

	dbus_message_append_args(message, 
								DBUS_TYPE_UINT32, &value, 
								DBUS_TYPE_INVALID);

 	return g_dbus_send_message(connection, message);
#else
	return true;
#endif
} 







