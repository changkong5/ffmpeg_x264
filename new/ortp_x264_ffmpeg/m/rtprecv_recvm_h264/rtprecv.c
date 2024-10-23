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

#include <bctoolbox/vfs.h>
#include <ortp/ortp.h>
#include <signal.h>
#include <stdlib.h>
#ifndef _WIN32
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#endif

/////////////////////////////////////////////////////////////////

// h264 nalu start code is [0x00 0x00 0x01] or [0x00 0x00 0x00 0x01]
struct h264_hdr {
	// uint8_t prefix[4];
	uint8_t prefix[3];
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

// uint8_t *rtp_fua_in;
// uint8_t *rtp_fua_out;

uint8_t h264_packet[4096];
int  h264_packet_remain = 0;
int  h264_packet_num = 0;
int  h264_packet_len = 0;
int  h264_packet_total = 0;

// char h264_pps_pkt[8] = {0x00, 0x00, 0x00, 0x01, 0x68, 0xCE, 0x3C, 0x80};
// char h264_sps_pkt[19] = {0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0xC0, 0x1F, 0x8C, 0x8D, 0x40, 0x48, 0x14, 0xB2, 0xF0, 0x0F, 0x08, 0x84, 0x6A};

char rtp_h264_pps_pkt[4] = {0x68, 0xCE, 0x3C, 0x80};
char rtp_h264_sps_pkt[15] = {0x67, 0x42, 0xC0, 0x1F, 0x8C, 0x8D, 0x40, 0x48, 0x14, 0xB2, 0xF0, 0x0F, 0x08, 0x84, 0x6A};
/////////////////////////////////////////////////////////////////

FILE *out_file = NULL;

void h264_file_frame_write(uint8_t *packet, int packet_len)
{
	static int count = 0;
	char name[64];
	FILE *file;
	sprintf(name, "yuv420p_w352xh288-%d.h264", count++);
	
	file = fopen(name, "wb+");
	
	fwrite(packet, 1, packet_len, file);
	fflush(file);
	
	fclose(file);
}

int ortp_h264_nalu_decode(uint8_t *nalu, int len)
{
	int i;
	static uint8_t *rtp_fua_out = NULL;
	struct h264_hdr *h264_hdr = (struct h264_hdr *)h264_packet;
	struct rtp_h264_hdr *rtp_h264_hdr = (struct rtp_h264_hdr *)nalu;
	struct rtp_fua_hdr *rtp_fua_hdr   = (struct rtp_fua_hdr *)nalu;
		
	// rtp_fua_in  = (uint8_t *)rtp_fua_hdr->data;
	// rtp_fua_out = (uint8_t *)h264_hdr->data;
		
	// len = sizeof(rtp_h264_sps_pkt);
		
	printf("%s: rtp_h264_hdr->forbidden_bit = %d\n", __func__, rtp_h264_hdr->forbidden_bit);
	printf("%s: rtp_h264_hdr->nal_ref_bit   = %d\n", __func__, rtp_h264_hdr->nal_ref_bit);
	printf("%s: rtp_h264_hdr->nal_unit_type = %d\n", __func__, rtp_h264_hdr->nal_unit_type);
	
	if (rtp_h264_hdr->nal_unit_type < 24)  {
		// h264 start code fill
		h264_hdr->prefix[0] = 0x00;
		h264_hdr->prefix[1] = 0x00;
		h264_hdr->prefix[2] = 0x01;
		
		// h264 header fill
		h264_hdr->forbidden_bit = rtp_h264_hdr->forbidden_bit;
		h264_hdr->nal_ref_bit   = rtp_h264_hdr->nal_ref_bit;
		h264_hdr->nal_unit_type = rtp_h264_hdr->nal_unit_type;
			
		h264_packet_len = len - sizeof(struct rtp_h264_hdr);
			
		for (i = 0; i < h264_packet_len; i++) {
			h264_hdr->data[i] = rtp_h264_hdr->data[i];
		}
			
		h264_packet_total = h264_packet_len + sizeof(struct h264_hdr);
		printf("%s: <1>len = %d, h264_packet_len = %d, h264_packet_total = %d\n", __func__, len, h264_packet_len, h264_packet_total);
		
		fwrite(h264_packet, 1, h264_packet_total, out_file);
		fflush(out_file);
		// h264_file_frame_write(h264_packet, h264_packet_total);
			
		for (i = 0; i < h264_packet_total; i++) {
			// printf("0x%02x ", h264_packet[i]);
		}
		printf("\n");
	} else if (rtp_h264_hdr->nal_unit_type == 28) {
		if (rtp_fua_hdr->hdr.start) {
			// the first rtp fua unit packet
			
			// h264 start code fill
			h264_hdr->prefix[0] = 0x00;
			h264_hdr->prefix[1] = 0x00;
			h264_hdr->prefix[2] = 0x01;
				
			// h264 header fill
			h264_hdr->forbidden_bit = rtp_fua_hdr->ind.forbidden_bit;
			h264_hdr->nal_ref_bit   = rtp_fua_hdr->ind.nal_ref_bit;
			h264_hdr->nal_unit_type = rtp_fua_hdr->hdr.nal_unit_type;
			
			rtp_fua_out = (uint8_t *)h264_hdr->data;
			h264_packet_total = sizeof(struct h264_hdr);
		}

		h264_packet_len = len - sizeof(struct rtp_fua_hdr);
			
		for (i = 0; i < h264_packet_len; i++) {
			*rtp_fua_out = rtp_fua_hdr->data[i];
			rtp_fua_out++;
			// h264_hdr->data[i] = rtp_fua_hdr->data[i];
		}
		h264_packet_total += h264_packet_len;
		printf("%s: <2>len = %d, h264_packet_len = %d, h264_packet_total = %d\n", __func__, len, h264_packet_len, h264_packet_total);
		

		if (rtp_fua_hdr->hdr.end) {
			// the last rtp fua unit packet
			rtp_fua_out = NULL;
			
			printf("%s: <3>len = %d, h264_packet_len = %d, h264_packet_total = %d\n", __func__, len, h264_packet_len, h264_packet_total);
			fwrite(h264_packet, 1, h264_packet_total, out_file);
			fflush(out_file);
			// h264_file_frame_write(h264_packet, h264_packet_total);
		}
	} else {
		
	}

	return 0;
}

/////////////////////////////////////////////////////////////////


// void rtp_session_set_recv_buf_size(RtpSession *session, int bufsize)
// void rtp_session_set_rtp_socket_send_buffer_size(RtpSession *session, unsigned int size)
// void rtp_session_set_rtp_socket_recv_buffer_size(RtpSession *session, unsigned int size) 

// ffplay -f rawvideo -s 352x288 yuv420p_w352xh288.yuv 																// correct   // play yuv
// ffplay yuv420p_w352xh288.h264 																					// correct   // play h264
// ffmpeg -s 352x288 -i yuv420p_w352xh288.yuv -c:v h264 yuv420p_w352xh288.h264   									// correct	 // yuv ---> h264
// ffmpeg -c:v h264 -i yuv420p_w352xh288.h264 yuv420p_w352xh288.yuv    												// correct   // h264 ---> yuv

int cond = 1;

void stop_handler(int signum) {
	cond = 0;
}

void ssrc_cb(RtpSession *session) {
	printf("hey, the ssrc has changed !\n");
}

int main(int argc, char *argv[]) {
	RtpSession *session;
	unsigned char buffer[4096];
	int err, rbyte;
	uint32_t ts = 0;
	int stream_received = 0;
	FILE *outfile;
	int local_port;
	int have_more;
	int i, len;
	int format = 0;
	int soundcard = 0;
	int sound_fd = 0;
	int jittcomp = 40;
	bool_t adapt = TRUE;
	
	int lport = 12222;
	char *lipaddr = "127.0.0.1";

	int rport = 8000;
	char *ripaddr = "127.0.0.1";

	ortp_init();
	ortp_scheduler_init();
	ortp_set_log_level_mask(NULL, ORTP_DEBUG | ORTP_MESSAGE | ORTP_WARNING | ORTP_ERROR);
	// signal(SIGINT, stop_handler);
	session = rtp_session_new(RTP_SESSION_RECVONLY);
	rtp_session_set_scheduling_mode(session, 1);
	rtp_session_set_blocking_mode(session, 1);
	rtp_session_set_rtp_socket_recv_buffer_size(session, 262144); 
	
	rtp_session_set_local_addr(session, lipaddr, lport, lport + 1);
	rtp_session_set_remote_addr(session, ripaddr, rport);
	
	
	// rtp_session_set_connected_mode(session, TRUE);
	rtp_session_set_ssrc(session, 0x22);
	
	rtp_session_set_symmetric_rtp(session, TRUE);
	rtp_session_enable_adaptive_jitter_compensation(session, adapt);
	rtp_session_set_jitter_compensation(session, jittcomp);
	
	rtp_session_signal_connect(session, "ssrc_changed", (RtpCallback)ssrc_cb, 0);
	rtp_session_signal_connect(session, "ssrc_changed", (RtpCallback)rtp_session_reset, 0);
	
	// rtp_session_set_payload_type(session, 0);
	rtp_profile_set_payload(&av_profile, 96, &payload_type_h264);
	rtp_session_set_payload_type(session, 96);
	
	int framerate = 25;
	int timestamp = 90000 / 25;
	
	out_file = fopen("yuv420p_w352xh288.h264", "wb+");
	// out_file = fopen("yuv420p_w352xh288.h264", "ab+");
	
/////////////////////////////////////////////////////////////////////////////////////////////////////	
	
	mblk_t *mp = NULL;
	char *packet = NULL;
	int packet_len = 0;
/*
typedef struct rtp_header {
#ifdef ORTP_BIGENDIAN
	uint16_t version : 2;
	uint16_t padbit : 1;
	uint16_t extbit : 1;
	uint16_t cc : 4;
	uint16_t markbit : 1;
	uint16_t paytype : 7;
#else
	uint16_t cc : 4;
	uint16_t extbit : 1;
	uint16_t padbit : 1;
	uint16_t version : 2;
	uint16_t paytype : 7;
	uint16_t markbit : 1;
#endif
	uint16_t seq_number;
	uint32_t timestamp;
	uint32_t ssrc;
	uint32_t csrc[16];
} rtp_header_t;
*/
	uint16_t rtp_version;
	uint16_t rtp_padbit;
	uint16_t rtp_extbit;
	uint16_t rtp_cc;
	uint16_t rtp_markbit;
	uint16_t rtp_paytype;
	
	uint16_t rtp_seq;
	uint32_t rtp_timestamp;
	uint32_t rtp_ssrc;
	
	
	while (1) {
		ts += timestamp;
		mp = rtp_session_recvm_with_ts (session, ts);
		
		if (!mp) {
			continue;
		}
		
		rtp_version = rtp_get_version(mp);
		rtp_padbit = rtp_get_padbit(mp);
		rtp_extbit = rtp_get_extbit(mp);
		rtp_cc = rtp_get_cc(mp);
		rtp_markbit = rtp_get_markbit(mp);
		rtp_paytype = rtp_get_payload_type(mp);
		
		rtp_seq = rtp_get_seqnumber(mp);
		rtp_timestamp = rtp_get_timestamp(mp);
		rtp_ssrc = rtp_get_ssrc(mp);
        
        rtp_get_payload(mp, &mp->b_rptr);
        
        packet = mp->b_rptr;
        packet_len = mp->b_wptr - mp->b_rptr;
        ortp_h264_nalu_decode(packet, packet_len);
        
//        printf("%s: rtp_version = %d, rtp_padbit = %d, rtp_extbit = %d, rtp_cc = %d, rtp_markbit = %d, rtp_paytype = %d, rtp_seq = %d, rtp_timestamp = %d, rtp_ssrc = %d, packet_len = %d\n", 
//        			__func__, rtp_version, rtp_padbit, rtp_extbit, rtp_cc, rtp_markbit, rtp_paytype, rtp_seq, rtp_timestamp, rtp_ssrc, packet_len);
        
        freemsg(mp);
        mp = NULL;
	}
	
	
	
/////////////////////////////////////////////////////////////////////////////////////////////////////

cond = 0;
	while(cond) {
		have_more = 1;
		
		memset(buffer, 0, sizeof(buffer));
		
		while (have_more) {

			ts += timestamp;
			rbyte = rtp_session_recv_with_ts(session, buffer, 4096, ts, &have_more);
			
			if (rbyte > 0) {
				stream_received = 1;
			} else {
			
			}
			
			if (stream_received) {
				ortp_message("<have_more = %d, rbyte = %d, ts= %d (0x%02x 0x%02x 0x%02x 0x%02x 0x%02x)>", have_more, rbyte, ts, buffer[0], buffer[1], buffer[2], buffer[3], buffer[4]);
				// ortp_message("Receiving packet. <buffer = %s>", buffer);
				
				ortp_h264_nalu_decode(buffer, rbyte);
			}

			stream_received = 0;
		}

		// ts += timestamp;
		// ortp_message("Receiving packet. <rbyte = %d, ts= %d>", rbyte, ts);
	}
	
	fclose(out_file);

	rtp_session_destroy(session);
	ortp_exit();

//	ortp_global_stats_display();

	return 0;
}









#if 0
int main(int argc, char *argv[]) {
	RtpSession *session;
	unsigned char buffer[160];
	int err;
	uint32_t ts = 0;
	int stream_received = 0;
	FILE *outfile;
	int local_port;
	int have_more;
	int i, len;
	int format = 0;
	int soundcard = 0;
	int sound_fd = 0;
	int jittcomp = 40;
	bool_t adapt = TRUE;
	
	int lport = 12222;
	char *lipaddr = "127.0.0.1";

	int rport = 8000;
	char *ripaddr = "127.0.0.1";

	ortp_init();
	ortp_scheduler_init();
	ortp_set_log_level_mask(NULL, ORTP_DEBUG | ORTP_MESSAGE | ORTP_WARNING | ORTP_ERROR);
	signal(SIGINT, stop_handler);
	session = rtp_session_new(RTP_SESSION_RECVONLY);
	rtp_session_set_scheduling_mode(session, 1);
	rtp_session_set_blocking_mode(session, 1);
	
	rtp_session_set_local_addr(session, lipaddr, lport, lport + 1);
	rtp_session_set_remote_addr(session, ripaddr, rport);
	
	
	rtp_session_set_connected_mode(session, TRUE);
	rtp_session_set_symmetric_rtp(session, TRUE);
	rtp_session_enable_adaptive_jitter_compensation(session, adapt);
	rtp_session_set_jitter_compensation(session, jittcomp);
	// rtp_session_set_payload_type(session, 0);
	rtp_session_signal_connect(session, "ssrc_changed", (RtpCallback)ssrc_cb, 0);
	rtp_session_signal_connect(session, "ssrc_changed", (RtpCallback)rtp_session_reset, 0);
	
	rtp_profile_set_payload(&av_profile, 96, &payload_type_h264);
	rtp_session_set_payload_type(session, 96);
	
	//while (0) 
	{
		struct h264_hdr *h264_hdr = (struct h264_hdr *)h264_packet;
		struct rtp_h264_hdr *rtp_h264_hdr = (struct rtp_h264_hdr *)rtp_h264_sps_pkt;
		struct rtp_fua_hdr *rtp_fua_hdr   = (struct rtp_fua_hdr *)rtp_h264_sps_pkt;
		
		rtp_fua_in  = (uint8_t *)rtp_fua_hdr->data;
		rtp_fua_out = (uint8_t *)h264_hdr->data;
		
		len = sizeof(rtp_h264_sps_pkt);
		
		printf("%s: rtp_h264_hdr->forbidden_bit = %d\n", __func__, rtp_h264_hdr->forbidden_bit);
		printf("%s: rtp_h264_hdr->nal_ref_bit   = %d\n", __func__, rtp_h264_hdr->nal_ref_bit);
		printf("%s: rtp_h264_hdr->nal_unit_type = %d\n", __func__, rtp_h264_hdr->nal_unit_type);
	
		if (rtp_h264_hdr->nal_unit_type < 24)  {
			// h264 start code fill
			h264_hdr->prefix[0] = 0x00;
			h264_hdr->prefix[1] = 0x00;
			h264_hdr->prefix[2] = 0x00;
			h264_hdr->prefix[3] = 0x01;
		
			// h264 header fill
			h264_hdr->forbidden_bit = rtp_h264_hdr->forbidden_bit;
			h264_hdr->nal_ref_bit   = rtp_h264_hdr->nal_ref_bit;
			h264_hdr->nal_unit_type = rtp_h264_hdr->nal_unit_type;
			
			h264_packet_len = len - sizeof(struct rtp_h264_hdr);
			printf("%s: len = %d, h264_packet_len = %d\n", __func__, len, h264_packet_len);
			
			for (i = 0; i < h264_packet_len; i++) {
				h264_hdr->data[i] = rtp_h264_hdr->data[i];
			}
			
			h264_packet_total = h264_packet_len + sizeof(struct h264_hdr);
			
			for (i = 0; i < h264_packet_total; i++) {
				printf("0x%02x ", h264_packet[i]);
			}
			printf("\n");
		} else if (rtp_h264_hdr->nal_unit_type == 28) {
			if (rtp_fua_hdr->hdr.start) {
				// the first rtp fua unit packet
			
				// h264 start code fill
				h264_hdr->prefix[0] = 0x00;
				h264_hdr->prefix[1] = 0x00;
				h264_hdr->prefix[2] = 0x00;
				h264_hdr->prefix[3] = 0x01;
				
				// h264 header fill
				h264_hdr->forbidden_bit = rtp_fua_hdr->ind.forbidden_bit;
				h264_hdr->nal_ref_bit   = rtp_fua_hdr->ind.nal_ref_bit;
				h264_hdr->nal_unit_type = rtp_fua_hdr->hdr.nal_unit_type;
			}

			h264_packet_len = len - sizeof(struct rtp_fua_hdr);
			printf("%s: len = %d, h264_packet_len = %d\n", __func__, len, h264_packet_len);
			
			for (i = 0; i < h264_packet_len; i++) {
				*rtp_fua_out = rtp_fua_hdr->data[i];
				rtp_fua_out++;
				// h264_hdr->data[i] = rtp_fua_hdr->data[i];
			}

			if (rtp_fua_hdr->hdr.end) {
				// the last rtp fua unit packet
			}
		} else {
		
		}
	}
	


	cond = 0;
	while(cond) {
		have_more = 1;
		
		memset(buffer, 0, sizeof(buffer));
		
		while (have_more) {

			err = rtp_session_recv_with_ts(session, buffer, 160, ts, &have_more);
			
			if (err > 0) {
				stream_received = 1;
			} else {
			
			}
			
			if (stream_received) {
				ortp_message("Receiving packet. <have_more = %d, err = %d>", have_more, err);
				ortp_message("Receiving packet. <buffer = %s>", buffer);
			}

			stream_received = 0;
		}

		ts+=160;
		ortp_message("Receiving packet. <err = %d, ts= %d>", err, ts);
	}

	rtp_session_destroy(session);
	ortp_exit();

//	ortp_global_stats_display();

	return 0;
}
#endif




