#ifndef _IO_FILES_H_
#define _IO_FILES_H_

/*************************************************
	Struct:			IOParam
	Description:	���������в���
*************************************************/
typedef struct _IOFiles
{
	const char *inputFileName;		//�����ļ���
	const char *outputFileName;		//����ļ���

	FILE *iFile;					//�����ļ�ָ��
	FILE *oFile;					//����ļ�ָ��

	uint8_t filterIdx;				//Filter����

	unsigned int frameWidth;		//ͼ����
	unsigned int frameHeight;		//ͼ��߶�
}IOFiles;

/*************************************************
	Function:		Read_yuv_data_to_buf
	Description:	������yuv�ļ��ж�ȡ���ݵ�buffer��frame�ṹ
	Calls:			��
	Called By:		main
	Input:			(in)files : ���������ļ��Ľṹ
	Output:			(out)frame_buffer_in : ָ�򻺴�����ָ��	
					(out)frameIn : ָ������frame��ָ��
	Return:			��
*************************************************/
bool Read_yuv_data_to_buf(unsigned char *frame_buffer_in, const IOFiles &files, AVFrame **frameIn);

/*************************************************
	Function:		Write_yuv_to_outfile
	Description:	�����frame��д���������ݵ�����ļ�
	Calls:			��
	Called By:		main
	Input:			(in)frame_out : filter graph�����frame
	Output:			(out)files : ��������ļ��Ľṹ	
	Return:			��
*************************************************/
void Write_yuv_to_outfile(const AVFrame *frame_out, IOFiles &files);

#endif