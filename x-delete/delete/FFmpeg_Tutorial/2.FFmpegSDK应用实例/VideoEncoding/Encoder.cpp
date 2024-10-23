#include "Encoder.h"
#include "VideoEncodingHeader.h"

/*************************************************
	Function:		setContext
	Description:	��������������ñ�����������
	Calls:			av_opt_set
	Called By:		Open_encoder
	Input:			(in)io_param : �������������
	Output:			(out)ctx : �����������Ĳ���					
	Return:			��
*************************************************/
void setContext(CodecCtx &ctx, IOParam io_param)
{
	/* put sample parameters */
	ctx.c->bit_rate = io_param.nBitRate;

    /* resolution must be a multiple of two */
	ctx.c->width = io_param.nImageWidth;
	ctx.c->height = io_param.nImageHeight;

    /* frames per second */
	AVRational rational = {1,25};
	ctx.c->time_base = rational;
	    
	ctx.c->gop_size = io_param.nGOPSize;
	ctx.c->max_b_frames = io_param.nMaxBFrames;
	ctx.c->pix_fmt = AV_PIX_FMT_YUV420P;

	av_opt_set(ctx.c->priv_data, "preset", "slow", 0);
}

bool Open_encoder(CodecCtx &ctx, IOParam io_param)
{
	int ret;

	avcodec_register_all();								//ע���������������Ƶ�������

	/* find the mpeg1 video encoder */
	ctx.codec = avcodec_find_encoder(AV_CODEC_ID_H264);	//����CODEC_ID���ұ����������ʵ����ָ��
	if (!ctx.codec) 
	{
		fprintf(stderr, "Codec not found\n");
		return false;
	}

	ctx.c = avcodec_alloc_context3(ctx.codec);			//����AVCodecContextʵ��
	if (!ctx.c)
	{
		fprintf(stderr, "Could not allocate video codec context\n");
		return false;
	}

	setContext(ctx,io_param);							//���ñ�������������

	/* open it */
	if (avcodec_open2(ctx.c, ctx.codec, NULL) < 0)		//���ݱ����������Ĵ򿪱�����
	{
		fprintf(stderr, "Could not open codec\n");
		exit(1);
	}

	ctx.frame = av_frame_alloc();						//����AVFrame����
	if (!ctx.frame) 
	{
        fprintf(stderr, "Could not allocate video frame\n");
        return false;
    }
	ctx.frame->format = ctx.c->pix_fmt;
	ctx.frame->width = ctx.c->width;
	ctx.frame->height = ctx.c->height;

    /* the image can be allocated by any means and av_image_alloc() is
     * just the most convenient way if av_malloc() is to be used */
	//����AVFrame�����������ش洢�ռ�
	ret = av_image_alloc(ctx.frame->data, ctx.frame->linesize, ctx.c->width, ctx.c->height, ctx.c->pix_fmt, 32);
	if (ret < 0) 
	{
		fprintf(stderr, "Could not allocate raw picture buffer\n");
		return false;
	}

	return true;
}

void Close_encoder(CodecCtx &ctx)
{
	avcodec_close(ctx.c);
	av_free(ctx.c);
	av_freep(&(ctx.frame->data[0]));
	av_frame_free(&(ctx.frame));
}

