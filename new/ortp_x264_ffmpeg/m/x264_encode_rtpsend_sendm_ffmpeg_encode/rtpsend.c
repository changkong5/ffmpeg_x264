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

#include <libavutil/opt.h>
#include <libavutil/imgutils.h>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#include <libavcodec/avcodec.h>
#include <libavutil/time.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>


#ifndef _WIN32
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
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

//uint8_t *rtp_fua_in;
//uint8_t *rtp_fua_out;

uint8_t h264_packet[32 * 1024];
int  h264_packet_remain = 0;
int  h264_packet_num = 0;
int  h264_packet_len = 0;
int  h264_packet_total = 0;

char h264_pps_pkt[8] = {0x00, 0x00, 0x00, 0x01, 0x68, 0xCE, 0x3C, 0x80};
char h264_sps_pkt[19] = {0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0xC0, 0x1F, 0x8C, 0x8D, 0x40, 0x48, 0x14, 0xB2, 0xF0, 0x0F, 0x08, 0x84, 0x6A};

/////////////////////////////////////////////////////////////////

void h264_file_frame_write(uint8_t *packet, int packet_len)
{
	static int count = 0;
	char name[64];
	FILE *file;
	sprintf(name, "yuv420p_w352xh288-%d.h264", count++);
	
	file = fopen(name, "wb+");
	
	fwrite(packet, 1, packet_len, file);
	
	fclose(file);
}

int rtp_session_send_with_ts_markerbit(RtpSession *session, uint8_t *packet, int packet_len, uint32_t userts, int markbit)
{
	mblk_t *m;
	
#ifdef USE_SENDMSG
	// message in two chained fragments: header and payload
	m = rtp_session_create_packet_header(session, 0);
	m->b_cont = rtp_package_packet((uint8_t *)packet, packet_len, NULL);
#else
	// message in one block - continuous buffer
	m = rtp_session_create_packet_header(session, packet_len); // ask for len size to be allocated after the header so we can copy the payload into it
	memcpy(m->b_wptr, packet, packet_len);
	m->b_wptr += packet_len;
#endif

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

	// rtp_set_version(m, 2);
	// rtp_set_padbit(m, 0);
	// rtp_set_extbit(m, 0);
	// rtp_set_cc(m, 0);
	rtp_set_markbit(m, markbit);
	// rtp_set_payload_type(m, 96);
	// rtp_set_timestamp(m, getTimestamp());

/*
	rtp_set_version(packet, 2);
	rtp_set_padbit(packet, (mBuffer[0] >> 5) & 0x1);
	rtp_set_extbit(packet, (mBuffer[0] >> 4) & 0x1);
	rtp_set_cc(packet, (mBuffer[0]) & 0xF);
	rtp_set_markbit(packet, (mBuffer[1] >> 7) & 0x1);
	rtp_set_payload_type(packet, mBuffer[1] & 0x7F);
	rtp_set_timestamp(packet, getTimestamp());
*/

	return rtp_session_sendm_with_ts(session, m, userts);
}

uint32_t user_ts = 0;

int ortp_h264_nalu_send(RtpSession *session, uint8_t *nalu, int len)
{
	int markbit = 0;
	int i, k, wbyte = 0;
	int framerate = 25;
	int timestamp = 90000 / 25;

	struct h264_hdr *h264_hdr = (struct h264_hdr *)nalu;
	struct rtp_h264_hdr *rtp_h264_hdr = (struct rtp_h264_hdr *)h264_packet;
	struct rtp_fua_hdr *rtp_fua_hdr   = (struct rtp_fua_hdr *)h264_packet;

	// rtp_fua_in  = (uint8_t *)h264_hdr->data;
	// rtp_fua_out = (uint8_t *)rtp_fua_hdr->data;
	
	printf("%s: h264_hdr->header   = %02x\n", __func__, h264_hdr->header);
	printf("%s: h264_hdr->forbidden_bit = %d\n", __func__, h264_hdr->forbidden_bit);
	printf("%s: h264_hdr->nal_ref_bit   = %d\n", __func__, h264_hdr->nal_ref_bit);
	printf("%s: h264_hdr->nal_unit_type = %d\n", __func__, h264_hdr->nal_unit_type);
		
	h264_packet_len = len - sizeof(struct h264_hdr);
	printf("%s: len = %d, h264_packet_len = %d, RTP_MAX_MTU = %d\n", __func__, len, h264_packet_len, RTP_MAX_MTU);
		
	if (len < RTP_MAX_MTU) {
		// rtp h264 header part fill
		rtp_h264_hdr->forbidden_bit = h264_hdr->forbidden_bit;
		rtp_h264_hdr->nal_ref_bit = h264_hdr->nal_ref_bit;
		rtp_h264_hdr->nal_unit_type = h264_hdr->nal_unit_type;
		
		// rtp h264 data part fill
		for (i = 0; i < h264_packet_len; i++) {
			rtp_h264_hdr->data[i] = h264_hdr->data[i];
		}
			
		h264_packet_total = h264_packet_len + sizeof(struct rtp_h264_hdr);
			
		
		// rtp_h264_packet = rtp_h264_hdr + rtp_h264_data
		// wbyte = rtp_session_send_with_ts(session, h264_packet, h264_packet_total, user_ts);
		
		markbit = 0;
		if (h264_hdr->nal_unit_type == H264_NALU_TYPE_IPB || h264_hdr->nal_unit_type == H264_NALU_TYPE_IDR) {
			markbit = 1;
			user_ts += timestamp;
		}
		
		printf("%s: >>>>>>>>>>>>>>>>>>>>>>> <1>user_ts = %d, h264_hdr->nal_unit_type = %d, markbit = %d, h264_packet_total = %d\n", 
					__func__, user_ts, h264_hdr->nal_unit_type, markbit, h264_packet_total);
		wbyte = rtp_session_send_with_ts_markerbit(session, h264_packet, h264_packet_total, user_ts, markbit);
		// usleep(100);
			
		 //如果是6,7,8类型的包，不应该延时；之前有停顿，原因这在这
/*		
		// display packet data
		for (i = 0; i < h264_packet_len; i++) {
			printf("0x%02x ", rtp_h264_hdr->data[i]);
		}
		printf("\n");
			
		for (i = 0; i < h264_packet_len; i++) {
			printf("0x%02x ", h264_hdr->data[i]);
		}
		printf("\n");
*/
	} else {
		uint8_t *rtp_fua_in  = (uint8_t *)h264_hdr->data;
	
		h264_packet_num = h264_packet_len / RTP_MAX_MTU;
		h264_packet_remain = h264_packet_len % RTP_MAX_MTU;
			
		if (h264_packet_remain > 0) {
			h264_packet_num += 1;
		}
		printf("%s: h264_packet_num = %d, h264_packet_remain = %d\n", __func__, h264_packet_num, h264_packet_remain);
		
		
		// user_ts += timestamp;
			
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
			printf("%s: <1>k = %d, h264_packet_num = %d, h264_packet_remain = %d\n", __func__, k, h264_packet_num, h264_packet_remain);
			
			if (k == 0) {
				// the first fua unit packet
				rtp_fua_hdr->hdr.start = 1;
				
				markbit = 0;
			}
			
			if (k == h264_packet_num - 1) {
				// the last fua unit packet
				rtp_fua_hdr->hdr.end     = 1;
					
				if (h264_packet_remain > 0) {
					h264_packet_total = h264_packet_remain + sizeof(struct rtp_fua_hdr);
				}
				
				markbit = 1;
				user_ts += timestamp;
			}
			h264_packet_len = h264_packet_total - sizeof(struct rtp_fua_hdr);
				
			for (i = 0; i < h264_packet_len; i++) {
				rtp_fua_hdr->data[i] = *rtp_fua_in;
				rtp_fua_in++;
				// rtp_fua_hdr->data[i] = h264_hdr->data[i];
			}
				
			printf("%s: >>>>>>>>>>>>>>>>>>>>>>> <2>user_ts = %d, h264_hdr->nal_unit_type = %d, markbit = %d, h264_packet_total = %d (0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x)\n", 
						__func__, user_ts, h264_hdr->nal_unit_type, markbit, h264_packet_total, 
						h264_packet[0], h264_packet[1], h264_packet[2], h264_packet[3], h264_packet[4], h264_packet[5]);
			
			wbyte = rtp_session_send_with_ts_markerbit(session, h264_packet, h264_packet_total, user_ts, markbit);
			// wbyte = rtp_session_send_with_ts(session, h264_packet, h264_packet_total, user_ts);
			printf("%s: <2>k = %d, h264_packet_total = %d, h264_packet_remain = %d\n", __func__, k, h264_packet_total, h264_packet_remain);
			// usleep(100);
		}
	}
	
	if (h264_hdr->nal_unit_type == H264_NALU_TYPE_IPB || h264_hdr->nal_unit_type == H264_NALU_TYPE_IDR) {
		usleep(40 * 1000);
	}
	
	// usleep(40 * 1000);
	
	return 0;
}

/////////////////////////////////////////////////////////////////

/* 
		p-3  p-2  p-1    p   p+1 p+2 p+3
------------------------------------------------
  .... \ 0  \  0  \ 1 \ hdr \ x \ x \ x \
------------------------------------------------
*/
uint8_t *h264_packet_find_nalu_hdr(uint8_t *packet, size_t len)
{
    uint8_t *p = packet;
    uint8_t *end = packet + len;

#if 1
    p = packet + 3;

    while (p < end) {
        if (p[-1] > 1) {
        	p += 3;
        }
        else if (p[-2]) {
        	p += 2;
        } else if (p[-3]|(p[-1]-1)) {
        	p++;
        }  else {
        	// printf("hdr_index = %ld, index = %ld\n", p - data, p - 3 - 1 - data);
            // nalus.emplace_back(p - 3 - data);
            // p++;
            return p;
        }
    }
#else

	for (int i = 0; i < len - 4; i++) {
		if (p[i] == 0 && p[i + 1] == 0 && p[i + 2] == 1) {
			return &p[i + 3];
		}
	}    
#endif
    
    return NULL;
}

struct h264_nalu {
	struct {
		uint8_t *payload;
		int payload_len;
	} nalu[256];
	int nalu_num;
	uint8_t *hdr;
	uint8_t *next;
	uint8_t *start;
	uint8_t *end;
	
	int hdr_len;
	int date_len;
	int next_hdr_len;
	int total;
};

struct h264_nalu nalu[1];

int h264_packet_split_to_nalu(uint8_t *packet, int len)
{
	nalu->nalu_num = 0;   
	nalu->hdr = NULL;
	nalu->next = NULL;  	
	nalu->start = &packet[0];
	nalu->end = &packet[len];
	nalu->total = len;	    	
		    	
	while (1) {
		nalu->next = h264_packet_find_nalu_hdr(nalu->start, nalu->total);

		if (nalu->next) {
			printf("%s: <1> nalu->start = %p, nalu->next = %p, offset = %ld (0x%08lx) [0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x]\n", 
			__func__, nalu->start, nalu->next, (nalu->next - packet), (nalu->next - packet), 
			nalu->next[-4], nalu->next[-3], nalu->next[-2], nalu->next[-1], nalu->next[0], nalu->next[1], nalu->next[2], nalu->next[3], nalu->next[4], nalu->next[5]);


			if (nalu->next[-4] == 0) {
				// 00 00 00 01 HH
				nalu->next_hdr_len = 5;
			} else {
				// 00 00 01 HH
				nalu->next_hdr_len = 4;
			}
			printf("%s: <2> nalu->next_hdr_len = %d \n", __func__, nalu->next_hdr_len);
			
			// nalu->start++;
			nalu->start = nalu->next + 1;
			nalu->total -= nalu->next_hdr_len;
			
			if (nalu->hdr == NULL) {
				nalu->hdr = nalu->next;
				nalu->hdr_len = nalu->next_hdr_len;
				continue;
			}
			
			nalu->date_len = nalu->next - nalu->hdr - nalu->next_hdr_len;
			printf("%s: <3> nalu->hdr_len = %d, nalu->date_len = %d \n", __func__, nalu->hdr_len, nalu->date_len);
			
			nalu->nalu[nalu->nalu_num].payload = &nalu->hdr[1 - nalu->hdr_len];
			nalu->nalu[nalu->nalu_num].payload_len = nalu->hdr_len + nalu->date_len;
			nalu->nalu_num++;
			
			nalu->hdr = nalu->next;
			nalu->hdr_len = nalu->next_hdr_len;
			printf("%s: <4> nalu->hdr_len = %d, nalu->nalu[nalu->nalu_num].payload_len = %d\n", __func__, nalu->hdr_len, nalu->nalu[nalu->nalu_num].payload_len);
		} else {
			printf("%s: <8> nalu->hdr = %p, nalu->hdr_len = %d, offset = %ld (0x%08lx)\n", __func__, nalu->hdr, nalu->hdr_len, (nalu->hdr - packet), (nalu->hdr - packet));
		
			nalu->date_len = nalu->end - 1 - nalu->hdr;
			nalu->nalu[nalu->nalu_num].payload = &nalu->hdr[1 - nalu->hdr_len];
			nalu->nalu[nalu->nalu_num].payload_len = nalu->hdr_len + nalu->date_len;
			nalu->nalu_num++;
			printf("%s: <9> nalu->nalu_num = %d, nalu->nalu[nalu->nalu_num - 1].payload_len = %d, nalu->hdr_len = %d, nalu->date_len = %d\n", 
			__func__, nalu->nalu_num, nalu->nalu[nalu->nalu_num - 1].payload_len, nalu->hdr_len, nalu->date_len);
			
			// end and exit loop
			break;
		}
	}
	
	return nalu->nalu_num;
}

/////////////////////////////////////////////////////////////////

// ffplay -f rawvideo -s 352x288 yuv420p_w352xh288.yuv 																// correct   // play yuv
// ffplay yuv420p_w352xh288.h264 																					// correct   // play h264
// ffmpeg -s 352x288 -i yuv420p_w352xh288.yuv -c:v h264 yuv420p_w352xh288.h264   									// correct	 // yuv ---> h264
// ffmpeg -c:v h264 -i yuv420p_w352xh288.h264 yuv420p_w352xh288.yuv    												// correct   // h264 ---> yuv

/*
int read_yuv_frame(FILE *fp, x264_picture_t *pic, int width, int height) 
{
	int y_size = width * height;
	int uv_size = y_size / 4;
	
	if (fread(pic->img.plane[0], 1, y_size, fp) != y_size) {
		return EXIT_FAILURE;
	}
	
	if (fread(pic->img.plane[1], 1, uv_size, fp) != uv_size) {
		return EXIT_FAILURE;
	}
	
	if (fread(pic->img.plane[2], 1, uv_size, fp) != uv_size) {
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}
*/

/////////////////////////////////////////////////////////////////

int runcond = 1;

void stophandler(int signum) {
	runcond = 0;
}

int main(int argc, char *argv[]) {
	RtpSession *session;
	//unsigned char buffer[160];
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
	// ortp_set_log_level_mask(NULL, ORTP_MESSAGE | ORTP_WARNING | ORTP_ERROR);
	session = rtp_session_new(RTP_SESSION_SENDONLY);

	rtp_session_set_scheduling_mode(session, 1);
	rtp_session_set_blocking_mode(session, 1);
	// rtp_session_set_connected_mode(session, TRUE);
	rtp_session_set_ssrc(session, 0x22);
	
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

	// signal(SIGINT, stophandler);


	printf("sizeof(struct h264_hdr)     = %ld\n", sizeof(struct h264_hdr));
	printf("sizeof(struct rtp_h264_hdr) = %ld\n", sizeof(struct rtp_h264_hdr));
	printf("sizeof(struct rtp_fua_hdr)  = %ld\n", sizeof(struct rtp_fua_hdr));
	printf("sizeof(struct rtp_fub_hdr)  = %ld\n", sizeof(struct rtp_fub_hdr));
	
	int wbyte = 0;
	int framerate = 25;
	int timestamp = 90000 / 25;

/*	
	uint8_t *packet;
	int packet_len;
	int zero;
*/
/////////////////////////////////////////////////////////////////////////////////////////

/*
	FILE* in_file = fopen("yuv420p_w352xh288.yuv", "rb");
	FILE* out_file = fopen("yuv420p_w352xh288.h264", "wb+");

	if (!in_file) {
		fprintf(stderr, "Failed to open input file\n"); 
		return EXIT_FAILURE;
	}

	if (!out_file) {
		fprintf(stderr, "Failed to open output file\n"); 
		fclose(in_file);
		return EXIT_FAILURE;
	}

	x264_t *encoder;            
	x264_picture_t pic_in, pic_out; 
	x264_param_t param;       
	x264_nal_t *nals;       
	int num_nals;             


	x264_param_default_preset(&param, "medium", "zerolatency");
	param.i_bitdepth = 8;     
	param.i_csp = X264_CSP_I420;  
	param.i_width = 352;   
	param.i_height = 288;    
	param.i_fps_num = 25;    
	param.i_fps_den = 1;      
	x264_param_apply_profile(&param, "high");

	encoder = x264_encoder_open(&param);
	
	if (!encoder) {
		fprintf(stderr, "Failed to open encoder\n"); 
		fclose(in_file);
		fclose(out_file);
		return EXIT_FAILURE;
	}
	
	x264_picture_alloc(&pic_in, param.i_csp, param.i_width, param.i_height);
	x264_picture_init(&pic_out);

	int frame_count = 0; 

	while (1 && read_yuv_frame(in_file, &pic_in, param.i_width, param.i_height) == 0) {
		pic_in.i_pts = frame_count++; 
		printf("%s: frame_count = %d, param.i_width = %d, param.i_height = %d, total = %d\n", __func__, frame_count, param.i_width, param.i_height, (param.i_width*param.i_height*3/2));


		int frame_size = x264_encoder_encode(encoder, &nals, &num_nals, &pic_in, &pic_out);
		
		printf("%s: frame_size = %d, num_nals = %d\n", __func__, frame_size, num_nals);
		if (frame_size < 0) {
			fprintf(stderr, "Failed to encode frame\n"); 
			x264_picture_clean(&pic_in); 
			x264_encoder_close(encoder); 
			fclose(in_file); 
			fclose(out_file);
			return EXIT_FAILURE;
		}

		for (int i = 0; i < num_nals; i++) {
		// printf("%s: i = %d, num_nals = %d, nals[i].i_payload = %d， nals[i].i_type = %d\n", __func__, i, num_nals, nals[i].i_payload, nals[i].i_type);
			printf("%s: i = %d, num_nals = %d, nals[i].i_payload = %d, nals[i].i_type = %d (0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x)\n", __func__, 
			i, num_nals, nals[i].i_payload, nals[i].i_type, nals[i].p_payload[0], nals[i].p_payload[1], nals[i].p_payload[2], nals[i].p_payload[3], nals[i].p_payload[4], nals[i].p_payload[5]);
			
			// if (nals[i].i_payload > 512) continue;
			
			for (zero = 0; zero < 4; zero++) {
				if (nals[i].p_payload[zero] != 0) {
					break;
				}
			}
			printf("%s: zero = %d\n", __func__, zero);
			
			if (zero == 2) {
				// nalu start code [0x00 0x00 0x01]
				packet_len = nals[i].i_payload;
				packet = &nals[i].p_payload[0];
			} else {
				// nalu start code [0x00 0x00 0x00 0x01]
				packet_len = nals[i].i_payload - 1;
				packet = &nals[i].p_payload[1];
			}
			
			fwrite(packet, 1, packet_len, out_file);
			//h264_file_frame_write(packet, packet_len);
			
			
			ortp_h264_nalu_send(session, packet, packet_len);
			// usleep(100 * 1000);
		}
		
		// break;
	}

	x264_picture_clean(&pic_in);
	x264_encoder_close(encoder);

	fclose(in_file);
	fclose(out_file);
*/

/////////////////////////////////////////////////////////////////////////////////////////

	int fd, ret;
	int i_fd, got_picture;
	AVCodec *codec;
	AVCodecContext *codec_ctx;
	AVFrame *frame;
	AVPacket *packet;
	char *buffer = NULL;
	int size, zero_cont;
	int rbyte;
	char buf[4096];
	char *data = buf;
	int fps = 25;
	int64_t pts = 0;
	int max_packet = 0;
	int max_nalu = 0;
	
	uint8_t *packet_data;
	int packet_len;
	int zero;

	fd = open("yuv420p_w352xh288.h264", O_RDWR | O_CREAT, 0666);
	
	i_fd = open("yuv420p_w352xh288.yuv", O_RDWR, 0666);
	
	// codec = avcodec_find_encoder_by_name("libx264");
	// codec = avcodec_find_encoder_by_name("h264");
	codec = avcodec_find_encoder(AV_CODEC_ID_H264);

	if (!codec) {
		printf("%s: <avcodec_find_decoder error>\n", __func__);
	}
	
	codec_ctx = avcodec_alloc_context3(codec);
	
	if (!codec_ctx) {
		printf("%s: <avcodec_alloc_context3 error>\n", __func__);
	}
	
	codec_ctx->flags |= AV_CODEC_FLAG_QSCALE;
	codec_ctx->rc_min_rate =  500000;
	codec_ctx->rc_max_rate = 1500000;

	// pCodecCtx->bit_rate = 400000;
	// codec_ctx->bit_rate = 3000000;
	// codec_ctx->bit_rate = 90000;
	
	codec_ctx->bit_rate = 400000;
	
	codec_ctx->width = 352;
    codec_ctx->height = 288;
    codec_ctx->codec_id = AV_CODEC_ID_H264;
    codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
    codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    
    fps = 25;
    codec_ctx->framerate.num = fps;  //帧率
    codec_ctx->framerate.den = 1;
    codec_ctx->time_base.num = 1;
    codec_ctx->time_base.den = fps;  //time_base一般是帧率的倒数，但不总是
 
    // codec_ctx->time_base = (AVRational){1, 25};
    // codec_ctx->framerate = (AVRational){25, 1};
    codec_ctx->gop_size = 25; // 50;
    codec_ctx->max_b_frames = 1;
    
    codec_ctx->qmin = 20; // 10 - 30   // codec_ctx->qmin = 10 , picture_size = 2353227 and  codec_ctx->qmin = 30 , picture_size = 180202
    codec_ctx->qmax = 51;
    
    //codec_ctx->me_range = 16;
    //codec_ctx->max_qdiff = 4;
    codec_ctx->qcompress = 0.6;
    
    if (0 && codec->id == AV_CODEC_ID_H264) {
        ret = av_opt_set(codec_ctx->priv_data, "preset", "medium", 0);
        if(ret != 0) {
            printf("av_opt_set preset failed\n");
        }
        
        ret = av_opt_set(codec_ctx->priv_data, "profile", "main", 0); 
        if(ret != 0) {
            printf("av_opt_set profile failed\n");
        }
        
        ret = av_opt_set(codec_ctx->priv_data, "tune","zerolatency",0);
        if(ret != 0) {
            printf("av_opt_set tune failed\n");
        }
    }
    
    if (1 && codec->id == AV_CODEC_ID_H264) {
 #if 1
        // av_opt_set(codec_ctx->priv_data, "preset", "slow", 0);
 #else       
        // av_opt_set(codec_ctx->priv_data, "profile", "baseline", 0); 
        av_opt_set(codec_ctx->priv_data, "profile", "main", 0);
        av_opt_set(codec_ctx->priv_data, "preset", "medium", 0);
        //av_opt_set(codec_ctx->priv_data, "preset", "fast", 0);
        
        // av_opt_set(codec_ctx->priv_data, "tune","fastdecode",0);
        av_opt_set(codec_ctx->priv_data, "tune","zerolatency",0);
#endif
    }
    
	packet = av_packet_alloc();

	if (!packet) {
		printf("%s: <av_packet_alloc error>\n", __func__);
	}

	frame = av_frame_alloc();

	if (!frame) {
		printf("%s: <av_frame_alloc error>\n", __func__);
	}
	
	frame->format = codec_ctx->pix_fmt;
    frame->width  = codec_ctx->width;
    frame->height = codec_ctx->height;
    ret = av_frame_get_buffer(frame, 0);
    
    if (ret != 0) {
     	printf("Could not allocate the video frame data\n");
    }
    
	ret = avcodec_open2(codec_ctx, codec, NULL);

	if (ret < 0) {
		printf("%s: <avcodex_open2 error> ret = %d\n", __func__, ret);
	}

	zero_cont = 0;

	size = av_image_get_buffer_size(frame->format, frame->width, frame->height, 1);
	printf("%s: size = %d\n", __func__, size);
	buffer = av_mallocz(size * sizeof(uint8_t));
	
	int need_size = av_image_fill_arrays(frame->data, frame->linesize, buffer,
                                             frame->format, frame->width, frame->height, 1);
                                             
    printf("%s: size = %d, need_size = %d\n", __func__, size, need_size);  
    
	while (1) {
		rbyte = read(i_fd, buffer, size);
		printf("%s: zero_cont = %d, len = %d, rbyte = %d\n", __func__, zero_cont, len, rbyte);
		
		if (rbyte == 0) {
			zero_cont++;
			
			if (zero_cont == 1) {
				printf(">>>>>>>>>>>>>>>>>>>>>>> <flush_encode> \n");
			} else {
				break;
			}
		}

		ret = av_frame_make_writable(frame);
		
		if (ret != 0) {
            printf("av_frame_make_writable failed, ret = %d\n", ret);
            break;
        }
       
        // pts += 40;
        pts += 1;
        frame->pts = pts;
        printf("Send frame %ld \n", frame->pts);

		ret = avcodec_send_frame(codec_ctx, frame);     
        // printf("%s: <avcodec_send_frame> ret = %d\n", __func__, ret);
        
        if (ret < 0) {
		    fprintf(stderr, "Error sending a frame for encoding\n");
		    break;
		}
        
        while (1) {
        	ret = avcodec_receive_packet(codec_ctx, packet);
        	// printf("%s: <avcodec_receive_packet> ret = %d, AVERROR(EAGAIN) = %d, AVERROR_EOF = %d\n", __func__, ret, AVERROR(EAGAIN), AVERROR_EOF);
        	
        	 if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            	break;
        	} else if (ret < 0) {
            	return 0;
        	}
        	
        	printf("%s: <avcodec_send_frame> packet->size = %d\n", __func__, packet->size);
        	for (int o = 0; o < 9; o++) printf("0x%02x ", packet->data[o]); printf("\n");
        	
        	ret = write(fd, packet->data, packet->size);
        	fsync(fd);
        
   
/*     	
		h264_packet_split_to_nalu(packet->data, packet->size);
		
int all_byte = 0;
// max_packet = 0;
for (int i = 0; i < nalu->nalu_num; i++) {
	all_byte += nalu->nalu[i].payload_len;
	printf("%s: i = %d, offset = %ld (0x%08lx), nalu->nalu[nalu->nalu_num].payload = %p, nalu->nalu[nalu->nalu_num].payload_len = %d [0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x]\n", 
	__func__, i, (nalu->nalu[i].payload - packet->data), (nalu->nalu[i].payload - packet->data), nalu->nalu[i].payload, nalu->nalu[i].payload_len,
	nalu->nalu[i].payload[0], nalu->nalu[i].payload[1], nalu->nalu[i].payload[2], nalu->nalu[i].payload[3], nalu->nalu[i].payload[4], 
	nalu->nalu[i].payload[5], nalu->nalu[i].payload[6], nalu->nalu[i].payload[7], nalu->nalu[i].payload[8]);
	
	if (max_packet < nalu->nalu[i].payload_len) max_packet = nalu->nalu[i].payload_len;
}

if (max_nalu < nalu->nalu_num) max_nalu = nalu->nalu_num;
printf("%s: <avcodec_send_frame> all_byte = %d\n", __func__, all_byte);
*/

  
  #if 1   	
			h264_packet_split_to_nalu(packet->data, packet->size);
			///printf("%s: nalu->nalu_num = %d\n", __func__, nalu->nalu_num);
			
			for (int i = 0; i < nalu->nalu_num; i++) {
				
				
				for (zero = 0; zero < 4; zero++) {
					if (nalu->nalu[i].payload[zero] != 0) {
						break;
					}
				}
				printf("%s: nalu->nalu_num = %d, i = %d, zero = %d, nalu->nalu[i].payload_len = %d\n", __func__, nalu->nalu_num, i, zero, nalu->nalu[i].payload_len);
				for (int z = 0; z < 9; z++) printf("0x%02x ", nalu->nalu[i].payload[z]); printf("\n");
				
				if (zero == 2) {
					// nalu start code [0x00 0x00 0x01]
					packet_len = nalu->nalu[i].payload_len;
					packet_data = &nalu->nalu[i].payload[0];
				} else {
					// nalu start code [0x00 0x00 0x00 0x01]
					packet_len = nalu->nalu[i].payload_len - 1;
					packet_data = &nalu->nalu[i].payload[1];
				}
				
				//fwrite(packet, 1, packet_len, out_file);
				//h264_file_frame_write(packet, packet_len);
				if (max_packet < nalu->nalu[i].payload_len) max_packet = nalu->nalu[i].payload_len;
				
				
				ortp_h264_nalu_send(session, packet_data, packet_len);
				// usleep(100 * 1000);
			}
			
			 if (max_nalu < nalu->nalu_num) max_nalu = nalu->nalu_num;
#endif 
			 
        }

	}
	
	printf("%s: <avcodec_send_frame> max_packet = %d, max_nalu = %d\n", __func__, max_packet, max_nalu);

	close(i_fd);
	close(fd);
	av_freep(&buffer);
	
	av_packet_free(&packet);
	av_frame_free(&frame);
	avcodec_free_context(&codec_ctx);
        	
/////////////////////////////////////////////////////////////////////////////////////////

	rtp_session_destroy(session);
	ortp_exit();

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

	rtp_session_destroy(session);
	ortp_exit();

	return 0;
}

#endif











