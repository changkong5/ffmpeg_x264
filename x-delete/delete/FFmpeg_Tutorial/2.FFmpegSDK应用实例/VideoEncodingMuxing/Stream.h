#ifndef _STREAM_H_
#define _STREAM_H_

#include "common.h"

#define HAVE_VIDEO		1			//Ŀ���ļ�������Ƶ
#define ENCODE_VIDEO	1 << 1		//������Ƶ����
#define HAVE_AUDIO		1 << 2		//Ŀ���ļ�������Ƶ
#define ENCODE_AUDIO	1 << 3		//������Ƶ����

typedef struct OutputStream 
{
	AVStream *st;

	/* pts of the next frame that will be generated */
	int64_t next_pts;
	int samples_count;

	AVFrame *frame;
	AVFrame *tmp_frame;

	float t, tincr, tincr2;

	struct SwsContext *sws_ctx;
	struct SwrContext *swr_ctx;
} OutputStream;

int Add_audio_video_streams(OutputStream *video_st, OutputStream *audio_st, AVFormatContext *oc, AVOutputFormat *fmt, AVCodec *audio_codec, AVCodec *video_codec, IOParam &io);

void Close_stream(AVFormatContext *oc, OutputStream *ost);

#endif