#ifndef _ENCODING_MUXING_HEADER_H_
#define _ENCODING_MUXING_HEADER_H_

//#define snprintf(buf,len, format,...) _snprintf_s(buf, len,len, format, __VA_ARGS__)

extern "C"  
{  
	#include "libavcodec/avcodec.h"  
	#include "libavformat/avformat.h"  
	#include "libavfilter/avfiltergraph.h"  
	#include "libavfilter/buffersink.h"  
	#include "libavfilter/buffersrc.h"  
	#include "libavutil/avutil.h"  
	#include "libswscale/swscale.h"
	#include "libswresample/swresample.h"

	#include "libavutil/avassert.h"
	#include "libavutil/channel_layout.h"
	#include "libavutil/opt.h"
	#include "libavutil/mathematics.h"
	#include "libavutil/timestamp.h"
};  

/*************************************************
	Struct:			IOParam
	Description:	���������в���
*************************************************/
typedef struct _IOParam
{
	const char *input_file_name;	//����������ļ���
	const char *output_file_name;	//����ķ�װ��Ƶ�ļ���
	int frame_width;				//��Ƶ֡���
	int frame_height;				//��Ƶ֡�߶�
} IOParam;

#endif