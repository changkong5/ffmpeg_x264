#pragma once
#ifndef _DEMUXING_CONTEXT_H_
#define _DEMUXING_CONTEXT_H_

#include "VideoDemuxingHeader.h"
#include "IOFile.h"

/*************************************************
Struct:			DemuxingVideoAudioContex
Description:	����⸴�����ͽ����������������
*************************************************/
typedef struct
{
	AVFormatContext *fmt_ctx;
	AVCodecContext *video_dec_ctx, *audio_dec_ctx;
	AVStream *video_stream, *audio_stream;
	AVFrame *frame;
	AVPacket pkt;

	int video_stream_idx, audio_stream_idx;
	int width, height;

	uint8_t *video_dst_data[4];
	int video_dst_linesize[4];
	int video_dst_bufsize;
	enum AVPixelFormat pix_fmt;
} DemuxingVideoAudioContex;

/*************************************************
Function:		InitDemuxContext
Description:	��������������ñ�����������
Calls:			open_codec_context
Called By:		main
Input:			(in)files : �������
Output:			(out)va_ctx : �����������Ĳ���
Return:			������
*************************************************/
int InitDemuxContext(IOFileName &files, DemuxingVideoAudioContex &va_ctx);

/*************************************************
Function:		CloseDemuxContext
Description:	�رձ�����������
Calls:			��
Called By:		main
Input:			(in)files : �������
Output:			(out)va_ctx : �����������Ĳ���
Return:			��
*************************************************/
void CloseDemuxContext(IOFileName &files, DemuxingVideoAudioContex &va_ctx);

#endif