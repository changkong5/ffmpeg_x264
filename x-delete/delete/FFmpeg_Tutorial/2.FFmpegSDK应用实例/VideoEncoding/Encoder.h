#pragma once
#ifndef _ENCODER_H_
#define _ENCODER_H_
#include "VideoEncodingHeader.h"
#include "InputOutput.h"

/*************************************************
	Function:		Open_encoder
	Description:	���������������AVCodecContext������AVCodec�Ƚṹ����
	Calls:			avcodec_register_all
					avcodec_find_encoder
					avcodec_alloc_context3
					setContext
					avcodec_open2
					av_frame_alloc
					av_image_alloc
	Called By:		main
	Input:			(in)io_param : ������������ļ���ָ��					
	Output:			(out)ctx : ���������������Ľṹ����
	Return:			true : �򿪸��������������ɹ�
					false : �򿪱���������ʧ��
*************************************************/
bool Open_encoder(CodecCtx &ctx, IOParam io_param);

/*************************************************
	Function:		Close_encoder
	Description:	�رձ����������Ĳ���
	Calls:			avcodec_close
					av_free
					av_freep
					av_frame_free
	Called By:		main
	Input:			(in)ctx : �����������Ĳ���					
	Output:			��
	Return:			��
*************************************************/
void Close_encoder(CodecCtx &ctx);

#endif