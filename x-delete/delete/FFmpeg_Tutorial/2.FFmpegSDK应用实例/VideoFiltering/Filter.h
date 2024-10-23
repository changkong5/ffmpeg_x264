#ifndef _FILTER_H_
#define _FILTER_H_

/*************************************************
	Function:		Init_video_filter
	Description:	��ʼ��video filter��صĽṹ
	Calls:			��
	Called By:		main
	Input:			(in)filter_descr : ָ����filter����
					(in)width : ����ͼ����
					(in)height : ����ͼ��߶�
	Output:			��
	Return:			0 : video filter��ʼ����ȷ
					< 0 : video filter��ʼ����ȷ
*************************************************/
int Init_video_filter(const char *filter_descr, int width, int height);

/*************************************************
	Function:		Add_frame_to_filter
	Description:	�������������frame��ӽ�filter graph
	Calls:			��
	Called By:		main
	Input:			(in)frameIn : �����������frame
	Output:			��
	Return:			true : �������frame��ȷ
					false : �������frame��ȷ
*************************************************/
bool Add_frame_to_filter(AVFrame *frameIn);

/*************************************************
	Function:		Get_frame_from_filter
	Description:	��filter graph�л�ȡ���frame
	Calls:			��
	Called By:		main
	Input:			��
	Output:			(in)frameOut : ����������ص�frame
	Return:			0 : �������frame��ȷ
					< 0 : �������frame��ȷ
*************************************************/
int Get_frame_from_filter(AVFrame **frameOut);

/*************************************************
	Function:		Close_video_filter
	Description:	�ر�video filter��ؽṹ
	Calls:			��
	Called By:		main
	Input:			��
	Output:			��
	Return:			��
*************************************************/
void Close_video_filter();

#endif