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

/////////////////////////////////////////////////////////////////

struct h264_hdr {
	uint8_t prefix[4];
	union {
		struct {
			uint8_t nal_unit_type : 5;
			uint8_t nal_ref_bit   : 2;
			uint8_t forbidden_bit : 1;
		};
		uint8_t header;
	};
	uint8_t data[0];
};

struct rtp_h264_hdr {
	uint8_t nal_unit_type : 5;
	uint8_t nal_ref_bit   : 2;
	uint8_t forbidden_bit : 1;
	uint8_t data[0];
};

struct rtp_fua_hdr {
	struct {
		uint8_t rtp_h264_type : 5;
		uint8_t nal_ref_bit   : 2;
		uint8_t forbidden_bit : 1;
	} ind;
	
	struct {
		uint8_t nal_unit_type : 5;
		uint8_t reserve   : 1;
		uint8_t end       : 1;
		uint8_t start     : 1;
	} hdr;
	uint8_t data[0];
};

struct rtp_fub_hdr {
	struct rtp_fua_hdr fua_hdr;
	uint16_t don;
	uint8_t data[0];
};

#define H264_NALU_TYPE_IPB	1
#define H264_NALU_TYPE_DPA	2
#define H264_NALU_TYPE_DPB	3
#define H264_NALU_TYPE_DPC	4
#define H264_NALU_TYPE_IDR	5
#define H264_NALU_TYPE_SEI	6
#define H264_NALU_TYPE_SPS	7
#define H264_NALU_TYPE_PPS	8


#define RTP_MAX_MTU 		1400

uint8_t *rtp_fua_in;
uint8_t *rtp_fua_out;

uint8_t h264_packet[2048];
int  h264_packet_remain = 0;
int  h264_packet_num = 0;
int  h264_packet_len = 0;
int  h264_packet_total = 0;

char h264_pps_pkt[8] = {0x00, 0x00, 0x00, 0x01, 0x68, 0xCE, 0x3C, 0x80};
char h264_sps_pkt[19] = {0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0xC0, 0x1F, 0x8C, 0x8D, 0x40, 0x48, 0x14, 0xB2, 0xF0, 0x0F, 0x08, 0x84, 0x6A};

/////////////////////////////////////////////////////////////////

int runcond = 1;

void stophandler(int signum) {
	runcond = 0;
}

static const char *help =
    "usage: rtpsend	filename dest_ip4addr dest_port [ --with-clockslide <value> ] [ --with-jitter <milliseconds>]\n";

int main(int argc, char *argv[]) {
	RtpSession *session;
	unsigned char buffer[160];
	int i, k, len;
	FILE *infile;
	char *ssrc;
	uint32_t user_ts = 0;
	int clockslide = 0;
	int jitter = 0;
	
	int lport = 8000;
	char *lipaddr = "127.0.0.1";

	int rport = 12222;
	char *ripaddr = "127.0.0.1";

	ortp_init();
	ortp_scheduler_init();
	ortp_set_log_level_mask(NULL, ORTP_MESSAGE | ORTP_WARNING | ORTP_ERROR);
	session = rtp_session_new(RTP_SESSION_SENDONLY);

	rtp_session_set_scheduling_mode(session, 1);
	rtp_session_set_blocking_mode(session, 1);
	rtp_session_set_connected_mode(session, TRUE);

	// rtp_session_set_remote_addr(session, ipaddr, ipport);
	// rtp_session_set_remote_addr(session, argv[2], atoi(argv[3]));
	
	rtp_session_set_local_addr(session, lipaddr, lport, lport + 1);
	rtp_session_set_remote_addr(session, ripaddr, rport);
	
	// rtp_session_set_payload_type(session, 0);
	rtp_profile_set_payload(&av_profile, 96, &payload_type_h264);
	rtp_session_set_payload_type(session, 96);

	ssrc = getenv("SSRC");
	if (ssrc != NULL) {
		printf("using SSRC=%i.\n", atoi(ssrc));
		rtp_session_set_ssrc(session, atoi(ssrc));
	}

	signal(SIGINT, stophandler);


	printf("sizeof(struct h264_hdr)     = %ld\n", sizeof(struct h264_hdr));
	printf("sizeof(struct rtp_h264_hdr) = %ld\n", sizeof(struct rtp_h264_hdr));
	printf("sizeof(struct rtp_fua_hdr)  = %ld\n", sizeof(struct rtp_fua_hdr));
	printf("sizeof(struct rtp_fub_hdr)  = %ld\n", sizeof(struct rtp_fub_hdr));
	
	int wbyte = 0;
	int framerate = 25;
	int timestamp = 90000 / 25;

	//while (0) 
	{
		
		struct h264_hdr *h264_hdr = (struct h264_hdr *)h264_sps_pkt;
		struct rtp_h264_hdr *rtp_h264_hdr = (struct rtp_h264_hdr *)h264_packet;
		struct rtp_fua_hdr *rtp_fua_hdr   = (struct rtp_fua_hdr *)h264_packet;
		
		rtp_fua_in  = (uint8_t *)h264_hdr->data;
		rtp_fua_out = (uint8_t *)rtp_fua_hdr->data;
	
		len = sizeof(h264_sps_pkt);
		
		printf("%s: h264_hdr->header   = %02x\n", __func__, h264_hdr->header);
		printf("%s: h264_hdr->forbidden_bit = %d\n", __func__, h264_hdr->forbidden_bit);
		printf("%s: h264_hdr->nal_ref_bit   = %d\n", __func__, h264_hdr->nal_ref_bit);
		printf("%s: h264_hdr->nal_unit_type = %d\n", __func__, h264_hdr->nal_unit_type);
		
		h264_packet_len = len - sizeof(struct h264_hdr);
		
		if (len < RTP_MAX_MTU) {
			// rtp h264 header part fill
			rtp_h264_hdr->forbidden_bit = h264_hdr->forbidden_bit;
			rtp_h264_hdr->nal_ref_bit = h264_hdr->nal_ref_bit;
			rtp_h264_hdr->nal_unit_type = h264_hdr->nal_unit_type;
			
			user_ts += timestamp;
		
			// rtp h264 data part fill
			for (i = 0; i < h264_packet_len; i++) {
				rtp_h264_hdr->data[i] = h264_hdr->data[i];
			}
			
			h264_packet_total = h264_packet_len + sizeof(struct rtp_h264_hdr);
			
			// rtp_h264_packet = rtp_h264_hdr + rtp_h264_data
			wbyte = rtp_session_send_with_ts(session, h264_packet, h264_packet_total, user_ts);
			
			// //如果是6,7,8类型的包，不应该延时；之前有停顿，原因这在这
		
			// display packet data
			for (i = 0; i < h264_packet_len; i++) {
				printf("0x%02x ", rtp_h264_hdr->data[i]);
			}
			printf("\n");
			
			for (i = 0; i < h264_packet_len; i++) {
				printf("0x%02x ", h264_hdr->data[i]);
			}
			printf("\n");
		} else {
			h264_packet_num = h264_packet_len / RTP_MAX_MTU;
			h264_packet_remain = h264_packet_len % RTP_MAX_MTU;
			
			if (h264_packet_remain > 0) {
				h264_packet_num += 1;
			}
			
			user_ts += timestamp;
			
			for (k = 0; k < h264_packet_num; k++) {
				h264_packet_total = RTP_MAX_MTU + sizeof(struct rtp_fua_hdr);
				
				// rtp h264 fu_a indicator first byte
				rtp_fua_hdr->ind.forbidden_bit = h264_hdr->forbidden_bit;
				rtp_fua_hdr->ind.nal_ref_bit   = h264_hdr->nal_ref_bit;
				rtp_fua_hdr->ind.rtp_h264_type = 28;
				
				// rtp h264 fu_a header second byte
				rtp_fua_hdr->hdr.start   = 0;
				rtp_fua_hdr->hdr.reserve = 0;
				rtp_fua_hdr->hdr.end     = 0;
				rtp_fua_hdr->hdr.nal_unit_type = h264_hdr->nal_unit_type;
			
				if (i == 0) {
					// the first fua unit packet
					rtp_fua_hdr->hdr.start   = 1;
				}
			
				if (i == h264_packet_num - 1) {
					// the last fua unit packet
					rtp_fua_hdr->hdr.end     = 1;
					
					if (h264_packet_remain > 0) {
						h264_packet_total = h264_packet_remain + sizeof(struct rtp_fua_hdr);
					}
				}
				
				for (i = 0; i < h264_packet_len; i++) {
					rtp_fua_hdr->data[i] = *rtp_fua_in;
					rtp_fua_in++;
					// rtp_fua_hdr->data[i] = h264_hdr->data[i];
				}
				
				wbyte = rtp_session_send_with_ts(session, h264_packet, h264_packet_total, user_ts);
			}
		}
		
		if (h264_hdr->nal_unit_type == H264_NALU_TYPE_IPB || h264_hdr->nal_unit_type == H264_NALU_TYPE_IDR) {
			usleep(40 * 1000);
		}
	}

runcond = 0;
while (runcond) {
	for (i = 0 ; i < 160; i++) {
		buffer[i] = 'A';
	}
	
	len = rtp_session_send_with_ts(session, buffer, i, user_ts);
	
	user_ts += 160;

	ortp_message("%s: i = %d, len = %d, user_ts = %d", __func__, i, len, user_ts);

	//ortp_message("Clock sliding of %i miliseconds now",clockslide);
	//rtp_session_make_time_distorsion(session,clockslide);
	
	sleep(3);
}	

	rtp_session_destroy(session);
	ortp_exit();
//	ortp_global_stats_display();

	return 0;
}






#if 0
int main(int argc, char *argv[]) {
	RtpSession *session;
	unsigned char buffer[160];
	int i, k, len;
	FILE *infile;
	char *ssrc;
	uint32_t user_ts = 0;
	int clockslide = 0;
	int jitter = 0;
	
	int lport = 8000;
	char *lipaddr = "127.0.0.1";

	int rport = 12222;
	char *ripaddr = "127.0.0.1";

	ortp_init();
	ortp_scheduler_init();
	ortp_set_log_level_mask(NULL, ORTP_MESSAGE | ORTP_WARNING | ORTP_ERROR);
	session = rtp_session_new(RTP_SESSION_SENDONLY);

	rtp_session_set_scheduling_mode(session, 1);
	rtp_session_set_blocking_mode(session, 1);
	rtp_session_set_connected_mode(session, TRUE);

	// rtp_session_set_remote_addr(session, ipaddr, ipport);
	// rtp_session_set_remote_addr(session, argv[2], atoi(argv[3]));
	
	rtp_session_set_local_addr(session, lipaddr, lport, lport + 1);
	rtp_session_set_remote_addr(session, ripaddr, rport);
	
	// rtp_session_set_payload_type(session, 0);
	rtp_profile_set_payload(&av_profile, 96, &payload_type_h264);
	rtp_session_set_payload_type(session, 96);

	ssrc = getenv("SSRC");
	if (ssrc != NULL) {
		printf("using SSRC=%i.\n", atoi(ssrc));
		rtp_session_set_ssrc(session, atoi(ssrc));
	}

	signal(SIGINT, stophandler);


	printf("sizeof(struct h264_hdr)     = %ld\n", sizeof(struct h264_hdr));
	printf("sizeof(struct rtp_h264_hdr) = %ld\n", sizeof(struct rtp_h264_hdr));
	printf("sizeof(struct rtp_fua_hdr)  = %ld\n", sizeof(struct rtp_fua_hdr));
	printf("sizeof(struct rtp_fub_hdr)  = %ld\n", sizeof(struct rtp_fub_hdr));
	
	int wbyte = 0;
	int framerate = 25;
	int timestamp = 90000 / 25;

	//while (0) 
	{
		struct h264_hdr *h264_hdr = (struct h264_hdr *)h264_sps_pkt;
		struct rtp_h264_hdr *rtp_h264_hdr = (struct rtp_h264_hdr *)h264_packet;
		struct rtp_fua_hdr *rtp_fua_hdr   = (struct rtp_fua_hdr *)h264_packet;
	
		len = sizeof(h264_sps_pkt);
		
		printf("%s: h264_hdr->header   = %02x\n", __func__, h264_hdr->header);
		printf("%s: h264_hdr->forbidden_bit = %d\n", __func__, h264_hdr->forbidden_bit);
		printf("%s: h264_hdr->nal_ref_bit   = %d\n", __func__, h264_hdr->nal_ref_bit);
		printf("%s: h264_hdr->nal_unit_type = %d\n", __func__, h264_hdr->nal_unit_type);
		
		h264_packet_len = len - sizeof(struct h264_hdr);
		
		if (len < RTP_MAX_MTU) {
			// rtp h264 header part fill
			rtp_h264_hdr->forbidden_bit = h264_hdr->forbidden_bit;
			rtp_h264_hdr->nal_ref_bit = h264_hdr->nal_ref_bit;
			rtp_h264_hdr->nal_unit_type = h264_hdr->nal_unit_type;
			
			user_ts += timestamp;
		
			// rtp h264 data part fill
			for (i = 0; i < h264_packet_len; i++) {
				rtp_h264_hdr->data[i] = h264_hdr->data[i];
			}
			
			// rtp_h264_packet = rtp_h264_hdr + rtp_h264_data
			wbyte = rtp_session_send_with_ts(session, h264_packet, h264_packet_len + 1, user_ts);
			
			// //如果是6,7,8类型的包，不应该延时；之前有停顿，原因这在这
		
			// display packet data
			for (i = 0; i < h264_packet_len; i++) {
				printf("0x%02x ", rtp_h264_hdr->data[i]);
			}
			printf("\n");
			
			for (i = 0; i < h264_packet_len; i++) {
				printf("0x%02x ", h264_hdr->data[i]);
			}
			printf("\n");
		} else {
			h264_packet_num = h264_packet_len / RTP_MAX_MTU;
			h264_packet_remain = h264_packet_len % RTP_MAX_MTU;
			
			if (h264_packet_remain > 0) {
				h264_packet_num += 1;
			}
			
			// rtp h264 fu_a indicator first byte
			rtp_fua_hdr->ind.forbidden_bit = h264_hdr->forbidden_bit;
			rtp_fua_hdr->ind.nal_ref_bit   = h264_hdr->nal_ref_bit;
			rtp_fua_hdr->ind.rtp_h264_type = 28;
			
			// rtp h264 fu_a header second byte
			rtp_fua_hdr->hdr.start   = 1;
			rtp_fua_hdr->hdr.reserve = 0;
			rtp_fua_hdr->hdr.end     = 0;
			rtp_fua_hdr->hdr.nal_unit_type = h264_hdr->nal_unit_type;
			
			user_ts += timestamp;
			
			for (i = 0; i < h264_packet_len; i++) {
				rtp_fua_hdr->data[i] = h264_hdr->data[i];
			}
			wbyte = rtp_session_send_with_ts(session, h264_packet, RTP_MAX_MTU + 2, user_ts);
			
			for (k = 1; k < h264_packet_num - 1; k++) {
				// rtp h264 fu_a indicator first byte
				rtp_fua_hdr->ind.forbidden_bit = h264_hdr->forbidden_bit;
				rtp_fua_hdr->ind.nal_ref_bit   = h264_hdr->nal_ref_bit;
				rtp_fua_hdr->ind.rtp_h264_type = 28;
				
				// rtp h264 fu_a header second byte
				rtp_fua_hdr->hdr.start   = 0;
				rtp_fua_hdr->hdr.reserve = 0;
				rtp_fua_hdr->hdr.end     = 0;
				rtp_fua_hdr->hdr.nal_unit_type = h264_hdr->nal_unit_type;
				
				for (i = 0; i < h264_packet_len; i++) {
					rtp_fua_hdr->data[i] = h264_hdr->data[i];
				}
				wbyte = rtp_session_send_with_ts(session, h264_packet, RTP_MAX_MTU + 2, user_ts);
			}
			
			// rtp h264 fu_a indicator first byte
			rtp_fua_hdr->ind.forbidden_bit = h264_hdr->forbidden_bit;
			rtp_fua_hdr->ind.nal_ref_bit   = h264_hdr->nal_ref_bit;
			rtp_fua_hdr->ind.rtp_h264_type = 28;
			
			// rtp h264 fu_a header second byte
			rtp_fua_hdr->hdr.start   = 0;
			rtp_fua_hdr->hdr.reserve = 0;
			rtp_fua_hdr->hdr.end     = 1;
			rtp_fua_hdr->hdr.nal_unit_type = h264_hdr->nal_unit_type;
			
			for (i = 0; i < h264_packet_remain; i++) {
				rtp_fua_hdr->data[i] = h264_hdr->data[i];
			}
			wbyte = rtp_session_send_with_ts(session, h264_packet, h264_packet_remain + 2, user_ts);
		}
		
		if (h264_hdr->nal_unit_type == H264_NALU_TYPE_IPB || h264_hdr->nal_unit_type == H264_NALU_TYPE_IDR) {
			usleep(40 * 1000);
		}
	}

runcond = 0;
while (runcond) {
	for (i = 0 ; i < 160; i++) {
		buffer[i] = 'A';
	}
	
	len = rtp_session_send_with_ts(session, buffer, i, user_ts);
	
	user_ts += 160;

	ortp_message("%s: i = %d, len = %d, user_ts = %d", __func__, i, len, user_ts);

	//ortp_message("Clock sliding of %i miliseconds now",clockslide);
	//rtp_session_make_time_distorsion(session,clockslide);
	
	sleep(3);
}	

	rtp_session_destroy(session);
	ortp_exit();
//	ortp_global_stats_display();

	return 0;
}
#endif

















