/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * Second generation work queue implementation
 */
#define DEBUG  1

#include <eXosip2/eXosip.h>
#include <osip2/osip_mt.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "initcall.h"

#include "eXosip_main.h"

#include "info.h"
#include "tinyplay_cap.h"
#include "emulplay_cap.h"

#include "gdbus/gdbus.h"
#include "src/error.h"
#include "src/dbus-common.h"

struct eXosip_i {
	int run;
	
	int reg_id;
	int call_id;
	int dialog_id;
	
    int tid; /**< unique id for transactions (to be used for answers) */
    int did; /**< unique id for SIP dialogs */

    int rid; /**< unique id for registration */
    int cid; /**< unique id for SIP calls (but multiple dialogs!) */
    int sid; /**< unique id for outgoing subscriptions */
    int nid; /**< unique id for incoming subscriptions */
	
	
	int port;
	const char *from;
	const char *to;
	const char *proxy;
	const char *username;
	const char *password;
	struct eXosip_t *ctx;
};

#if 1
static struct eXosip_i eXosip_i[1] = {
	[0] = {
		.run = 0,
		.reg_id = 0,
		.call_id = 0,
		.dialog_id = 0,
		.port  = 5061,
		.from  = "sip:haakon@127.0.0.1:5061",
		.to    = "sip:david@127.0.0.1:5062",
		.proxy = "sip:127.0.0.1:5060",
		.username = "haakon",
		.password = "12345678",
	}
};
#else
static struct eXosip_i eXosip_i[1] = {
	[0] = {
		.run = 0,
		.reg_id = 0,
		.call_id = 0,
		.dialog_id = 0,
		.port  = 5061,
		.from  = "sip:haakon@192.168.3.10:5061",
		.to    = "sip:david@192.168.3.4:5062",
		.proxy = "sip:192.168.3.10:5060",
		.username = "haakon",
		.password = "12345678",
	}
};
#endif



static __exit_0 int eXosip_thread_quit(void)
{
	eXosip_i->run = 0;

	return 0;
}

static const char *eXosip_eid(eXosip_event_type_t ev)
{
    switch(ev) {
    case EXOSIP_REGISTRATION_SUCCESS:
        return "register";
    case EXOSIP_CALL_INVITE:
        return "invite";
    case EXOSIP_CALL_REINVITE:
        return "reinvite";
    case EXOSIP_CALL_NOANSWER:
    case EXOSIP_SUBSCRIPTION_NOANSWER:
    case EXOSIP_NOTIFICATION_NOANSWER:
        return "noanswer";
    case EXOSIP_MESSAGE_PROCEEDING:
    case EXOSIP_NOTIFICATION_PROCEEDING:
    case EXOSIP_CALL_MESSAGE_PROCEEDING:
    case EXOSIP_SUBSCRIPTION_PROCEEDING:
    case EXOSIP_CALL_PROCEEDING:
        return "proceed";
    case EXOSIP_CALL_RINGING:
        return "ring";
    case EXOSIP_MESSAGE_ANSWERED:
    case EXOSIP_CALL_ANSWERED:
    case EXOSIP_CALL_MESSAGE_ANSWERED:
    case EXOSIP_SUBSCRIPTION_ANSWERED:
    case EXOSIP_NOTIFICATION_ANSWERED:
        return "answer";
    case EXOSIP_SUBSCRIPTION_REDIRECTED:
    case EXOSIP_NOTIFICATION_REDIRECTED:
    case EXOSIP_CALL_MESSAGE_REDIRECTED:
    case EXOSIP_CALL_REDIRECTED:
    case EXOSIP_MESSAGE_REDIRECTED:
        return "redirect";
    case EXOSIP_REGISTRATION_FAILURE:
        return "noreg";
    case EXOSIP_SUBSCRIPTION_REQUESTFAILURE:
    case EXOSIP_NOTIFICATION_REQUESTFAILURE:
    case EXOSIP_CALL_REQUESTFAILURE:
    case EXOSIP_CALL_MESSAGE_REQUESTFAILURE:
    case EXOSIP_MESSAGE_REQUESTFAILURE:
        return "failed";
    case EXOSIP_SUBSCRIPTION_SERVERFAILURE:
    case EXOSIP_NOTIFICATION_SERVERFAILURE:
    case EXOSIP_CALL_SERVERFAILURE:
    case EXOSIP_CALL_MESSAGE_SERVERFAILURE:
    case EXOSIP_MESSAGE_SERVERFAILURE:
        return "server";
    case EXOSIP_SUBSCRIPTION_GLOBALFAILURE:
    case EXOSIP_NOTIFICATION_GLOBALFAILURE:
    case EXOSIP_CALL_GLOBALFAILURE:
    case EXOSIP_CALL_MESSAGE_GLOBALFAILURE:
    case EXOSIP_MESSAGE_GLOBALFAILURE:
        return "global";
    case EXOSIP_CALL_ACK:
        return "ack";
    case EXOSIP_CALL_CLOSED:
    case EXOSIP_CALL_RELEASED:
        return "bye";
    case EXOSIP_CALL_CANCELLED:
        return "cancel";
    case EXOSIP_MESSAGE_NEW:
    case EXOSIP_CALL_MESSAGE_NEW:
    case EXOSIP_IN_SUBSCRIPTION_NEW:
        return "new";
    case EXOSIP_SUBSCRIPTION_NOTIFY:
        return "notify";
    default:
        break;
    }
    return "unknown";
}

static void *eXosip_thread(void *arg)
{
	int i, j, ret;
	int pos = 0;
	int port = 0;
	eXosip_event_t *event;
	osip_message_t *ack = NULL;
	osip_message_t *answer = NULL;
	
	sdp_message_t *sdp = NULL;
  	sdp_media_t *sdp_media = NULL;

  	char *key, *value;

	eXosip_i->ctx = eXosip_malloc();
	
	if (!eXosip_i->ctx) {
		pr_err("%s: error eXosip_malloc \n", __func__);
		goto err_malloc;
	}

	ret = eXosip_init(eXosip_i->ctx);
	if (ret != 0) {
		pr_err("%s: error eXosip_init ret = %d \n", __func__, ret);
		goto err_init;
    }

    // port = eXosip_i->port;
    port = info_exosip_from_port_get();
    
    ret = eXosip_listen_addr(eXosip_i->ctx, IPPROTO_UDP, NULL, port, AF_INET, 0);
  	if (ret != 0) {
		pr_err("%s: error eXosip_listen_addr ret = %d \n", __func__, ret);
		goto err_listen;
    }
    eXosip_i->run = 1;
    
    while (eXosip_i->run) {
    	event = eXosip_event_wait(eXosip_i->ctx, 0, -1);
    	
    	eXosip_lock(eXosip_i->ctx);
    	eXosip_automatic_action(eXosip_i->ctx);
    	eXosip_unlock(eXosip_i->ctx);
    	
    	if (!event) {
    		continue;
    	}
    	pr_debug("sip: event %s(%d); cid=%d, did=%d\n", eXosip_eid(event->type), event->type, event->cid, event->did);
    	
    	switch (event->type) {
    	case EXOSIP_REGISTRATION_SUCCESS:
    		pr_debug("----------<EXOSIP_REGISTRATION_SUCCESS>---------\n");
    		pr_debug("registrered successfully\n");
    		break;
    	case EXOSIP_REGISTRATION_FAILURE:
    		pr_debug("----------<EXOSIP_REGISTRATION_FAILURE>---------\n");

    		if ((event->response != NULL) && (event->response->status_code == 401)) {
    			pr_debug("%s: event->response->status_code = %d\n", __func__, event->response->status_code);
    		/*
    			// for EXOSIP_REGISTRATION_FAILURE event
    		
    			// method 1
    			// call eXosip_automatic_action function to dealwith 401 and 407 state
    			
    			// method 2
    			// add the follow code to send register info
    			
    			osip_message_t *reg = NULL;
				eXosip_lock();
				eXosip_clear_authentication_info(); // 清空认证信息      
				eXosip_add_authentication_info(username, username, password, "MD5", NULL); // 添加认证信息
				eXosip_register_build_register(event->rid, expires, &reg);
				eXosip_register_send_register(event->rid, reg); // 发送注册请求
				eXosip_unlock();
			*/
    		} else {
    			pr_debug("registrered failure\n");
    		}
    		
    		break;
    		
    	case EXOSIP_CALL_PROCEEDING:
		    pr_debug("----------<EXOSIP_CALL_PROCEEDING>---------\n");
		    pr_debug ("proceeding!\n");
			break;

		case EXOSIP_CALL_RINGING:
		    pr_debug("----------<EXOSIP_CALL_RINGING>---------\n");
			pr_debug("ringing!\n");
			// eXosip_i->call_id = event->cid;
			// eXosip_i->dialog_id = event->did;
			pr_debug("call_id is %d, dialog_id is %d \n", event->cid, event->did);
			break;
		case EXOSIP_CALL_ANSWERED:
			pr_debug("----------<EXOSIP_CALL_ANSWERED>---------\n");
		    pr_debug("ok! connected!\n");
		    eXosip_i->call_id = event->cid;
		    eXosip_i->dialog_id = event->did;
		    
		    eXosip_i->cid = event->cid;
		    eXosip_i->did = event->did;
		    
		    pr_debug("call_id is %d, dialog_id is %d \n", event->cid, event->did);
		    
		    sdp = eXosip_get_remote_sdp(eXosip_i->ctx, eXosip_i->did);
		    
      		printf ("------------------1-0-1-------------------\n");
 		
      		// printf("sdp->v_version      = %s\n", sdp->v_version);
      		printf("sdp->v_version      = %s\n", sdp_message_v_version_get(sdp));
      		printf("sdp->o_username     = %s\n", sdp->o_username);
      		printf("sdp->o_sess_id      = %s\n", sdp->o_sess_id);
      		printf("sdp->o_sess_version = %s\n", sdp->o_sess_version);
      		printf("sdp->o_nettype      = %s\n", sdp->o_nettype);
      		printf("sdp->o_addrtype     = %s\n", sdp->o_addrtype);
      		printf("sdp->o_addr         = %s\n", sdp->o_addr);
      		printf("sdp->s_name         = %s\n", sdp->s_name);
      		printf("sdp->i_info         = %s\n", sdp->i_info);
      		// printf("sdp->u_uri          = %s\n", sdp->u_uri);
      		printf("sdp->u_uri          = %s\n", sdp_message_u_uri_get(sdp));
      		
      		printf ("------------------1-1--------------------\n");
      		
      		// for (i = 0; !osip_list_eol(&sdp->m_medias, i); i++) {
      		for (i = 0; !sdp_message_endof_media(sdp, i); i++) {
      			sdp_media = osip_list_get(&sdp->m_medias, i);

      			key = g_strdup_printf("%s:rport", sdp_message_m_media_get(sdp, i));
      			value = sdp_message_m_port_get(sdp, i);
      			info_key_value_set(key, value);
            	g_free(key);
            	
      		
      			printf("sdp_media->m_media          = %s\n", sdp_message_m_media_get(sdp, i));
      			printf("sdp_media->m_port           = %s\n", sdp_message_m_port_get(sdp, i));
      			printf("sdp_media->m_number_of_port = %s\n", sdp_message_m_number_of_port_get(sdp, i));
      			printf("sdp_media->m_proto          = %s\n", sdp_message_m_proto_get(sdp, i));
      			
			  	printf("sdp_key->k_keytype          = %s\n", sdp_message_k_keytype_get(sdp, i));
			  	printf("sdp_key->k_keydata          = %s\n", sdp_message_k_keydata_get(sdp, i));
      			
      			for (j = 0; !osip_list_eol(&sdp_media->m_payloads, j); j++) {
      				printf("sdp_media->m_payload	= %s\n", sdp_message_m_payload_get(sdp, i, j));
      			}
      			
      			for (j = 0; !osip_list_eol(&sdp_media->a_attributes, j); j++) {
      				printf("sdp_attribute->a_att_field = %s\n", sdp_message_a_att_field_get(sdp, i, j));
      				printf("sdp_attribute->a_att_value = %s\n", sdp_message_a_att_value_get(sdp, i, j));
      			}
      			
      			
      			for (j = 0; !osip_list_eol(&sdp_media->c_connections, j); j++) {
      				printf("sdp_connection->c_nettype = %s\n", sdp_message_c_nettype_get(sdp, i, j));
      				printf("sdp_connection->c_addrtype = %s\n", sdp_message_c_addrtype_get(sdp, i, j));
      				
      				printf("sdp_connection->c_addr = %s\n", sdp_message_c_addr_get(sdp, i, j));
      				printf("sdp_connection->c_addr_multicast_ttl = %s\n", sdp_message_c_addr_multicast_ttl_get(sdp, i, j));
      				printf("sdp_connection->c_addr_multicast_int = %s\n", sdp_message_c_addr_multicast_int_get(sdp, i, j));

      				key = g_strdup_printf("%s:ripaddr", sdp_message_m_media_get(sdp, i));
	      			value = sdp_message_c_addr_get(sdp, i, j);
	      			info_key_value_set(key, value);
	            	g_free(key);
      			}
      			
      		}
		
			printf ("------------------2---------------------\n");
   		
      		for (i = 0; !osip_list_eol(&sdp->a_attributes, i); i++) {
      			printf("sdp_attribute->a_att_field = %s\n", sdp_message_a_att_field_get(sdp, -1, i));
      			printf("sdp_attribute->a_att_value = %s\n", sdp_message_a_att_value_get(sdp, -1, i));
      		}

			for (i = 0; !osip_list_eol(&sdp->t_descrs, i); i++) {
      			printf("sdp_time_descr->t_start_time = %s\n", sdp_message_t_start_time_get(sdp, i));
      			printf("sdp_time_descr->t_stop_time = %s\n", sdp_message_t_stop_time_get(sdp, i));
      		}

			for (i = 0; !osip_list_eol(&sdp->e_emails, i); i++) {
				printf("sdp->e_emails = %s\n", sdp_message_e_email_get(sdp, i));
			}
			  		
			for (i = 0; !osip_list_eol(&sdp->p_phones, i); i++) {
			  	printf("sdp->p_phones = %s\n", sdp_message_p_phone_get(sdp, i));
			}
			
			info_htable_display();

			exosip_call_answered_event();
			// start ortp audio thread
			// ortp_audio_start_i();
			printf ("------------------3---------------------\n");

		    eXosip_call_build_ack(eXosip_i->ctx, event->did, &ack);
		    eXosip_call_send_ack(eXosip_i->ctx, event->did, ack);

		    // int main_send(int argc, char *argv[]);
		    // main_send(0, NULL);
		    // tinycap_start();
			break;
#if 0
		// client
		case EXOSIP_CALL_INVITE:
		    pr_debug("----------<EXOSIP_CALL_INVITE>---------\n");
		    pr_debug("a new invite reveived!\n");
		    break;
#else
		case EXOSIP_CALL_INVITE:
			pr_debug("----------<EXOSIP_CALL_INVITE>---------\n");
			pr_debug("Received a INVITE msg from %s:%s, UserName is %s, password is %s\n",
					 event->request->req_uri->host,
          			 event->request->req_uri->port, 
          			 event->request->req_uri->username, 
          			 event->request->req_uri->password);

		  	sdp = eXosip_get_remote_sdp(eXosip_i->ctx, event->did);
		  	eXosip_i->call_id = event->cid;
		  	eXosip_i->dialog_id = event->did;
		  	
		  	eXosip_i->cid = event->cid;
		    eXosip_i->did = event->did;
		    eXosip_i->tid = event->tid;

      		eXosip_lock(eXosip_i->ctx);
      		eXosip_call_send_answer(eXosip_i->ctx, event->tid, 180, NULL);
      		ret = eXosip_call_build_answer(eXosip_i->ctx, event->tid, 200, &answer);
      		
      		if (ret != 0) {
		    	pr_debug("This request msg is invalid!Cann't response!\n");
		      	eXosip_call_send_answer(eXosip_i->ctx, event->tid, 400, NULL);
        	} else {
        		char body[1024];
        		
        		char *local_ipaddr = info_audio_local_ipaddr_get(); 	// "127.0.0.1";
				int   audio_lport = info_audio_local_port_get();		// 8000
				char *audio_lipaddr = info_audio_local_ipaddr_get(); 	// "127.0.0.1";
				int   video_lport = info_video_local_port_get();		// 6000
				char *video_lipaddr = info_video_local_ipaddr_get(); 	// "127.0.0.1";
        		
				snprintf(body, sizeof(body),
				    "v=0\r\n"
				    "o=username 0 0 IN IP4 %s\r\n"
				    "s=session_name\r\n"
				    "i=A Seminar on the session description protocol\r\n"
				    "e=mjh@isi.edu (Mark Handley)\r\n"
				    "p=+44-171-380-7777\r\n"
				    "t=1 10\r\n"
				    "a=recvonly\r\n"
				    "a=framerate:50\r\n"
				    "a=username:rainfish\r\n"
				    "a=password:123\r\n"
				    "m=audio %d/1 RTP/AVP 8\r\n"
				    "c=IN IP4 %s\r\n"
				    "a=rtpmap:8 PCMA/8000\r\n"
				    "a=sendrecv\r\n"
				    "a=active\r\n"
				    "m=video %d/1 RTP/AVP 96\r\n"
				    "c=IN IP4 %s\r\n"
				    "a=rtpmap:96 H264/90000\r\n"
				    "a=sendonly\r\n"
				    "a=active\r\n", local_ipaddr, audio_lport, audio_lipaddr, video_lport, video_lipaddr);

				osip_message_set_body(answer, body, strlen(body));
		      	osip_message_set_content_type(answer, "application/sdp");

		      	eXosip_call_send_answer(eXosip_i->ctx, event->tid, 200, answer);
		      	pr_debug("send 200 over!\n");
        	}
      		eXosip_unlock(eXosip_i->ctx);

      		printf ("------------------1-0-2-------------------\n");
 		
      		// printf("sdp->v_version      = %s\n", sdp->v_version);
      		printf("sdp->v_version      = %s\n", sdp_message_v_version_get(sdp));
      		printf("sdp->o_username     = %s\n", sdp->o_username);
      		printf("sdp->o_sess_id      = %s\n", sdp->o_sess_id);
      		printf("sdp->o_sess_version = %s\n", sdp->o_sess_version);
      		printf("sdp->o_nettype      = %s\n", sdp->o_nettype);
      		printf("sdp->o_addrtype     = %s\n", sdp->o_addrtype);
      		printf("sdp->o_addr         = %s\n", sdp->o_addr);
      		printf("sdp->s_name         = %s\n", sdp->s_name);
      		printf("sdp->i_info         = %s\n", sdp->i_info);
      		// printf("sdp->u_uri          = %s\n", sdp->u_uri);
      		printf("sdp->u_uri          = %s\n", sdp_message_u_uri_get(sdp));
      		
      		printf ("------------------1-1--------------------\n");
      		
      		// for (i = 0; !osip_list_eol(&sdp->m_medias, i); i++) {
      		for (i = 0; !sdp_message_endof_media(sdp, i); i++) {
      			sdp_media = osip_list_get(&sdp->m_medias, i);

      			key = g_strdup_printf("%s:rport", sdp_message_m_media_get(sdp, i));
      			value = sdp_message_m_port_get(sdp, i);
      			info_key_value_set(key, value);
            	g_free(key);
      		
      			printf("sdp_media->m_media          = %s\n", sdp_message_m_media_get(sdp, i));
      			printf("sdp_media->m_port           = %s\n", sdp_message_m_port_get(sdp, i));
      			printf("sdp_media->m_number_of_port = %s\n", sdp_message_m_number_of_port_get(sdp, i));
      			printf("sdp_media->m_proto          = %s\n", sdp_message_m_proto_get(sdp, i));
      			
			  	printf("sdp_key->k_keytype          = %s\n", sdp_message_k_keytype_get(sdp, i));
			  	printf("sdp_key->k_keydata          = %s\n", sdp_message_k_keydata_get(sdp, i));
      			
      			for (j = 0; !osip_list_eol(&sdp_media->m_payloads, j); j++) {
      				printf("sdp_media->m_payload	= %s\n", sdp_message_m_payload_get(sdp, i, j));
      			}
      			
      			for (j = 0; !osip_list_eol(&sdp_media->a_attributes, j); j++) {
      				printf("sdp_attribute->a_att_field = %s\n", sdp_message_a_att_field_get(sdp, i, j));
      				printf("sdp_attribute->a_att_value = %s\n", sdp_message_a_att_value_get(sdp, i, j));
      			}
      			
      			
      			for (j = 0; !osip_list_eol(&sdp_media->c_connections, j); j++) {
      				printf("sdp_connection->c_nettype = %s\n", sdp_message_c_nettype_get(sdp, i, j));
      				printf("sdp_connection->c_addrtype = %s\n", sdp_message_c_addrtype_get(sdp, i, j));
      				
      				printf("sdp_connection->c_addr = %s\n", sdp_message_c_addr_get(sdp, i, j));
      				printf("sdp_connection->c_addr_multicast_ttl = %s\n", sdp_message_c_addr_multicast_ttl_get(sdp, i, j));
      				printf("sdp_connection->c_addr_multicast_int = %s\n", sdp_message_c_addr_multicast_int_get(sdp, i, j));

      				key = g_strdup_printf("%s:ripaddr", sdp_message_m_media_get(sdp, i));
	      			value = sdp_message_c_addr_get(sdp, i, j);
	      			info_key_value_set(key, value);
	            	g_free(key);
      			}
      			
      		}
		
			printf ("------------------2---------------------\n");
   		
      		for (i = 0; !osip_list_eol(&sdp->a_attributes, i); i++) {
      			printf("sdp_attribute->a_att_field = %s\n", sdp_message_a_att_field_get(sdp, -1, i));
      			printf("sdp_attribute->a_att_value = %s\n", sdp_message_a_att_value_get(sdp, -1, i));
      		}

			for (i = 0; !osip_list_eol(&sdp->t_descrs, i); i++) {
      			printf("sdp_time_descr->t_start_time = %s\n", sdp_message_t_start_time_get(sdp, i));
      			printf("sdp_time_descr->t_stop_time = %s\n", sdp_message_t_stop_time_get(sdp, i));
      		}

			for (i = 0; !osip_list_eol(&sdp->e_emails, i); i++) {
				printf("sdp->e_emails = %s\n", sdp_message_e_email_get(sdp, i));
			}
			  		
			for (i = 0; !osip_list_eol(&sdp->p_phones, i); i++) {
			  	printf("sdp->p_phones = %s\n", sdp_message_p_phone_get(sdp, i));
			}
			
			info_htable_display();
			printf ("------------------3---------------------\n");

      		pr_debug("the INFO is :\n");
      		
      		while (!osip_list_eol (&sdp->a_attributes, pos)) {
				sdp_attribute_t *at;

          		at = (sdp_attribute_t *) osip_list_get (&sdp->a_attributes, pos);
          		pr_debug("%s : %s\n", at->a_att_field, at->a_att_value);

          		pos ++;
        	}
		break;
#endif

#if 0
		// client
		case EXOSIP_CALL_CLOSED:
		    pr_debug("----------<EXOSIP_CALL_CLOSED>---------\n");
		    pr_debug("the other sid closed!\n");
		    break;
#else
		// server
    	case EXOSIP_CALL_CLOSED:
    		pr_debug("----------<EXOSIP_CALL_CLOSED>---------\n");
		    pr_debug("a BYE was received for this call\n");
      		pr_debug("the remote hold the session!\n");
      		// eXosip_call_build_ack(dialog_id, &ack);
      		//eXosip_call_send_ack(dialog_id, ack); 
      		ret = eXosip_call_build_answer(eXosip_i->ctx, event->tid, 200, &answer);
      		if (ret != 0) {
          		pr_debug("This request msg is invalid!Cann't response!\n");
          		eXosip_call_send_answer(eXosip_i->ctx, event->tid, 400, NULL);
          	} else {
          		eXosip_call_send_answer(eXosip_i->ctx, event->tid, 200, answer);
          		pr_debug("bye send 200 over!\n");
        	}

			exosip_call_closed_event();
        	// stop ortp audio thread
        	// ortp_audio_stop_i(); 
		break;
#endif	      		
		case EXOSIP_CALL_ACK:
		    pr_debug("----------<EXOSIP_CALL_ACK>---------\n");
		    pr_debug("ACK received!\n");

			exosip_call_ack_event();
		    // start ortp audio thread
		    // ortp_audio_start_i();
		    break;	
    	case EXOSIP_MESSAGE_NEW:
    		pr_debug("----------<EXOSIP_MESSAGE_NEW>---------\n");
    		
      		if (MSG_IS_MESSAGE(event->request)) {
				osip_body_t *body;
				osip_message_get_body(event->request, 0, &body); 
				pr_debug("I get the msg is: %s\n", body->body);
			  	eXosip_message_build_answer(eXosip_i->ctx, event->tid, 200, &answer);
			  	eXosip_message_send_answer(eXosip_i->ctx, event->tid, 200, answer);
        	}
			break;
			
		case EXOSIP_CALL_MESSAGE_NEW:
			pr_debug("----------<EXOSIP_CALL_MESSAGE_NEW>---------\n");

      		if (MSG_IS_INFO(event->request)) {
				eXosip_lock (eXosip_i->ctx);
              	ret = eXosip_call_build_answer(eXosip_i->ctx, event->tid, 200, &answer);
              	if (ret == 0) {
                	eXosip_call_send_answer(eXosip_i->ctx, event->tid, 200, answer);
				}
              	eXosip_unlock (eXosip_i->ctx);

				osip_body_t *body;
				osip_message_get_body(event->request, 0, &body);
				pr_debug("the body is %s\n", body->body);
      		}
			break;
			
		case EXOSIP_CALL_MESSAGE_ANSWERED:
			pr_debug("----------<EXOSIP_CALL_MESSAGE_ANSWERED>---------\n");
			pr_debug("announce a 200ok \n");
			
			exosip_call_message_answered_event();
			// stop ortp audio thread
		    // ortp_audio_stop_i();
			break;
			
		case EXOSIP_MESSAGE_ANSWERED:
		    pr_debug("----------<EXOSIP_MESSAGE_ANSWERED>---------\n");
		    pr_debug("200ok received!\n");
		    break;
		    
		case EXOSIP_CALL_RELEASED:
		    pr_debug("----------<EXOSIP_CALL_RELEASED>---------\n");
		    pr_debug("call context is cleared\n");
		    break;
			
		default:
			pr_debug("----------<OTHER RESPONES>---------\n");
			pr_debug("----------<event->type = %d>---------\n", event->type);
			break;
    	}
    	
    	eXosip_event_free(event);
    } 

err_listen:
err_init:
err_malloc:
	eXosip_quit(eXosip_i->ctx);

	return NULL;
}
thread_initcall(eXosip_thread);


static DBusMessage *method_eXosip_register(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
	DBusMessage *message;
    osip_message_t *reg = NULL;
    
    char *from = "sip:haakon@127.0.0.1:5061";
    // char *to   = "sip:david@127.0.0.1:5062";
    char *proxy = "sip:127.0.0.1:5060";
    char *contact ="sip:haakon@127.0.0.1:5061";
    char *username = "haakon";
    char *password = "12345678";
    int ret, expires = 3600;  					// 1 hour 

	if (dbus_message_get_args(msg, NULL,
					DBUS_TYPE_STRING, &from,
					DBUS_TYPE_STRING, &proxy,
					DBUS_TYPE_STRING, &contact, 
					DBUS_TYPE_UINT32, &expires,
					DBUS_TYPE_STRING, &username,
					DBUS_TYPE_STRING, &password,
					DBUS_TYPE_INVALID) == FALSE) {
		return btd_error_invalid_args(msg);
	}
	
	// eXosip_send_register_request();
	// contact = NULL;
	
	pr_info("%s: from     = %s\n", __func__, from);
	pr_info("%s: proxy    = %s\n", __func__, proxy);
	pr_info("%s: contact  = %s\n", __func__, contact);
	pr_info("%s: expires  = %d\n", __func__, expires);
	pr_info("%s: username = %s\n", __func__, username);
	pr_info("%s: password = %s\n", __func__, password);

	eXosip_lock (eXosip_i->ctx);
	
	eXosip_i->rid = eXosip_register_build_initial_register(eXosip_i->ctx, from, proxy, contact, expires, &reg);
	
	if (eXosip_i->rid < 0) {
    	pr_err("%s: <error> [eXosip_i->rid = %d]\n", __func__, eXosip_i->rid);
    	eXosip_unlock (eXosip_i->ctx);
    	return btd_error_invalid_args(msg);
    }
    eXosip_i->reg_id = eXosip_i->rid;
    
    eXosip_set_user_agent(eXosip_i->ctx, "Client1");
	eXosip_add_authentication_info(eXosip_i->ctx, username, username, password, NULL, NULL);
      		
	osip_message_set_supported (reg, "100rel");
	osip_message_set_supported (reg, "path");
			
	ret = eXosip_register_send_register(eXosip_i->ctx, eXosip_i->rid, reg);
	pr_debug("<eXosip_i->rid = %d, ret = %d>\n", eXosip_i->rid, ret);

	eXosip_unlock (eXosip_i->ctx);

	return dbus_message_new_method_return(msg);
}

static DBusMessage *method_eXosip_invite(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
	uint32_t which;
	DBusMessage *message;
	
	int ret;
	char body[1024];
	// char route[32] = {0};
	osip_message_t *invite = NULL;
	
	char *to = "sip:david@127.0.0.1:5062";
	char *from = "sip:haakon@127.0.0.1:5061";
	char *route = "<sip:127.0.0.1:5060;lr>";
	char *subject = "This is a call subject";
	
	char *local_ipaddr = "127.0.0.1";
	int   audio_lport = 8000;
	char *audio_lipaddr = "127.0.0.1";
	int   video_lport = 6000;
	char *video_lipaddr = "127.0.0.1";
	
	
	if (dbus_message_get_args(msg, NULL,
					DBUS_TYPE_STRING, &to,
					DBUS_TYPE_STRING, &from,
					DBUS_TYPE_STRING, &route,
					DBUS_TYPE_STRING, &subject,
					DBUS_TYPE_STRING, &local_ipaddr,
					DBUS_TYPE_UINT32, &audio_lport,
					DBUS_TYPE_STRING, &audio_lipaddr,
					DBUS_TYPE_UINT32, &video_lport,
					DBUS_TYPE_STRING, &video_lipaddr,
					DBUS_TYPE_INVALID) == FALSE) {
		return btd_error_invalid_args(msg);
	}

	// eXosip_send_invite_request();
	
	pr_info("%s: to             = %s\n", __func__, to);
	pr_info("%s: from           = %s\n", __func__, from);
	pr_info("%s: route          = %s\n", __func__, route);
	pr_info("%s: subject        = %s\n", __func__, subject);
	
	pr_info("%s: local_ipaddr   = %s\n", __func__, local_ipaddr);
	pr_info("%s: audio_lport     = %d\n", __func__, audio_lport);
	pr_info("%s: audio_lipaddr   = %s\n", __func__, audio_lipaddr);
	pr_info("%s: video_lport     = %d\n", __func__, video_lport);
	pr_info("%s: video_lipaddr   = %s\n", __func__, video_lipaddr);
	
	// sprintf(route, "<sip:%s:%d;lr>", "127.0.0.1", 5060);
	// sprintf(route, "<%s;lr>", eXosip_i->proxy);
	// pr_debug("route = %s\n", route);
	
	ret = eXosip_call_build_initial_invite(eXosip_i->ctx, &invite, to, from, route, subject);
    if (ret != 0) {
    	pr_err("%s: Intial INVITE failed! <ret = %d>\n", __func__, ret);
    	return btd_error_invalid_args(msg);
    }

	snprintf(body, sizeof(body),
		        "v=0\r\n"
		        "o=username 0 0 IN IP4 %s\r\n"
		        "s=session_name\r\n"
		        "i=A Seminar on the session description protocol\r\n"
		        "e=mjh@isi.edu (Mark Handley)\r\n"
		        "p=+44-171-380-7777\r\n"
		        "t=1 10\r\n"
		        "a=recvonly\r\n"
		        "a=framerate:50\r\n"
		        "a=username:rainfish\r\n"
		        "a=password:123\r\n"
		        "m=audio %d/1 RTP/AVP 8\r\n"
		        "c=IN IP4 %s\r\n"
		        "a=rtpmap:8 PCMA/8000\r\n"
		        "a=sendrecv\r\n"
		        "a=active\r\n"
		        "m=video %d/1 RTP/AVP 96\r\n"
		        "c=IN IP4 %s\r\n"
		        "a=rtpmap:96 H264/90000\r\n"
		        "a=sendonly\r\n"
		        "a=active\r\n", local_ipaddr, audio_lport, audio_lipaddr, video_lport, video_lipaddr); 
		        
	osip_message_set_body(invite, body, strlen(body));
	osip_message_set_content_type(invite, "application/sdp");

	eXosip_lock(eXosip_i->ctx);
	ret = eXosip_call_send_initial_invite(eXosip_i->ctx, invite);
    eXosip_unlock(eXosip_i->ctx);
	
	pr_debug("%s: <eXosip_send_invite_request> \n", __func__);
	
	return dbus_message_new_method_return(msg);
}

static DBusMessage *method_eXosip_message(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
	char *body = NULL;
	osip_message_t *message = NULL;
	
	char *to = "sip:david@127.0.0.1:5062";
	char *from = "sip:haakon@127.0.0.1:5061";
	char *route = "<sip:127.0.0.1:5060;lr>";
	
	if (dbus_message_get_args(msg, NULL,
					DBUS_TYPE_STRING, &to,
					DBUS_TYPE_STRING, &from,
					DBUS_TYPE_STRING, &route,
					DBUS_TYPE_STRING, &body,
					DBUS_TYPE_INVALID) == FALSE) {
		return btd_error_invalid_args(msg);
	}

	// eXosip_send_invite_request();
	
	pr_info("%s: to             = %s\n", __func__, to);
	pr_info("%s: from           = %s\n", __func__, from);
	pr_info("%s: route          = %s\n", __func__, route);
	pr_info("%s: body           = %s\n", __func__, body);

	// eXosip_send_message_request();
	
	// char tmp[4096];
	
	eXosip_message_build_request(eXosip_i->ctx, &message, "MESSAGE", to, from, NULL);
	// eXosip_message_build_request(eXosip_i->ctx, &message, "MESSAGE", to, from, route);
	// snprintf(tmp, 4096, "hellor rainfish");
	osip_message_set_body(message, body, strlen(body));
	osip_message_set_content_type(message, "text/xml");
	eXosip_message_send_request(eXosip_i->ctx, message);
	
	pr_debug("%s: <eXosip_send_message_request> \n", __func__);
	
	return dbus_message_new_method_return(msg);
}

static DBusMessage *method_eXosip_terminate(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
	uint32_t which;
	DBusMessage *message;
	
	if (dbus_message_get_args(msg, NULL, DBUS_TYPE_UINT32, &which,
						DBUS_TYPE_INVALID) == FALSE) {
		return btd_error_invalid_args(msg);
	}
		
	pr_info("%s: which = %d\n", __func__, which);

	// eXosip_send_terminate_request();
	eXosip_lock (eXosip_i->ctx);
 	eXosip_call_terminate(eXosip_i->ctx, eXosip_i->cid, eXosip_i->did);
	eXosip_unlock (eXosip_i->ctx);
	
	pr_debug("%s: <eXosip_send_terminate_request> capture \n", __func__);
	
	return dbus_message_new_method_return(msg);
}

static DBusMessage *method_eXosip_unregister(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
	int ret;
	uint32_t expires = 0;
	osip_message_t *reg = NULL;
	DBusMessage *message;
	
	if (dbus_message_get_args(msg, NULL, 
						DBUS_TYPE_UINT32, &expires,
						DBUS_TYPE_INVALID) == FALSE) {
		return btd_error_invalid_args(msg);
	}
		
	pr_info("%s: expires = %d\n", __func__, expires);
	
	eXosip_lock (eXosip_i->ctx);
	
	// expires is delay for n second to unregister (i.e expires = 0, immediately unregister)
	ret = eXosip_register_build_register(eXosip_i->ctx, eXosip_i->rid, expires, &reg);
	
	if(ret < 0) {
        pr_err("%s: unregister failed! <ret = %d>\n", __func__, ret);
    	return btd_error_invalid_args(msg);
    }

    ret = eXosip_register_send_register(eXosip_i->ctx, eXosip_i->rid, reg);
    pr_debug("<eXosip_i->rid = %d, ret = %d>\n", eXosip_i->rid, ret);
	
	eXosip_unlock(eXosip_i->ctx);
	
	pr_debug("%s: <eXosip_send_terminate_unregistert> capture \n", __func__);
	
	return dbus_message_new_method_return(msg);
}

static const GDBusMethodTable exosip_eXosip_methods[] = {
	{ GDBUS_METHOD("Register",
			GDBUS_ARGS({ "eXosip", "sssuss" }), NULL, method_eXosip_register) },
	{ GDBUS_METHOD("Invite",
			GDBUS_ARGS({ "eXosip", "sssssusus" }), NULL, method_eXosip_invite) },
	{ GDBUS_METHOD("Message",
			GDBUS_ARGS({ "eXosip", "ssss" }), NULL, method_eXosip_message) },
	{ GDBUS_METHOD("Terminate",
			GDBUS_ARGS({ "eXosip", "u" }), NULL, method_eXosip_terminate) },
	{ GDBUS_METHOD("Unregister",
			GDBUS_ARGS({ "eXosip", "u" }), NULL, method_eXosip_unregister) },
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

static const GDBusPropertyTable exosip_eXosip_properties[] = {
	{ "Switch1", "b", property_get_switch, property_set_switch },
	{ }
};

static int exosip_eXosip_init(void)
{
	DBusConnection *conn = btd_get_dbus_connection();

	/* Registering interface after querying properties */
	if (!g_dbus_register_interface(conn,
				       "/org/exosip",
				       "org.exosip.eXosip", exosip_eXosip_methods,
				       NULL, exosip_eXosip_properties, NULL, NULL)) {
				       
		pr_err("Failed to register " "/org/exosip \n");
	}	

	return 0;	
}
fn_initcall(exosip_eXosip_init);






const char *dbus_service_name(void);

bool exosip_eXosip_register(void)
{
 	DBusMessage *message;
 	char *service_name = (char *)dbus_service_name();
 	DBusConnection *connection = btd_get_dbus_connection();

 	uint32_t value = 8;
 	char *from = "sip:haakon@127.0.0.1:5061";
    char *proxy = "sip:127.0.0.1:5060";
    char *contact ="sip:haakon@127.0.0.1:5061";
    char *username = "haakon";
    char *password = "12345678";
    int ret, expires = 3600;
    
	int   proxyport = info_exosip_proxy_port_get();
	char *proxyipaddr = info_exosip_proxy_ipaddr_get();
	
	int fromport = info_exosip_from_port_get();
	char *fromipaddr = info_exosip_from_ipaddr_get();
	char *fromusername = info_exosip_local_username_get();
	char *frompassword = info_exosip_local_password_get();
 	
 	message = dbus_message_new_method_call(
 					service_name, 
 					"/org/exosip",
 					"org.exosip.eXosip", "Register");
 					
 	if (message == NULL) {
		return FALSE;
	}

	from = g_strdup_printf("sip:%s@%s:%d", fromusername, fromipaddr, fromport);
	proxy = g_strdup_printf("sip:%s:%d", proxyipaddr, proxyport);
	
	contact = from;
	expires = 3600;
	username = fromusername;
	password = frompassword;

	dbus_message_append_args(message, 
					DBUS_TYPE_STRING, &from,
					DBUS_TYPE_STRING, &proxy,
					DBUS_TYPE_STRING, &contact, 
					DBUS_TYPE_UINT32, &expires,
					DBUS_TYPE_STRING, &username,
					DBUS_TYPE_STRING, &password,
					DBUS_TYPE_INVALID);

	g_free(from);
	g_free(proxy);

 	return g_dbus_send_message(connection, message);
} 

bool exosip_eXosip_invite(void)
{
 	DBusMessage *message;
 	char *service_name = (char *)dbus_service_name();
 	DBusConnection *connection = btd_get_dbus_connection();

	char *to = "sip:david@127.0.0.1:5062";
	char *from = "sip:haakon@127.0.0.1:5061";
	char *route = "<sip:127.0.0.1:5060;lr>";
	char *subject = "This is a call subject";
	
	int   proxyport = info_exosip_proxy_port_get();
	char *proxyipaddr = info_exosip_proxy_ipaddr_get();
	
	int   toport = info_exosip_to_port_get();
	char *toipaddr = info_exosip_to_ipaddr_get();
	char *tousername = info_exosip_remote_username_get();
	
	int fromport = info_exosip_from_port_get();
	char *fromipaddr = info_exosip_from_ipaddr_get();
	char *fromusername = info_exosip_local_username_get();
	
	char *local_ipaddr = info_audio_local_ipaddr_get(); 	// "127.0.0.1";
	int   audio_lport = info_audio_local_port_get();		// 8000
	char *audio_lipaddr = info_audio_local_ipaddr_get(); 	// "127.0.0.1";
	int   video_lport = info_video_local_port_get();		// 6000
	char *video_lipaddr = info_video_local_ipaddr_get(); 	// "127.0.0.1";
 	
 	message = dbus_message_new_method_call(
 					service_name, 
 					"/org/exosip",
 					"org.exosip.eXosip", "Invite");
 					
 	if (message == NULL) {
		return FALSE;
	}
	
	to = g_strdup_printf("sip:%s@%s:%d", tousername, toipaddr, toport);
	from = g_strdup_printf("sip:%s@%s:%d", fromusername, fromipaddr, fromport);
	route = g_strdup_printf("<sip:%s:%d;lr>", proxyipaddr, proxyport);

	dbus_message_append_args(message, 
					DBUS_TYPE_STRING, &to,
					DBUS_TYPE_STRING, &from,
					DBUS_TYPE_STRING, &route,
					DBUS_TYPE_STRING, &subject,
					DBUS_TYPE_STRING, &local_ipaddr,
					DBUS_TYPE_UINT32, &audio_lport,
					DBUS_TYPE_STRING, &audio_lipaddr,
					DBUS_TYPE_UINT32, &video_lport,
					DBUS_TYPE_STRING, &video_lipaddr,
					DBUS_TYPE_INVALID);

	g_free(to);
	g_free(from);
	g_free(route);

 	return g_dbus_send_message(connection, message);
} 


bool exosip_eXosip_message(void)
{
 	DBusMessage *message;
 	char *service_name = (char *)dbus_service_name();
 	DBusConnection *connection = btd_get_dbus_connection();

	char *to = "sip:david@127.0.0.1:5062";
	char *from = "sip:haakon@127.0.0.1:5061";
	char *route = "<sip:127.0.0.1:5060;lr>";
	char *body = "This is a message body";
	
	int   proxyport = info_exosip_proxy_port_get();
	char *proxyipaddr = info_exosip_proxy_ipaddr_get();
	
	int   toport = info_exosip_to_port_get();
	char *toipaddr = info_exosip_to_ipaddr_get();
	char *tousername = info_exosip_remote_username_get();
	
	int fromport = info_exosip_from_port_get();
	char *fromipaddr = info_exosip_from_ipaddr_get();
	char *fromusername = info_exosip_local_username_get();
 	
 	message = dbus_message_new_method_call(
 					service_name, 
 					"/org/exosip",
 					"org.exosip.eXosip", "Message");
 					
 	if (message == NULL) {
		return FALSE;
	}
	
	to = g_strdup_printf("sip:%s@%s:%d", tousername, toipaddr, toport);
	from = g_strdup_printf("sip:%s@%s:%d", fromusername, fromipaddr, fromport);
	route = g_strdup_printf("<sip:%s:%d;lr>", proxyipaddr, proxyport);

	dbus_message_append_args(message, 
					DBUS_TYPE_STRING, &to,
					DBUS_TYPE_STRING, &from,
					DBUS_TYPE_STRING, &route,
					DBUS_TYPE_STRING, &body,
					DBUS_TYPE_INVALID);
	g_free(to);
	g_free(from);
	g_free(route);

 	return g_dbus_send_message(connection, message);
}

bool exosip_eXosip_terminate(void)
{
 	DBusMessage *message;
 	char *service_name = (char *)dbus_service_name();
 	DBusConnection *connection = btd_get_dbus_connection();

 	uint32_t value = 8;
 	
 	message = dbus_message_new_method_call(
 					service_name, 
 					"/org/exosip",
 					"org.exosip.eXosip", "Terminate");
 					
 	if (message == NULL) {
		return FALSE;
	}

	dbus_message_append_args(message, 
							DBUS_TYPE_UINT32, &value, 
							DBUS_TYPE_INVALID);

 	return g_dbus_send_message(connection, message);
}


bool exosip_eXosip_unregister(void)
{
 	DBusMessage *message;
 	char *service_name = (char *)dbus_service_name();
 	DBusConnection *connection = btd_get_dbus_connection();

	// (i.e expires = 0, immediately unregister)
 	uint32_t expires = 0;
 	
 	message = dbus_message_new_method_call(
 					service_name, 
 					"/org/exosip",
 					"org.exosip.eXosip", "Unregister");
 					
 	if (message == NULL) {
		return FALSE;
	}

	dbus_message_append_args(message, 
							DBUS_TYPE_UINT32, &expires, 
							DBUS_TYPE_INVALID);

 	return g_dbus_send_message(connection, message);
}



static gboolean cancel_fire_7(gpointer data){
  g_print("cancel_fire_7() quit \n");

	// eXosip_send_register_request();
	exosip_eXosip_register();

  return G_SOURCE_REMOVE;
}

static gboolean cancel_fire_8(gpointer data){
  g_print("cancel_fire_8() quit \n");

  // eXosip_send_invite_request();
  // exosip_eXosip_invite();

  return G_SOURCE_REMOVE;
}

static gboolean cancel_fire_9(gpointer data){
  g_print("cancel_fire_9() quit \n");

	// eXosip_send_message_request();
	// exosip_eXosip_message();

  return G_SOURCE_REMOVE;
}

static gboolean cancel_fire_a(gpointer data){
  g_print("cancel_fire_a() quit \n");

	// eXosip_send_terminate_request();
	// exosip_eXosip_terminate();

  return G_SOURCE_REMOVE;
}

static gboolean cancel_fire_b(gpointer data){
  g_print("cancel_fire_b() quit \n");

	// eXosip_send_unregister_request();
	// exosip_eXosip_unregister();

  return G_SOURCE_REMOVE;
}

static int main_init(void)
{
	printf("-----main_init------ \n");

	g_timeout_add(1000, cancel_fire_7, NULL);
	/* 
	g_timeout_add(3000, cancel_fire_8, NULL);
	g_timeout_add(5000, cancel_fire_9, NULL);
	g_timeout_add(8000, cancel_fire_a, NULL);
	g_timeout_add(15000, cancel_fire_b, NULL);
	*/
	return 0;
}
fn_initcall(main_init);


// exosip start event
static DBusMessage *method_eXosip_call_ack(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
	uint32_t which;
	DBusMessage *message;
	
	if (dbus_message_get_args(msg, NULL, DBUS_TYPE_UINT32, &which,
						DBUS_TYPE_INVALID) == FALSE) {
		return btd_error_invalid_args(msg);
	}
		
	pr_info("%s: which = %d\n", __func__, which);
	
	ortp_audio_start_i();
	// emulate_ortp_audio_start_i();
	
	pr_debug("%s: <eXosip_send_terminate_request> capture \n", __func__);
	
	return dbus_message_new_method_return(msg);
}

// exosip start event
static DBusMessage *method_eXosip_call_answered(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
	int ret;
	uint32_t expires = 0;
	osip_message_t *reg = NULL;
	DBusMessage *message;
	
	if (dbus_message_get_args(msg, NULL, 
						DBUS_TYPE_UINT32, &expires,
						DBUS_TYPE_INVALID) == FALSE) {
		return btd_error_invalid_args(msg);
	}
		
	pr_info("%s: expires = %d\n", __func__, expires);
	
	ortp_audio_start_i();
	// emulate_ortp_audio_start_i();
	
	pr_debug("%s: <eXosip_send_terminate_unregistert> capture \n", __func__);
	
	return dbus_message_new_method_return(msg);
}

// exosip finish event
static DBusMessage *method_eXosip_call_closed(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
	uint32_t which;
	DBusMessage *message;
	
	if (dbus_message_get_args(msg, NULL, DBUS_TYPE_UINT32, &which,
						DBUS_TYPE_INVALID) == FALSE) {
		return btd_error_invalid_args(msg);
	}
		
	pr_info("%s: which = %d\n", __func__, which);
	
	ortp_audio_stop_i();
	// emulate_ortp_audio_stop_i();
	
	pr_debug("%s: <eXosip_send_terminate_request> capture \n", __func__);
	
	return dbus_message_new_method_return(msg);
}

// exosip finish event
static DBusMessage *method_eXosip_call_message_answered(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
	uint32_t which;
	DBusMessage *message;
	
	if (dbus_message_get_args(msg, NULL, DBUS_TYPE_UINT32, &which,
						DBUS_TYPE_INVALID) == FALSE) {
		return btd_error_invalid_args(msg);
	}
		
	pr_info("%s: which = %d\n", __func__, which);
	
	ortp_audio_stop_i();
	// emulate_ortp_audio_stop_i();
	
	pr_debug("%s: <eXosip_send_terminate_request> capture \n", __func__);
	
	return dbus_message_new_method_return(msg);
}

static const GDBusMethodTable exosip_event_methods[] = {
			
	{ GDBUS_METHOD("EXOSIP_CALL_ACK",
			GDBUS_ARGS({ "eXosip_start", "u" }), NULL, method_eXosip_call_ack) },
	{ GDBUS_METHOD("EXOSIP_CALL_ANSWERED",
			GDBUS_ARGS({ "eXosip_start", "u" }), NULL, method_eXosip_call_answered) },
			
	{ GDBUS_METHOD("EXOSIP_CALL_CLOSED",
			GDBUS_ARGS({ "eXosip_finish", "u" }), NULL, method_eXosip_call_closed) },
	{ GDBUS_METHOD("EXOSIP_CALL_MESSAGE_ANSWERED",
			GDBUS_ARGS({ "eXosip_finish", "u" }), NULL, method_eXosip_call_message_answered) },

	{ }
};

static gboolean property_event_get_switch(const GDBusPropertyTable *property,
					DBusMessageIter *iter, void *user_data)
{
	dbus_bool_t enable = TRUE; // FALSE
	
	dbus_message_iter_append_basic(iter, DBUS_TYPE_BOOLEAN, &enable);
	
	pr_info("%s: enable = %d\n", __func__, enable);

	return TRUE;
}

static void property_event_set_switch(const GDBusPropertyTable *property,
				DBusMessageIter *iter,
				GDBusPendingPropertySet id, void *user_data)
{
	dbus_bool_t enable;

	dbus_message_iter_get_basic(iter, &enable);
	
	pr_info("%s: id = %d, enable = %d\n", __func__, id, enable);

	g_dbus_pending_property_success(id);
}

static const GDBusPropertyTable exosip_event_properties[] = {
	{ "Switch1", "b", property_event_get_switch, property_event_set_switch },
	{ }
};

static int exosip_event_init(void)
{
	DBusConnection *conn = btd_get_dbus_connection();

	/* Registering interface after querying properties */
	if (!g_dbus_register_interface(conn,
				       "/org/exosip",
				       "org.exosip.event", exosip_event_methods,
				       NULL, exosip_event_properties, NULL, NULL)) {
				       
		pr_err("Failed to register " "/org/exosip \n");
	}	

	return 0;	
}
fn_initcall(exosip_event_init);


bool exosip_call_ack_event(void)
{
 	DBusMessage *message;
 	char *service_name = (char *)dbus_service_name();
 	DBusConnection *connection = btd_get_dbus_connection();

 	uint32_t value = 8;
 	
 	message = dbus_message_new_method_call(
 					service_name, 
 					"/org/exosip",
 					"org.exosip.event", "EXOSIP_CALL_ACK");
 					
 	if (message == NULL) {
		return FALSE;
	}

	dbus_message_append_args(message, 
							DBUS_TYPE_UINT32, &value, 
							DBUS_TYPE_INVALID);

 	return g_dbus_send_message(connection, message);
}

bool exosip_call_answered_event(void)
{
 	DBusMessage *message;
 	char *service_name = (char *)dbus_service_name();
 	DBusConnection *connection = btd_get_dbus_connection();

 	uint32_t value = 8;
 	
 	message = dbus_message_new_method_call(
 					service_name, 
 					"/org/exosip",
 					"org.exosip.event", "EXOSIP_CALL_ANSWERED");
 					
 	if (message == NULL) {
		return FALSE;
	}

	dbus_message_append_args(message, 
							DBUS_TYPE_UINT32, &value, 
							DBUS_TYPE_INVALID);

 	return g_dbus_send_message(connection, message);
}


bool exosip_call_closed_event(void)
{
 	DBusMessage *message;
 	char *service_name = (char *)dbus_service_name();
 	DBusConnection *connection = btd_get_dbus_connection();

 	uint32_t value = 8;
 	
 	message = dbus_message_new_method_call(
 					service_name, 
 					"/org/exosip",
 					"org.exosip.event", "EXOSIP_CALL_CLOSED");
 					
 	if (message == NULL) {
		return FALSE;
	}

	dbus_message_append_args(message, 
							DBUS_TYPE_UINT32, &value, 
							DBUS_TYPE_INVALID);

 	return g_dbus_send_message(connection, message);
}

bool exosip_call_message_answered_event(void)
{
 	DBusMessage *message;
 	char *service_name = (char *)dbus_service_name();
 	DBusConnection *connection = btd_get_dbus_connection();

 	uint32_t value = 8;
 	
 	message = dbus_message_new_method_call(
 					service_name, 
 					"/org/exosip",
 					"org.exosip.event", "EXOSIP_CALL_MESSAGE_ANSWERED");
 					
 	if (message == NULL) {
		return FALSE;
	}

	dbus_message_append_args(message, 
							DBUS_TYPE_UINT32, &value, 
							DBUS_TYPE_INVALID);

 	return g_dbus_send_message(connection, message);
}







