#ifndef _FRAME_H_
#define _FRAME_H_

/*************************************************
	Function:		Init_video_frame_in_out
	Description:	��ʼ���������frame��������ػ���
	Calls:			��
	Called By:		main
	Input:			(in)frameWidth : ͼ����
					(in)frameHeight : ͼ��߶�
	Output:			(out)frameIn : ����frame
					(out)frameOut : ���frame
					(out)frame_buffer_in ������frame�����ػ���
					(out)frame_buffer_out�����frame�����ػ���
	Return:			��
*************************************************/
void Init_video_frame_in_out(AVFrame **frameIn, AVFrame **frameOut, unsigned char **frame_buffer_in, unsigned char **frame_buffer_out, int frameWidth, int frameHeight);

#endif