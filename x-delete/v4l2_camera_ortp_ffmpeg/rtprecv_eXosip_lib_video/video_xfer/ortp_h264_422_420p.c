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

#include "lvgl/lvgl.h"

#include "initcall.h"
#include "gdbus/gdbus.h"

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

static int yuyv422_420p_init(void)
{
	pr_info("%s: ...\n", __func__);
	async_queue_packet_init();
	
    return 0;
}
fn_initcall(yuyv422_420p_init);

static int yuyv422_420p_exit(void)
{
	pr_info("%s: ...\n", __func__);
	async_queue_packet_deinit();
	
    return 0;
}
fn_exitcall(yuyv422_420p_exit);

int yuyv422_420p_packet_length(void)
{
	return g_async_queue_length(aqueue);
}

int yuyv422_420p_packet_write(uint8_t *data, int len)
{
	return async_queue_packet_push(data, len);
}

int yuyv422_420p_packet_read(uint8_t *data, int len)
{
	return async_queue_packet_pop(data, len);
}

int h264_decoder_packet_write(uint8_t *data, int len);

static void *yuyv422_420p_thread(void *arg)
{
	int i, rbyte, wbyte;

	AVFrame frame_422[1];
	int frame_422_len;

	AVFrame frame_420p[1];
	int frame_420p_len;
	struct SwsContext *sws_422_420p;
	
	frame_422->width  = VIDEO_FRAME_WIDTH;
    frame_422->height = VIDEO_FRAME_HEIGHT;
	frame_422->format = AV_PIX_FMT_YUYV422;
    
	frame_422_len = av_image_alloc(frame_422->data, frame_422->linesize, frame_422->width, frame_422->height, frame_422->format, 1);
	printf("%s: frame_422_len = %d\n", __func__, frame_422_len);
	
	frame_420p->width  = frame_422->width;
    frame_420p->height = frame_422->height;
	frame_420p->format = AV_PIX_FMT_YUV420P;
    
    frame_420p_len = av_image_alloc(frame_420p->data, frame_420p->linesize, frame_420p->width, frame_420p->height, frame_420p->format, 1);
    printf("%s: frame_420p_len = %d\n", __func__, frame_420p_len);
    
    sws_422_420p = sws_getContext(frame_422->width, frame_422->height, frame_422->format, 
    							   frame_420p->width, frame_420p->height, frame_420p->format, 
    							   SWS_BICUBIC, NULL, NULL, NULL);
	
	while (1) {
		rbyte = yuyv422_420p_packet_read(frame_422->data[0], frame_422_len);
		
		// while (1) usleep(1000);
		printf("%s: rbyte = %d, frame_422_len = %d\n", __func__, rbyte, frame_422_len);
		for (i = 0; i < 9; i++) printf("0x%02x ", frame_422->data[0][i]); printf("\n");
		
		sws_scale(sws_422_420p, (const uint8_t* const *)frame_422->data, frame_422->linesize, 0, 
				  frame_422->height, frame_420p->data, frame_420p->linesize);

        printf("%s: rbyte = %d, frame_420p_len = %d\n", __func__, rbyte, frame_420p_len);
        for (i = 0; i < 9; i++) printf("0x%02x ", frame_420p->data[0][i]); printf("\n");
 
 		h264_decoder_packet_write(frame_420p->data[0], frame_420p_len);
 		// packet_pipe_write(frame_420p->data[0], frame_420p_len);
  

        // usleep(40 * 1000);
	}
	 printf("%s: ==================> \n", __func__);
	
	av_freep(frame_422->data);
	// av_frame_free(&frame_422);
	
	av_freep(frame_420p->data);
	// av_frame_free(&frame_420p);
	
	sws_freeContext(sws_422_420p);

	return NULL;
}
thread_initcall(yuyv422_420p_thread);


