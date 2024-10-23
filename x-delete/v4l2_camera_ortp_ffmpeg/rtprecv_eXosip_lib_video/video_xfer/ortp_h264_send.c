#define DEBUG  1

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

//////////////////////////////////////////////////////////////////////////////

struct aqpacket {
	int len;
	uint8_t data[0];
};

static GAsyncQueue *aqueue = NULL;

static void async_queue_packet_init(void)
{
	aqueue = g_async_queue_new();
}

static void async_queue_packet_deinit(void)
{
	g_async_queue_unref(aqueue);
}

static int async_queue_packet_push(uint8_t *data, int len)
{
	int size = 0;
	struct aqpacket *packet;
	
	size = sizeof(struct aqpacket) + len;
	
	packet = malloc(size);
	
	pr_debug("%s: size = %d, len = %d\n", __func__, size, len);
	if (!packet) {
		return 0;
	}
	packet->len = len;
	memcpy(packet->data, data, len);
	
	g_async_queue_push(aqueue, packet);
	
	return size;
}

static int async_queue_packet_pop(uint8_t *data, int len)
{
	int size = 0;
	struct aqpacket *packet;
	
	packet = g_async_queue_pop(aqueue);
	
	if (!packet) {
		return 0;
	}
	
	if (len > packet->len) {
		size = packet->len;
	} else {
		size = len;
	}
	pr_debug("%s: size = %d, len = %d, packet->len = %d\n", __func__, size, len, packet->len);
	memcpy(data, packet->data, packet->len);
	
	free(packet);
	
	return size;
}

int ortp_h264_nalu_packet_write(uint8_t *data, int len)
{
	return async_queue_packet_push(data, len);
}

int ortp_h264_nalu_packet_read(uint8_t *data, int len)
{
	return async_queue_packet_pop(data, len);
}

static int ortp_h264_nalu_init(void)
{
	pr_info("%s: ...\n", __func__);
	async_queue_packet_init();
	
    return 0;
}
fn_initcall(ortp_h264_nalu_init);

static int ortp_h264_nalu_exit(void)
{
	pr_info("%s: ...\n", __func__);
	async_queue_packet_deinit();
	
    return 0;
}
fn_exitcall(ortp_h264_nalu_exit);
//////////////////////////////////////////////////////////////////////////////

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


uint8_t h264_packet[32 * 1024];
int  h264_packet_remain = 0;
int  h264_packet_num = 0;
int  h264_packet_len = 0;
int  h264_packet_total = 0;

/////////////////////////////////////////////////////////////////

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

	// rtp_set_version(m, 2);
	// rtp_set_padbit(m, 0);
	// rtp_set_extbit(m, 0);
	// rtp_set_cc(m, 0);
	rtp_set_markbit(m, markbit);
	// rtp_set_payload_type(m, 96);
	// rtp_set_timestamp(m, 0);

	return rtp_session_sendm_with_ts(session, m, userts);
}

uint32_t user_ts = 0;

int ortp_h264_nalu_send(RtpSession *session, uint8_t *nalu, int len)
{
	int markbit = 0;
	int i, k, wbyte = 0;
	int framerate = 50;
	int timestamp = 90000 / framerate;
	int interval = 1000 / framerate;

	struct h264_hdr *h264_hdr = (struct h264_hdr *)nalu;
	struct rtp_h264_hdr *rtp_h264_hdr = (struct rtp_h264_hdr *)h264_packet;
	struct rtp_fua_hdr *rtp_fua_hdr   = (struct rtp_fua_hdr *)h264_packet;
	
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
	
	// if (h264_hdr->nal_unit_type == H264_NALU_TYPE_IPB || h264_hdr->nal_unit_type == H264_NALU_TYPE_IDR) {
	
	if (h264_hdr->nal_unit_type == H264_NALU_TYPE_IDR) {
		usleep(interval * 1000);
	}
	
	return 0;
}

/////////////////////////////////////////////////////////////////


static void ssrc_cb(RtpSession *session) 
{
	pr_debug("hey, the ssrc has changed !\n");
}

static void *ortp_h264_send_thread(void *arg)
{
	uint32_t ts = 0;
	mblk_t *mp = NULL;
	char *packet;
	int packet_size;
	int packet_len = 0;
	RtpSession *session;
	
	int jittcomp = 40;
	bool_t adapt = TRUE;
	
	int lport = 8000;
	char *lipaddr = "127.0.0.1";

	int rport = 12222;
	char *ripaddr = "127.0.0.1";

	char *ssrc;
	uint16_t rtp_version;
	uint16_t rtp_padbit;
	uint16_t rtp_extbit;
	uint16_t rtp_cc;
	uint16_t rtp_markbit;
	uint16_t rtp_paytype;
	
	uint16_t rtp_seq;
	uint32_t rtp_timestamp;
	uint32_t rtp_ssrc;
	
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
	packet_size = 32 * 1024;
	packet = malloc(packet_size);

	printf("sizeof(struct h264_hdr)     = %ld\n", sizeof(struct h264_hdr));
	printf("sizeof(struct rtp_h264_hdr) = %ld\n", sizeof(struct rtp_h264_hdr));
	printf("sizeof(struct rtp_fua_hdr)  = %ld\n", sizeof(struct rtp_fua_hdr));
	printf("sizeof(struct rtp_fub_hdr)  = %ld\n", sizeof(struct rtp_fub_hdr));

	pr_info("%s: >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> <2>ts = %d\n", __func__, ts);
	
	while (1) {
		packet_len = ortp_h264_nalu_packet_read(packet, packet_size);
		pr_info("%s: >>>>> <2> packet_len = %d, packet_size = %d\n", __func__, packet_len, packet_size);
		
		ortp_h264_nalu_send(session, packet, packet_len);
		//usleep(1);
	}
	
	free(packet);
	rtp_session_destroy(session);

	return NULL;
}
thread_initcall(ortp_h264_send_thread);






