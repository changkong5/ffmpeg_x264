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

extern lv_color32_t img_star_map_xx[][640];

void vedio_canvas_update_iii(void);


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
	
	pr_debug("%s: >> packet = %p, size = %d, len = %d\n", __func__, packet, size, len);
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
	
	pr_debug("%s: >> packet = %p\n", __func__, packet);
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

static int yuv420p_argb_init(void)
{
	pr_info("%s: ...\n", __func__);
	async_queue_packet_init();
	
    return 0;
}
fn_initcall(yuv420p_argb_init);

static int yuv420p_argb_exit(void)
{
	pr_info("%s: ...\n", __func__);
	async_queue_packet_deinit();
	
    return 0;
}
fn_exitcall(yuv420p_argb_exit);

int yuv420p_argb_packet_length(void)
{
	return g_async_queue_length(aqueue);
}

int yuv420p_argb_packet_write(uint8_t *data, int len)
{
	pr_info("%s: >>>>>>>>>>>>>>>>>>>>>> <len = %d>\n", __func__, len);

	return async_queue_packet_push(data, len);
}

int yuv420p_argb_packet_read(uint8_t *data, int len)
{
	pr_info("%s: >>>>>>>>>>>>>>>>>>>>>> <len = %d> \n", __func__, len);

	return async_queue_packet_pop(data, len);
}


static void *yuv420p_argb_thread(void *arg)
{
	int i, i_fd, o_fd;
	int rbyte, wbyte;

	AVFrame frame_420p[1];
	int frame_420p_len;
	
	AVFrame frame_argb[1];
	int frame_argb_len;
	struct SwsContext *sws_420p_argb;
	
	// p420p_argb_pipe_open();
	
	frame_420p->width  = VIDEO_FRAME_WIDTH;
    frame_420p->height = VIDEO_FRAME_HEIGHT;
	frame_420p->format = AV_PIX_FMT_YUV420P;
    
    frame_420p_len = av_image_alloc(frame_420p->data, frame_420p->linesize, frame_420p->width, frame_420p->height, frame_420p->format, 1);
    pr_debug("%s: frame_420p_len = %d\n", __func__, frame_420p_len);
	
	
	frame_argb->width  = frame_420p->width;
    frame_argb->height = frame_420p->height;
	// frame_argb->format = AV_PIX_FMT_ARGB;
	frame_argb->format = AV_PIX_FMT_BGRA;
    
	frame_argb_len = av_image_alloc(frame_argb->data, frame_argb->linesize, frame_argb->width, frame_argb->height, frame_argb->format, 1);
	pr_debug("%s: frame_argb_len = %d\n", __func__, frame_argb_len);
	

    
    sws_420p_argb = sws_getContext(frame_420p->width, frame_420p->height, frame_420p->format, 
    							   frame_argb->width, frame_argb->height, frame_argb->format, 
    							   SWS_BICUBIC, NULL, NULL, NULL);
	
	while (1) {
		rbyte = yuv420p_argb_packet_read(frame_420p->data[0], frame_420p_len);
		
		pr_debug("%s: rbyte = %d, frame_420p_len = %d\n", __func__, rbyte, frame_420p_len);
		for (i = 0; i < 9; i++) pr_debug("0x%02x ", frame_420p->data[0][i]); pr_debug("\n");
		
		if (rbyte == 0) {
			continue;
		}
		
		sws_scale(sws_420p_argb, (const uint8_t* const *)frame_420p->data, frame_420p->linesize, 0, 
				  frame_420p->height, frame_argb->data, frame_argb->linesize);
				  

        pr_debug("%s: wbyte = %d, frame_argb_len = %d\n", __func__, wbyte, frame_argb_len);
        for (i = 0; i < 9; i++) pr_debug("0x%02x ", frame_argb->data[0][i]); printf("\n");
 
 		memcpy(img_star_map_xx, frame_argb->data[0], frame_argb_len);
 		vedio_canvas_update_iii();
 		
 		usleep(100 * 1000);
 		pr_debug("%s: ===================================================> frame_argb_len = %d\n", __func__, frame_argb_len);
	}
	 pr_debug("%s: ==================> \n", __func__);
	
	
	av_freep(frame_420p->data);
	// av_frame_free(&frame_420p);
	
	av_freep(frame_argb->data);
	// av_frame_free(&frame_argb);
	
	sws_freeContext(sws_420p_argb);

	return NULL;
}
thread_initcall(yuv420p_argb_thread);


