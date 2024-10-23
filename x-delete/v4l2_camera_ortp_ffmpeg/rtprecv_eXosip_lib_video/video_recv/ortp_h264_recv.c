// #define DEBUG  1

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <linux/fb.h>
#include <time.h>
#include <linux/rtc.h>
#include <sys/time.h>

#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include <bctoolbox/vfs.h>
#include <ortp/ortp.h>
#include <signal.h>
#include <stdlib.h>

#include "initcall.h"
#include "gdbus/gdbus.h"

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

static uint8_t h264_packet[32 * 1024];
// static uint8_t h264_packet[4096];
static int  h264_packet_remain = 0;
static int  h264_packet_num = 0;
static int  h264_packet_len = 0;
static int  h264_packet_total = 0;

int h264_decoder_packet_write(uint8_t *data, int len);

struct ortp_h264_nalu {
	int len;
	uint8_t buf[32 * 1024];
};

struct ortp_h264_nalu ortp_h264_nalu[1] = {
	[0] = {
		.len = 0
	}
};

static int ortp_h264_nalu_decode(uint8_t *nalu, int len, uint16_t rtp_markbit)
{
	int i;
	static uint8_t *rtp_fua_out = NULL;
	struct h264_hdr *h264_hdr = (struct h264_hdr *)h264_packet;
	struct rtp_h264_hdr *rtp_h264_hdr = (struct rtp_h264_hdr *)nalu;
	struct rtp_fua_hdr *rtp_fua_hdr   = (struct rtp_fua_hdr *)nalu;
		
	pr_debug("%s: rtp_h264_hdr->forbidden_bit = %d\n", __func__, rtp_h264_hdr->forbidden_bit);
	pr_debug("%s: rtp_h264_hdr->nal_ref_bit   = %d\n", __func__, rtp_h264_hdr->nal_ref_bit);
	pr_debug("%s: rtp_h264_hdr->nal_unit_type = %d\n", __func__, rtp_h264_hdr->nal_unit_type);
	
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
		
		memcpy(&ortp_h264_nalu->buf[ortp_h264_nalu->len], h264_packet, h264_packet_total);
		ortp_h264_nalu->len += h264_packet_total;
		
		pr_debug("%s: <1>len = %d, h264_packet_len = %d, h264_packet_total = %d, rtp_markbit = %d, ortp_h264_nalu->len = %d\n", 
			__func__, len, h264_packet_len, h264_packet_total, rtp_markbit, ortp_h264_nalu->len);
		
		if (rtp_markbit == 1) {
			// h264_decoder_packet_write(ortp_h264_nalu->buf, ortp_h264_nalu->len);
			ortp_h264_nalu->len = 0;
		}
		h264_decoder_packet_write(h264_packet, h264_packet_total);
		// emulcap_pipe_write(h264_packet, h264_packet_total);
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
			ortp_h264_nalu->len = 0;
		}

		h264_packet_len = len - sizeof(struct rtp_fua_hdr);
			
		for (i = 0; i < h264_packet_len; i++) {
			*rtp_fua_out = rtp_fua_hdr->data[i];
			rtp_fua_out++;
			// h264_hdr->data[i] = rtp_fua_hdr->data[i];
		}
		h264_packet_total += h264_packet_len;
		pr_debug("%s: <2>len = %d, h264_packet_len = %d, h264_packet_total = %d, rtp_markbit = %d\n", __func__, len, h264_packet_len, h264_packet_total, rtp_markbit);
		
		memcpy(&ortp_h264_nalu->buf[ortp_h264_nalu->len], h264_packet, h264_packet_total);
		ortp_h264_nalu->len += h264_packet_total;

		if (rtp_fua_hdr->hdr.end) {
			// the last rtp fua unit packet
			rtp_fua_out = NULL;
			
			pr_debug("%s: <3>len = %d, h264_packet_len = %d, h264_packet_total = %d, rtp_markbit = %d, ortp_h264_nalu->len = %d\n", 
				__func__, len, h264_packet_len, h264_packet_total, rtp_markbit, ortp_h264_nalu->len);

			h264_decoder_packet_write(h264_packet, h264_packet_total);
			// emulcap_pipe_write(h264_packet, h264_packet_total);
			
			if (rtp_markbit == 1) {
				// h264_decoder_packet_write(ortp_h264_nalu->buf, ortp_h264_nalu->len);
				ortp_h264_nalu->len = 0;
			}
		}
	} else {
		
	}

	return 0;
}

static void ssrc_cb(RtpSession *session) 
{
	pr_debug("hey, the ssrc has changed !\n");
}

static void *ortp_h264_recv_thread(void *arg)
{
	uint32_t ts = 0;
	mblk_t *mp = NULL;
	char *packet = NULL;
	int packet_len = 0;
	RtpSession *session;
	
	int framerate = 50;
	int timestamp = 90000 / framerate;
	int interval = 1000 / framerate;
	
	int jittcomp = 40;
	bool_t adapt = TRUE;
	
	int lport = 12222;
	char *lipaddr = "127.0.0.1";

	int rport = 8000;
	char *ripaddr = "127.0.0.1";

	uint16_t rtp_version;
	uint16_t rtp_padbit;
	uint16_t rtp_extbit;
	uint16_t rtp_cc;
	uint16_t rtp_markbit;
	uint16_t rtp_paytype;
	
	uint16_t rtp_seq;
	uint32_t rtp_timestamp;
	uint32_t rtp_ssrc;
	
	session = rtp_session_new(RTP_SESSION_RECVONLY);
	rtp_session_set_scheduling_mode(session, 1);
	rtp_session_set_blocking_mode(session, 1);
	// rtp_session_set_rtp_socket_recv_buffer_size(session, 262144); 
	rtp_session_set_recv_buf_size(session, 262144);
	
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

	pr_info("%s: >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> <2>ts = %d\n", __func__, ts);
	
	while (1) {
		ts += timestamp;
		mp = rtp_session_recvm_with_ts (session, ts);
		
		if (!mp) {
			usleep(20 * 1000);
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
        // pr_info("%s: >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> <1>ts = %d, rtp_markbit = %d, packet_len = %d\n", __func__, ts, rtp_markbit, packet_len);
	
		rtp_markbit = 1;
        ortp_h264_nalu_decode(packet, packet_len, rtp_markbit);
        
//        pr_debug("%s: rtp_version = %d, rtp_padbit = %d, rtp_extbit = %d, rtp_cc = %d, rtp_markbit = %d, rtp_paytype = %d, rtp_seq = %d, rtp_timestamp = %d, rtp_ssrc = %d, packet_len = %d\n", 
//        			__func__, rtp_version, rtp_padbit, rtp_extbit, rtp_cc, rtp_markbit, rtp_paytype, rtp_seq, rtp_timestamp, rtp_ssrc, packet_len);
        
        freemsg(mp);
        mp = NULL;
	}

	return NULL;
}
thread_initcall(ortp_h264_recv_thread);






