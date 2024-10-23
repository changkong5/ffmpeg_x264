#pragma once
#ifndef _INPUT_OUTPUT_H_
#define _INPUT_OUTPUT_H_

/*************************************************
Struct:			IOParam
Description:	���������в���
*************************************************/
typedef struct
{
	FILE *pFin;
	FILE *pFout;

	char *pNameIn;
	char *pNameOut;
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
void Parse(int argc, char **argv, IOParam &IOParam);

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
bool Open_files(IOParam &IOParam);

/*************************************************
Function:		Close_file
Description:	�ر���������ļ�
Calls:			��
Called By:		main
Input:			(in)io_param : ������������ļ���ָ��
Output:			��
Return:			��
*************************************************/
void Close_files(IOParam &IOParam);

#endif