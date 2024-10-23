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


int h264_decoder_packet_write(uint8_t *data, int len)
{
	return async_queue_packet_push(data, len);
}

int h264_decoder_packet_read(uint8_t *data, int len)
{
	return async_queue_packet_pop(data, len);
}

int yuv420p_argb_packet_write(uint8_t *data, int len);

// -decoders           show available decoders
// -encoders           show available encoders

static void *h264_decoder_thread(void *arg)
{
	int ret;
	char *buffer = NULL;
	int size, zero_cont;
	int len = 0, rbyte;
	char *data;
	char *buf;
	int buf_len;
	//uint8_t buf[8192];
	//char *data = buf;
	
	AVFrame *frame;
	AVPacket *packet;
	const AVCodec *codec;
	AVCodecContext *codec_context;
	AVCodecParserContext *codec_parser;

	codec = avcodec_find_decoder(AV_CODEC_ID_H264);
	if (!codec) {
		pr_err("%s: <avcodec_find_decoder error>\n", __func__);
	}
	
	codec_context = avcodec_alloc_context3(codec);
	if (!codec_context) {
		pr_err("%s: <avcodec_alloc_context3 error>\n", __func__);
	}
	
	codec_parser = av_parser_init(codec->id);
	if (!codec_parser) {
		pr_err("%s: <av_parser_init error>\n", __func__);
	}
	
	packet = av_packet_alloc();

	if (!packet) {
		pr_err("%s: <av_packet_alloc error>\n", __func__);
	}
	
	frame = av_frame_alloc();
	if (!frame) {
		pr_err("%s: <av_frame_alloc error>\n", __func__);
	}

	ret = avcodec_open2(codec_context, codec, NULL);
	if (ret < 0) {
		pr_err("%s: <avcodex_open2 error> ret = %d\n", __func__, ret);
	}
	
	buf_len = 32 * 1024;
	buf = malloc(buf_len);
	if (!buf) {
		pr_err("%s: <malloc error>\n", __func__);
	}
	data = buf;
	
	zero_cont = 0;
	pr_info("%s: >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> <2>zero_cont = %d\n", __func__, zero_cont);
	
	while (1) {
		// rbyte = emulcap_pipe_read(buf, sizeof(buf));
		// rbyte = h264_decoder_packet_read(buf, sizeof(buf));
		rbyte = h264_decoder_packet_read(buf, buf_len);
		
		pr_debug("%s: zero_cont = %d, len = %d, rbyte = %d\n", __func__, zero_cont, len, rbyte);
		
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
			pr_debug("%s: zero_cont = %d, rbyte = %d, len = %d, packet->size = %d\n", __func__, zero_cont, rbyte, len, packet->size);
										
			if (packet->size == 0) {
				continue;
			}
			for (int i = 0; i < 9; i++) pr_debug("0x%02x ", packet->data[i]); pr_debug("\n");
			
			ret = avcodec_send_packet(codec_context, packet);
			
			if (ret < 0) {
				char errbuf[1024]; 
        		av_strerror(ret, errbuf, sizeof (errbuf));
				pr_err("%s: <avcodec_send_packet error> ret = %d, errbuf = %s\n", __func__, ret, errbuf);
				
				break;
				// return 0;
			}
			
			while (1) {
				ret = avcodec_receive_frame(codec_context, frame);
				// pr_debug("ret = %d\n", ret);
				
				if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
					break;
        		} else if (ret < 0) {
        			char errbuf[1024]; 
            		av_strerror(ret, errbuf, sizeof (errbuf));
            		pr_err("%s: <avcodec_send_packet error> ret = %d, errbuf = %s\n", __func__, ret, errbuf);
            		
            		break;
        			// return 0;
        		}
        		pr_debug("%s: AV_PIX_FMT_YUV420P = %d, frame->width = %d, frame->height = %d, frame->format = %d\n", __func__, AV_PIX_FMT_YUV420P, frame->width, frame->height, frame->format);

				if (1 && !buffer) {
					size = av_image_get_buffer_size(frame->format, frame->width, frame->height, 1);
					pr_debug("%s: >>>> size = %d\n", __func__, size);
					buffer = av_mallocz(size * sizeof(uint8_t));
				}

        		av_image_copy_to_buffer(buffer, size, (const uint8_t * const *)frame->data, frame->linesize, frame->format, frame->width, frame->height, 1);
				
				yuv420p_argb_packet_write(buffer, size);
				// p420p_argb_packet_pipe_write(buffer, size);
				pr_debug("%s: ================ >>>> size = %d\n", __func__, size);
			}
		}
	}
	printf("%s: <00> rbyte = %d, len = %d, packet->size = %d\n", __func__, rbyte, len, packet->size);

	// emulcap_pipe_close();

	free(buf);
	av_freep(&buffer);

	av_packet_free(&packet);
	av_frame_free(&frame);
	av_parser_close(codec_parser);
	avcodec_free_context(&codec_context);
	
    return NULL;
}
thread_initcall(h264_decoder_thread);















