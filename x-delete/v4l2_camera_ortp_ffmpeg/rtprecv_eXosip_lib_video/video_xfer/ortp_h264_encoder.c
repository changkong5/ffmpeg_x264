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

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>

#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>

#include "initcall.h"
#include "gdbus/gdbus.h"



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
#include <libswscale/swscale.h>


#define VIDEO_FRAME_WIDTH	640
#define VIDEO_FRAME_HEIGHT	480

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

static int h264_decoder_init(void)
{
	pr_info("%s: ...\n", __func__);
	async_queue_packet_init();
	
    return 0;
}
fn_initcall(h264_decoder_init);

static int h264_decoder_exit(void)
{
	pr_info("%s: ...\n", __func__);
	async_queue_packet_deinit();
	
    return 0;
}
fn_exitcall(h264_decoder_exit);

int h264_decoder_packet_read(uint8_t *data, int len);
int h264_decoder_packet_write(uint8_t *data, int len);


int h264_decoder_packet_write(uint8_t *data, int len)
{
	return async_queue_packet_push(data, len);
}

int h264_decoder_packet_read(uint8_t *data, int len)
{
	return async_queue_packet_pop(data, len);
}

int yuv420p_argb_packet_write(uint8_t *data, int len);

///////////////////////////////////////////////////////////////////////////////


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
	nalu->date_len = 0;
	nalu->hdr = NULL;
	nalu->next = NULL;  	
	nalu->start = &packet[0];
	nalu->end = &packet[len];
	nalu->total = len;	  
	
	printf("%s: <0000> &packet[0] = %p, &packet[len] = %p, nalu->start = %p, nalu->end = %p, len = %d (0x%08x)\n", __func__, &packet[0], &packet[len], nalu->start, nalu->end, len, len);
		    	
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
			printf("%s: <20> nalu->total = %d, nalu->next_hdr_len = %d, nalu->date_len = %d \n", __func__, nalu->total, nalu->next_hdr_len, nalu->date_len);
			
			if (nalu->hdr == NULL) {
				nalu->hdr = nalu->next;
				nalu->hdr_len = nalu->next_hdr_len;
				continue;
			}
			
			nalu->date_len = nalu->next - nalu->hdr - nalu->next_hdr_len;
			nalu->total -= nalu->date_len;
			printf("%s: <3> nalu->hdr = %p, nalu->next = %p, nalu->hdr_len = %d, nalu->date_len = %d \n", __func__, nalu->hdr, nalu->next, nalu->hdr_len, nalu->date_len);
			
			nalu->nalu[nalu->nalu_num].payload = &nalu->hdr[1 - nalu->hdr_len];
			nalu->nalu[nalu->nalu_num].payload_len = nalu->hdr_len + nalu->date_len;
			nalu->nalu_num++;
			printf("%s: <30> nalu->nalu_num = %d, nalu->nalu[nalu->nalu_num - 1].payload_len = %d \n", __func__, nalu->nalu_num, nalu->nalu[nalu->nalu_num - 1].payload_len);
			
			nalu->hdr = nalu->next;
			nalu->hdr_len = nalu->next_hdr_len;
			printf("%s: <4> nalu->hdr_len = %d, nalu->total = %d\n", __func__, nalu->hdr_len, nalu->total);
		} else {
			printf("%s: <9> nalu->hdr = %p, nalu->end = %p, nalu->hdr_len = %d, offset = %ld (0x%08lx)\n", __func__, nalu->hdr, nalu->end, nalu->hdr_len, (nalu->hdr - packet), (nalu->hdr - packet));
		
			nalu->date_len = nalu->end - 1 - nalu->hdr;
			nalu->nalu[nalu->nalu_num].payload = &nalu->hdr[1 - nalu->hdr_len];
			nalu->nalu[nalu->nalu_num].payload_len = nalu->hdr_len + nalu->date_len;
			nalu->nalu_num++;
			printf("%s: <31> nalu->nalu_num = %d, nalu->nalu[nalu->nalu_num - 1].payload_len = %d , nalu->total = %d\n", 
			__func__, nalu->nalu_num, nalu->nalu[nalu->nalu_num - 1].payload_len, nalu->total);
			
			// end and exit loop
			break;
		}
	}
	
	return nalu->nalu_num;
}


///////////////////////////////////////////////////////////////////////////////

int ortp_h264_nalu_packet_write(uint8_t *data, int len);

static void *h264_encoder_thread(void *arg)
{
	int ret;
	char *buffer = NULL;
	int size, zero_cont;
	int len = 0, rbyte;
	char *data;
	char *buf;
	int buf_len;
	
	int fps = 25;
	int64_t pts = 0;
	int max_packet = 0;
	int max_nalu = 0;
	
	uint8_t *packet_data;
	int packet_len;
	int zero;
	
	int wbyte = 0;
	int framerate = 25;
	int timestamp = 90000 / 25;
	
	AVFrame *frame;
	AVPacket *packet;
	const AVCodec *codec;
	AVCodecContext *codec_ctx;

	codec = avcodec_find_encoder(AV_CODEC_ID_H264);
	if (!codec) {
		pr_err("%s: <avcodec_find_decoder error>\n", __func__);
	}
	
	codec_ctx = avcodec_alloc_context3(codec);
	if (!codec_ctx) {
		pr_err("%s: <avcodec_alloc_context3 error>\n", __func__);
	}
	
/*	
	codec_ctx->flags |= AV_CODEC_FLAG_QSCALE;
	codec_ctx->rc_min_rate =  500000;
	codec_ctx->rc_max_rate = 1500000;
*/

	// pCodecCtx->bit_rate = 400000;
	// codec_ctx->bit_rate = 3000000;
	// codec_ctx->bit_rate = 90000;
	
	codec_ctx->bit_rate = 400000;
	
	codec_ctx->width = VIDEO_FRAME_WIDTH;
    codec_ctx->height = VIDEO_FRAME_HEIGHT;
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
        // av_opt_set(codec_ctx->priv_data, "profile", "baseline", 0); 
        av_opt_set(codec_ctx->priv_data, "profile", "main", 0);
        av_opt_set(codec_ctx->priv_data, "preset", "medium", 0);
        //av_opt_set(codec_ctx->priv_data, "preset", "fast", 0);
        
        // av_opt_set(codec_ctx->priv_data, "tune","fastdecode",0);
        av_opt_set(codec_ctx->priv_data, "tune","zerolatency",0);
    }
	
	packet = av_packet_alloc();

	if (!packet) {
		pr_err("%s: <av_packet_alloc error>\n", __func__);
	}
	
	frame = av_frame_alloc();
	if (!frame) {
		pr_err("%s: <av_frame_alloc error>\n", __func__);
	}

	frame->format = codec_ctx->pix_fmt;
    frame->width  = codec_ctx->width;
    frame->height = codec_ctx->height;
    ret = av_frame_get_buffer(frame, 0);

	ret = avcodec_open2(codec_ctx, codec, NULL);
	if (ret < 0) {
		pr_err("%s: <avcodex_open2 error> ret = %d\n", __func__, ret);
	}

	zero_cont = 0;

	size = av_image_get_buffer_size(frame->format, frame->width, frame->height, 1);
	printf("%s: size = %d\n", __func__, size);
	buffer = av_mallocz(size * sizeof(uint8_t));
	
	int need_size = av_image_fill_arrays(frame->data, frame->linesize, buffer,
                                             frame->format, frame->width, frame->height, 1);
                                             
    printf("%s: size = %d, need_size = %d\n", __func__, size, need_size); 
	pr_info("%s: >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> <2>zero_cont = %d\n", __func__, zero_cont);
	
	while (1) {
		rbyte = h264_decoder_packet_read(buffer, size);
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
        	
        	//ret = write(fd, packet->data, packet->size);
        	//fsync(fd);
  
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
				
				ortp_h264_nalu_packet_write(packet_data, packet_len);
				// ortp_h264_nalu_send(session, packet_data, packet_len);
				// usleep(100 * 1000);
			}
			
			 if (max_nalu < nalu->nalu_num) max_nalu = nalu->nalu_num;
#endif 
			 
        }

	}
	
	printf("%s: <avcodec_send_frame> max_packet = %d, max_nalu = %d\n", __func__, max_packet, max_nalu);

	av_freep(&buffer);
	
	av_packet_free(&packet);
	av_frame_free(&frame);
	avcodec_free_context(&codec_ctx);
	
    return NULL;
}
thread_initcall(h264_encoder_thread);















