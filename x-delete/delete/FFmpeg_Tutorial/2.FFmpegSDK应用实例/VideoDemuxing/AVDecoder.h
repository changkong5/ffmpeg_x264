#ifndef _AV_DECODER_H_
#define _AV_DECODER_H_

#include "IOFile.h"
#include "DemuxingContext.h"

/*************************************************
Function:		Get_format_from_sample_fmt
Description:	��ȡ��Ƶ������ʽ
Calls:			FFMpeg API
Called By:		main
Input:			(in)sample_fmt : ������ʽ�ṹ��
Output:			(out)fmt : ��ʶ������ʽ���ַ���
Return:			������
*************************************************/
int Get_format_from_sample_fmt(const char **fmt, enum AVSampleFormat sample_fmt);

/*************************************************
Function:		Decode_packet
Description:	����һ����Ƶ����Ƶ���ݰ�
Calls:			FFMpeg API
Called By:		main
Input:			(in)files : �������
				(in)va_ctx : �����������Ĳ���
				(in)cached : �Ƿ���������еĻ�������
Output:			(out)got_frame : ��������һ֡��ʶ
Return:			������
*************************************************/
int Decode_packet(IOFileName &files, DemuxingVideoAudioContex &va_ctx, int *got_frame, int cached);

#endif