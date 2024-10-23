#pragma once
#ifndef _INPUT_OUTPUT_H_
#define _INPUT_OUTPUT_H_

#include <stdio.h>
#include <stdint.h>
#include "VideoEncodingHeader.h"

/*************************************************
	Struct:			IOParam
	Description:	���������в���
*************************************************/
typedef struct
{
	FILE *pFin;				//�����YUV�ļ�ָ��
	FILE *pFout;			//����������ļ�ָ��

	char *pNameIn;			//����YUV�ļ���
	char *pNameOut;			//��������ļ���

	uint16_t nImageWidth;	//ͼ����
	uint16_t nImageHeight;	//ͼ��߶�

	uint16_t nFrameRate;	//����֡��
	uint64_t nBitRate;		//��������
	uint16_t nGOPSize;		//һ��GOP��С
	uint16_t nMaxBFrames;	//���B֡����
	uint16_t nTotalFrames;	//������֡��
} IOParam;

/*************************************************
	Function:		Parse_input_param
	Description:	���������д���Ĳ���
	Calls:			��
	Called By:		main
	Input:			(in)argc : Ĭ�������в���
					(in)argv : Ĭ�������в���					
	Output:			(out)io_param : ���������еĽ��
	Return:			true : �����н�����ȷ
					false : �����н�������
*************************************************/
bool Parse_input_param(int argc, char **argv, IOParam &io_param);

/*************************************************
	Function:		Open_file
	Description:	����������ļ�
	Calls:			��
	Called By:		main
	Input:			(in/out)io_param : �����ļ���·��					
	Output:			(in/out)io_param : �ýṹ���Ա������ļ���ָ��
	Return:			true : ����������ļ���ȷ
					false : ����������ļ�����
*************************************************/
bool Open_file(IOParam &io_param);

/*************************************************
	Function:		Close_file
	Description:	�ر���������ļ�
	Calls:			��
	Called By:		main
	Input:			(in)io_param : ������������ļ���ָ��					
	Output:			��
	Return:			��
*************************************************/
void Close_file(IOParam &io_param);

/*************************************************
	Function:		Read_yuv_data
	Description:	�Ӵ򿪵������ļ��ж�ȡyuv���ݣ������浽CodecCtx::AVFrame
	Calls:			��
	Called By:		main
	Input:			(in)io_param : ������������ļ���ָ��	
					(in)color_plane �� ��ɫ������ʶ��0-Y, 1-U, 2-V
	Output:			(out)ctx : ����AVFrame�Ľṹʵ��
	Return:			��
*************************************************/
int Read_yuv_data(CodecCtx &ctx, IOParam &io_param, int color_plane);

#endif