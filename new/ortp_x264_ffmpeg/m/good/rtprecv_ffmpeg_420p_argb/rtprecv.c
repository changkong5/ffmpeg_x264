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
 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>

#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
 
#define IMAGE_WIDTH  352	// 320
#define IMAGE_HEIGHT 288	// 240



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

uint8_t h264_packet[32 * 1024];
// uint8_t h264_packet[4096];
int  h264_packet_remain = 0;
int  h264_packet_num = 0;
int  h264_packet_len = 0;
int  h264_packet_total = 0;

// char h264_pps_pkt[8] = {0x00, 0x00, 0x00, 0x01, 0x68, 0xCE, 0x3C, 0x80};
// char h264_sps_pkt[19] = {0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0xC0, 0x1F, 0x8C, 0x8D, 0x40, 0x48, 0x14, 0xB2, 0xF0, 0x0F, 0x08, 0x84, 0x6A};

char rtp_h264_pps_pkt[4] = {0x68, 0xCE, 0x3C, 0x80};
char rtp_h264_sps_pkt[15] = {0x67, 0x42, 0xC0, 0x1F, 0x8C, 0x8D, 0x40, 0x48, 0x14, 0xB2, 0xF0, 0x0F, 0x08, 0x84, 0x6A};

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <sys/stat.h>
#include <pthread.h>
#include <semaphore.h>


struct p420p_argb {
	sem_t sem;
	int pfd[2];
	int run;
	int loop;
};

static struct p420p_argb p420p_argb[1];

static int p420p_argb_pipe_open(void)
{
	return pipe(p420p_argb->pfd);
}

static int p420p_argb_pipe_write(void *data, int len)
{
	uint8_t *buf = data;

	return write(p420p_argb->pfd[1], buf, len);
}

static int p420p_argb_pipe_read(void *data, int len)
{
	uint8_t *buf = data;

	return read(p420p_argb->pfd[0], buf, len);
}

static int p420p_argb_pipe_close(void)
{
	close(p420p_argb->pfd[0]);
	close(p420p_argb->pfd[1]);
	
	return 0;
}

static int p420p_argb_packet_pipe_write(uint8_t *data, int len)
{
	const int max_byte = 4096;
	int ret_byte, offset, wbyte, remain_byte;
	
    offset = 0;
    remain_byte = len;
        
    while (remain_byte > 0) {
		if (remain_byte > max_byte) {
        	wbyte = max_byte;
        } else {
        	wbyte = remain_byte;
       }
        	
       ret_byte = p420p_argb_pipe_write(data + offset, wbyte);
       
       offset += ret_byte;
       remain_byte -= ret_byte;
    }
    
    return offset;
}

static int p420p_argb_packet_pipe_read(uint8_t *data, int len)
{
	const int max_byte = 4096;
    int ret_byte, offset, remain_byte, rbyte;
    
    offset = 0;
	remain_byte = len;
		
	while (remain_byte > 0) {
		if (remain_byte > max_byte) {
			rbyte = max_byte;
		} else {
			rbyte = remain_byte;
		}
			
		ret_byte = p420p_argb_pipe_read(data + offset, rbyte);
			
		offset += ret_byte;
		remain_byte -= ret_byte;
	}
	
	return offset;
}
/////////////////////////////////////////////////////////////////

// ffplay -f rawvideo -s 352x288 yuv420p_w352xh288.yuv 																// correct   // play yuv
// ffplay yuv420p_w352xh288.h264 																					// correct   // play h264
// ffmpeg -s 352x288 -i yuv420p_w352xh288.yuv -c:v h264 yuv420p_w352xh288.h264   									// correct	 // yuv ---> h264
// ffmpeg -c:v h264 -i yuv420p_w352xh288.h264 yuv420p_w352xh288.yuv    												// correct   // h264 ---> yuv


// ffplay -s 352x288 yuv420p_w352xh288.yuv 

// ffplay -pixel_format yuyv422 -s 352x288 yuyv422_w352xh288.yuv 

// ffplay -pixel_format yuv420p -s 352x288 yuv420p_w352xh288.yuv 

// ffplay -pixel_format rgb24 -s 352x288 yuv420p_w352xh288.rgb

// ffplay -pixel_format argb -s 352x288 yuv420p_w352xh288.rgb

static void *frame_420p_argb_thread(void *arg)
{
	int i, i_fd, o_fd;
	int rbyte, wbyte;

	AVFrame frame_420p[1];
	int frame_420p_len;
	
	AVFrame frame_argb[1];
	int frame_argb_len;
	struct SwsContext *sws_420p_argb;
	
	p420p_argb_pipe_open();

	//i_fd = open("yuyv422_w352xh288.yuv", O_RDWR, 0666);
	//o_fd = open("yuv420p_w352xh288.yuv", O_RDWR | O_CREAT, 0666);
	
	o_fd = open("yuv420p_w352xh288.rgb", O_RDWR | O_CREAT, 0666);
	
	frame_420p->width  = 352;
    frame_420p->height = 288;
	frame_420p->format = AV_PIX_FMT_YUV420P;
    
    frame_420p_len = av_image_alloc(frame_420p->data, frame_420p->linesize, frame_420p->width, frame_420p->height, frame_420p->format, 1);
    printf("%s: frame_420p_len = %d\n", __func__, frame_420p_len);
	
	
	frame_argb->width  = frame_420p->width;
    frame_argb->height = frame_420p->height;
	// frame_argb->format = AV_PIX_FMT_ARGB;
	frame_argb->format = AV_PIX_FMT_BGRA;
    
	frame_argb_len = av_image_alloc(frame_argb->data, frame_argb->linesize, frame_argb->width, frame_argb->height, frame_argb->format, 1);
	printf("%s: frame_argb_len = %d\n", __func__, frame_argb_len);
	

    
    sws_420p_argb = sws_getContext(frame_420p->width, frame_420p->height, frame_420p->format, 
    							   frame_argb->width, frame_argb->height, frame_argb->format, 
    							   SWS_BICUBIC, NULL, NULL, NULL);
	
	sleep(1);
	
	while (1) {
		//rbyte = read(i_fd, frame_422->data[0], frame_422_len);
		rbyte = p420p_argb_packet_pipe_read(frame_420p->data[0], frame_420p_len);
		
		// while (1) usleep(1000);
		printf("%s: rbyte = %d, frame_420p_len = %d\n", __func__, rbyte, frame_420p_len);
		for (i = 0; i < 9; i++) printf("0x%02x ", frame_420p->data[0][i]); printf("\n");
		
		if (rbyte == 0) {
			break;
		}
		
		sws_scale(sws_420p_argb, (const uint8_t* const *)frame_420p->data, frame_420p->linesize, 0, 
				  frame_420p->height, frame_argb->data, frame_argb->linesize);
				  
		wbyte = write(o_fd, frame_argb->data[0], frame_argb_len);
        fsync(o_fd);
        
        if (wbyte < 0) {
        	perror("wbyte error\n");
        }
        printf("%s: wbyte = %d, frame_argb_len = %d\n", __func__, wbyte, frame_argb_len);
        for (i = 0; i < 9; i++) printf("0x%02x ", frame_argb->data[0][i]); printf("\n");
 

        // usleep(40 * 1000);
        //lseek(i_fd, 0, SEEK_SET);
	}
	 printf("%s: ==================> \n", __func__);
	
	p420p_argb_pipe_close();
	
	//close(i_fd);
	close(o_fd);
	
	av_freep(frame_420p->data);
	// av_frame_free(&frame_420p);
	
	av_freep(frame_argb->data);
	// av_frame_free(&frame_argb);
	
	sws_freeContext(sws_420p_argb);

	return NULL;
}

/////////////////////////////////////////////////////////////////


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <sys/stat.h>
#include <pthread.h>
#include <semaphore.h>


struct emulcap {
	sem_t sem;
	int pfd[2];
	int run;
	int loop;
	int delay;
	int frame_len;
	int frame_count;
};


static struct emulcap emulcap[1];

static int emulcap_pipe_open(void)
{
	return pipe(emulcap->pfd);
}

static int emulcap_pipe_write(void *data, int len)
{
	uint8_t *buf = data;

	// pr_debug("%s: <emulcap->pfd[1] = %d, len = %d> \n", __func__, emulcap->pfd[1], len);

	return write(emulcap->pfd[1], buf, len);
}

static int emulcap_pipe_read(void *data, int len)
{
	uint8_t *buf = data;

	return read(emulcap->pfd[0], buf, len);
}

static int emulcap_pipe_close(void)
{
	close(emulcap->pfd[0]);
	close(emulcap->pfd[1]);

	return 0;
}
/////////////////////////////////////////////////////////////////



// ffplay -f rawvideo -s 352x288 yuv420p_w352xh288.yuv 																// correct   // play yuv
// ffplay yuv420p_w352xh288.h264 																					// correct   // play h264
// ffmpeg -s 352x288 -i yuv420p_w352xh288.yuv -c:v h264 yuv420p_w352xh288.h264   									// correct	 // yuv ---> h264
// ffmpeg -c:v h264 -i yuv420p_w352xh288.h264 yuv420p_w352xh288.yuv    												// correct   // h264 ---> yuv

// int main111(int argc, int *argv[])
static void *emulcap_thread(void *arg)
{
	int fd, ret;
	int i_fd, got_picture;
	AVCodec *codec;
	AVCodecContext *codec_context;
	AVCodecParserContext *codec_parser;
	// AVFormatContext *format_context = NULL;
	AVFrame *frame;
	AVPacket *packet;
	char *buffer = NULL;
	int size, zero_cont;
	int len = 0, rbyte;
	uint8_t buf[4096];
	char *data = buf;

	fd = open("yuv420p_w352xh288.yuv", O_RDWR | O_CREAT, 0666);
	
	// i_fd = open("yuv420p_w352xh288.h264", O_RDWR, 0666);
	emulcap_pipe_open();

	// codec = avcodec_find_decoder_by_name("h264");
	codec = avcodec_find_decoder(AV_CODEC_ID_H264);

	if (!codec) {
		printf("%s: <avcodec_find_decoder error>\n", __func__);
	}
	
	codec_context = avcodec_alloc_context3(codec);
	
	if (!codec_context) {
		printf("%s: <avcodec_alloc_context3 error>\n", __func__);
	}

//	if (codec->capabilities & AV_CODEC_CAP_TRUNCATED) {
//		codec_context->flags |= AV_CODEC_FLAG_TRUNCATED; // we do not send complete frames
//	}

	codec_parser = av_parser_init(codec->id);
	
	if (!codec_parser) {
		printf("%s: <av_parser_init error>\n", __func__);
	}

	packet = av_packet_alloc();

	if (!packet) {
		printf("%s: <av_packet_alloc error>\n", __func__);
	}
	
	frame = av_frame_alloc();

	if (!frame) {
		printf("%s: <av_frame_alloc error>\n", __func__);
	}

	ret = avcodec_open2(codec_context, codec, NULL);

	if (ret < 0) {
		printf("%s: <avcodex_open2 error> ret = %d\n", __func__, ret);
	}
	
	zero_cont = 0;
	
	//size = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, IMAGE_WIDTH, IMAGE_HEIGHT, 1);
	//printf("%s: size = %d\n", __func__, size);
	//buffer = av_mallocz(size * sizeof(uint8_t));
	
	printf("%s: >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> \n", __func__);
	
	while (1) {
		rbyte = emulcap_pipe_read(buf, sizeof(buf));
		// rbyte = read(i_fd, buf, sizeof(buf));
		printf("%s: zero_cont = %d, len = %d, rbyte = %d\n", __func__, zero_cont, len, rbyte);
		
		if (rbyte == 0) {
			zero_cont++;
			
			if (len == 0) {
				break;
			} else {
				if (zero_cont == 1) {
					rbyte = len;
				} else {
					break;
				}
			}
		}
		
		data = buf;
		while (rbyte > 0) {
			len = av_parser_parse2(codec_parser, codec_context, 
										&packet->data, &packet->size, 
										data, rbyte, 
										AV_NOPTS_VALUE, AV_NOPTS_VALUE, AV_NOPTS_VALUE);
										
			rbyte -= len;
			data  += len;
			printf("%s: zero_cont = %d, rbyte = %d, len = %d, packet->size = %d\n", __func__, zero_cont, rbyte, len, packet->size);
										
			if (packet->size == 0) {
				continue;
			}
			for (int i = 0; i < 9; i++) printf("0x%02x ", packet->data[i]); printf("\n");
			
			ret = avcodec_send_packet(codec_context, packet);
			
			if (ret < 0) {
				char errbuf[1024]; 
        		av_strerror(ret, errbuf, sizeof (errbuf));
				printf("%s: <avcodec_send_packet error> ret = %d\n", __func__, ret);
				return 0;
			}
			
			while (1) {
				ret = avcodec_receive_frame(codec_context, frame);
				// printf("ret = %d\n", ret);
				
				if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
					break;
        		} else if (ret < 0) {
        			char errbuf[1024]; 
            		av_strerror(ret, errbuf, sizeof (errbuf));
        			return 0;
        		}
        		printf("%s: AV_PIX_FMT_YUV420P = %d, frame->width = %d, frame->height = %d, frame->format = %d\n", __func__, AV_PIX_FMT_YUV420P, frame->width, frame->height, frame->format);

				if (1 && !buffer) {
					size = av_image_get_buffer_size(frame->format, frame->width, frame->height, 1);
					printf("%s: >>>> size = %d\n", __func__, size);
					buffer = av_mallocz(size * sizeof(uint8_t));
				}

        		av_image_copy_to_buffer(buffer, size, (const uint8_t * const *)frame->data, frame->linesize, frame->format, frame->width, frame->height, 1);
        		
        		// avpicture_layout((AVPicture *)frame, AV_PIX_FMT_YUV420P, IMAGE_WIDTH, IMAGE_HEIGHT, buffer, size);
				ret = write(fd, buffer, size);
				
				p420p_argb_packet_pipe_write(buffer, size);
				printf("%s: ================ >>>> size = %d\n", __func__, size);
			}
		}
	}
	printf("%s: <00> rbyte = %d, len = %d, packet->size = %d\n", __func__, rbyte, len, packet->size);

	// close(i_fd);
	emulcap_pipe_close();
	close(fd);
	av_freep(&buffer);

	av_packet_free(&packet);
	av_frame_free(&frame);
	av_parser_close(codec_parser);
	avcodec_free_context(&codec_context);
	
	
	return 0;
}
 

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
		
		emulcap_pipe_write(h264_packet, h264_packet_total);
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
			emulcap_pipe_write(h264_packet, h264_packet_total);
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
	
	int framerate = 25;
	int timestamp = 90000 / 25;
	
	out_file = fopen("yuv420p_w352xh288.h264", "wb+");
	// out_file = fopen("yuv420p_w352xh288.h264", "ab+");
	
	pthread_t tid;
	
	pthread_create(&tid, NULL, emulcap_thread, NULL);
	pthread_detach(tid);
	
	pthread_t tid_420p_argb;
	pthread_create(&tid_420p_argb, NULL, frame_420p_argb_thread, NULL);
	pthread_detach(tid_420p_argb);
	
	usleep(1000);
	// pthread_detach(pthread_self());
	
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

	ortp_global_stats_display();

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




