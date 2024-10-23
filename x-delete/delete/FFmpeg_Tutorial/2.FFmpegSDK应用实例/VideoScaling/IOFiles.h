#ifndef _IO_FILES_H_
#define _IO_FILES_H_

/*************************************************
	Struct:			IOFiles
	Description:	���������в���
*************************************************/
typedef struct _IOFiles
{
	char *inputName;			//�����ļ���
	char *outputName;			//����ļ���
	char *inputFrameSize;		//����ͼ��ĳߴ�
	char *outputFrameSize;		//���ͼ��ĳߴ�

	FILE *iFile;				//�����ļ�ָ��
	FILE *oFile;				//����ļ�ָ��

} IOFiles;
 

#endif