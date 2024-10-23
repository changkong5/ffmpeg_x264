#pragma once
#ifndef _DECODER_H_
#define _DECODER_H_
#include "VideoDecodingHeader.h"

/*************************************************
Function:		Open_deocder
Description:	����������ļ�
Calls:			��
Called By:		main
Input:			(in/out)ctx : ע���������ṹ����AVCodec��AVCodecContext��AVCodecParserContext�Ƚṹ
Output:			(in/out)ctx : �ýṹ���Ա������ļ���ָ��
Return:			true : ����������ļ���ȷ
				false : ����������ļ�����
*************************************************/
bool Open_deocder(CodecCtx &ctx);

/*************************************************
Function:		Close_decoder
Description:	����������ļ�
Calls:			��
Called By:		main
Input:			(in/out)ctx : �������ṹ
Output:			(in/out)ctx : �������ṹ
Return:			��
*************************************************/
void Close_decoder(CodecCtx &ctx);

#endif