#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>

#if 0
#define DECODED_OUTPUT_FORMAT  AV_PIX_FMT_YUV420P

#define INPUT_FILE_NAME "encode_w352xh288.h264"
#define OUTPUT_FILE_NAME "decode_yuv420p_w352xh288.yuv"
#define IMAGE_WIDTH  352	// 320
#define IMAGE_HEIGHT 288	// 240
 
void
error_handle(const char *errorInfo ){
	printf("%s error!\n",errorInfo);
	exit(EXIT_FAILURE);
}


// ffplay -f rawvideo -video_size 352x288 yuv_420-352x288.yuv
// ffplay -f rawvideo -video_size 352x288 decode_yuv420p_w352xh288.yuv
 
int
main111(int argc,char ** argv)
{
	int  write_fd,ret,videoStream;
	AVFormatContext * formatContext=NULL;
	AVCodec * codec;
	AVCodecContext * codecContext;
	AVFrame * decodedFrame;
	AVPacket packet;
	uint8_t *decodedBuffer;
	unsigned int decodedBufferSize;
	int finishedFrame;
 
	av_register_all();
 
 
	write_fd=open(OUTPUT_FILE_NAME,O_RDWR | O_CREAT,0666);						// #define OUTPUT_FILE_NAME "decode_yuv420p_w352xh288.yuv"
	if(write_fd<0){
		perror("open");
		exit(1);
	}
 
	ret=avformat_open_input(&formatContext, INPUT_FILE_NAME, NULL,NULL);		// #define INPUT_FILE_NAME "encode_w352xh288.h264"
	if(ret<0)
		error_handle("avformat_open_input error");
 
	ret=avformat_find_stream_info(formatContext,NULL);
	if(ret<0)
		error_handle("av_find_stream_info");
 
 
	videoStream=0;
	codecContext=formatContext->streams[videoStream]->codec;
 
	codec=avcodec_find_decoder(AV_CODEC_ID_H264);
	if(codec==NULL)
		error_handle("avcodec_find_decoder error!\n");
 
	ret=avcodec_open2(codecContext,codec,NULL);
	if(ret<0)
		error_handle("avcodec_open2");
 
	decodedFrame=av_frame_alloc();
	if(!decodedFrame)
		error_handle("avcodec_alloc_frame!");
 
	decodedBufferSize=avpicture_get_size(DECODED_OUTPUT_FORMAT,IMAGE_WIDTH,IMAGE_HEIGHT);		// #define DECODED_OUTPUT_FORMAT  AV_PIX_FMT_YUV420P
	decodedBuffer=(uint8_t *)malloc(decodedBufferSize);
	if(!decodedBuffer)
		error_handle("malloc decodedBuffer error!");
 
	av_init_packet(&packet);
	while(av_read_frame(formatContext,&packet)>=0){
	
	
			printf("%s: packet.stream_index = %d, packet.size = %d\n", __func__, packet.stream_index, packet.size);
			
			for (int i = 0; i < packet.size; i++) {
				//printf("0x%02x ", packet.data[i]);
				
				if (i > 0 && (i % 16) == 0) {
					//printf("\n");
				}
			}
			//printf("\n");
			
			
			
	
			ret=avcodec_decode_video2(codecContext,decodedFrame,&finishedFrame,&packet);
			if(ret<0)
				error_handle("avcodec_decode_video2 error!");
			if(finishedFrame){
				avpicture_layout((AVPicture*)decodedFrame,DECODED_OUTPUT_FORMAT,IMAGE_WIDTH,IMAGE_HEIGHT,decodedBuffer,decodedBufferSize);
				

				printf("%s: decodedBufferSize = %d, total = %d\n", __func__, decodedBufferSize, IMAGE_WIDTH*IMAGE_HEIGHT * 3 / 2);
				
				// ret=write(write_fd,decodedBuffer,decodedBufferSize);
				if(ret<0)
					error_handle("write yuv stream error!");
			}
 
		av_free_packet(&packet);
	}
 
	while(1){
		packet.data=NULL;
		packet.size=0;
		ret=avcodec_decode_video2(codecContext,decodedFrame,&finishedFrame,&packet);
		if(ret<=0 && (finishedFrame<=0))
			break;
		if(finishedFrame){
			avpicture_layout((AVPicture*)decodedFrame,DECODED_OUTPUT_FORMAT,IMAGE_WIDTH,IMAGE_HEIGHT,decodedBuffer,decodedBufferSize);
			// ret=write(write_fd,decodedBuffer,decodedBufferSize);
			if(ret<0)
				error_handle("write yuv stream error!");
		}
 
		av_free_packet(&packet);
	}
 
 
	avformat_close_input(&formatContext);
	free(decodedBuffer);
	av_free(decodedFrame);
	avcodec_close(codecContext);
 
	return 0;
}




#if 0


#include <stdio.h>

#include "InputOutput.h"
#include "Decoder.h"


void write_out_yuv_frame(const CodecCtx &ctx, IOParam &in_out)
{
	uint8_t **pBuf	= ctx.frame->data;
	int*	pStride = ctx.frame->linesize;
	
	for (int color_idx = 0; color_idx < 3; color_idx++)
	{
		int		nWidth	= color_idx == 0 ? ctx.frame->width : ctx.frame->width / 2;
		int		nHeight = color_idx == 0 ? ctx.frame->height : ctx.frame->height / 2;
		for(int idx=0;idx < nHeight; idx++)
		{
			fwrite(pBuf[color_idx],1, nWidth, in_out.pFout);
			pBuf[color_idx] += pStride[color_idx];
		}
		fflush(in_out.pFout);
	}
}

bool Open_deocder(CodecCtx &ctx)
{
	//注册编解码器对象
	avcodec_register_all();	

	//初始化AVPacket对象
	av_init_packet(&(ctx.pkt));

	//根据CODEC_ID查找AVCodec对象
	ctx.pCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
	if (!ctx.pCodec) 
	{
		fprintf(stderr, "Codec not found\n");
		return false;
	}

	//根据AVCodec对象分配AVCodecContext
	ctx.pCodecContext = avcodec_alloc_context3(ctx.pCodec);
	if (!ctx.pCodecContext)
	{
		fprintf(stderr, "Could not allocate video codec context\n");
		return false;
	}

	if (ctx.pCodec->capabilities & AV_CODEC_CAP_TRUNCATED)
		ctx.pCodecContext->flags |= AV_CODEC_FLAG_TRUNCATED; // we do not send complete frames

	//根据CODEC_ID初始化AVCodecParserContext对象
	ctx.pCodecParserCtx = av_parser_init(AV_CODEC_ID_H264);
	if (!ctx.pCodecParserCtx)
	{
		printf("Could not allocate video parser context\n");
		return false;
	}

	//打开AVCodec对象
	if (avcodec_open2(ctx.pCodecContext, ctx.pCodec, NULL) < 0)
	{
		fprintf(stderr, "Could not open codec\n");
		return false;
	}

	//分配AVFrame对象
	ctx.frame = av_frame_alloc();
	if (!ctx.frame) 
	{
		fprintf(stderr, "Could not allocate video frame\n");
		return false;
	}

	return true;
}


int main(int argc, char **argv)
{
	uint8_t *pDataPtr = NULL;
	int uDataSize = 0;
	int got_picture, len;

	CodecCtx ctx;
	IOParam inputoutput;
	inputoutput.pNameIn = "/media/soccor.264";
	inputoutput.pNameOut= "./soccor.yuv";


	Open_files(inputoutput);				//打开输入输出文件
	
	uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
	
	memset(inbuf + INBUF_SIZE, 0, AV_INPUT_BUFFER_PADDING_SIZE);
	
	printf("Decode video file %s to %s\n", argv[1], argv[2]);

	Open_deocder(ctx);						//打开编解码器各个组件

	while(1)
	{
		//将码流文件按某长度读入输入缓存区
		uDataSize = fread(inbuf, 1, INBUF_SIZE, inputoutput.pFin);
		if (0 == uDataSize)
		{
			break;
		}

		pDataPtr = inbuf;

		while(uDataSize > 0)
		{
			//解析缓存区中的数据为AVPacket对象，包含一个NAL Unit的数据
			len = av_parser_parse2(ctx.pCodecParserCtx, ctx.pCodecContext, 
										&(ctx.pkt.data), &(ctx.pkt.size), 
										pDataPtr, uDataSize, 
										AV_NOPTS_VALUE, AV_NOPTS_VALUE, AV_NOPTS_VALUE);	
			pDataPtr += len;
			uDataSize -= len;

			if (0 == ctx.pkt.size)
			{
				continue;
			}

			printf("Parse 1 packet. Packet pts: %d.\n", ctx.pkt.pts);

			//根据AVCodecContext的设置，解析AVPacket中的码流，输出到AVFrame
			int ret = avcodec_decode_video2(ctx.pCodecContext, ctx.frame, &got_picture, &(ctx.pkt));
			if (ret < 0) 
			{
				printf("Decode Error.\n");
				return ret;
			}

			if (got_picture) 
			{
				//获得一帧完整的图像，写出到输出文件
				write_out_yuv_frame(ctx, inputoutput);
				printf("Succeed to decode 1 frame! Frame pts: %d\n", ctx.frame->pts);
			}
		} //while(uDataSize > 0)
	}

    ctx.pkt.data = NULL;
    ctx.pkt.size = 0;
	while(1)
	{
		//将编码器中剩余的数据继续输出完
		int ret = avcodec_decode_video2(ctx.pCodecContext, ctx.frame, &got_picture, &(ctx.pkt));
		if (ret < 0) 
		{
			printf("Decode Error.\n");
			return ret;
		}

		if (got_picture) 
		{
			write_out_yuv_frame(ctx, inputoutput);
			printf("Flush Decoder: Succeed to decode 1 frame!\n");
		}
		else
		{
			break;
		}
	} //while(1)

	//收尾工作
	Close_files(inputoutput);
	Close_decoder(ctx);

	return 1;
}

#endif





#if 0
// FFMPEG 实时解码网络H264码流，RTP封装

int CVPPMediaPlayer::init_decode()
{
	av_init_packet(&m_avpkt);
 
	m_codec = avcodec_find_decoder(CODEC_ID_H264);
	if(!m_codec){
		TRACE(_T("Codec not found\n"));
		return -1;
	}
	m_pCodecCtx = avcodec_alloc_context3(m_codec);
	if(!m_pCodecCtx){
		TRACE(_T("Could not allocate video codec context\n"));
		return -1;
	}
 
	m_pCodecParserCtx=av_parser_init(AV_CODEC_ID_H264);
	if (!m_pCodecParserCtx){
		TRACE(_T("Could not allocate video parser context\n"));
		return -1;
	}
	
	if(m_codec->capabilities&CODEC_CAP_TRUNCATED)
		m_pCodecCtx->flags|= CODEC_FLAG_TRUNCATED; 
 
	if (avcodec_open2(m_pCodecCtx, m_codec, NULL) < 0) {
		TRACE(_T("Could not open codec\n"));
		return -1;
	}
 
	m_picture = av_frame_alloc();
	m_pFrameRGB = av_frame_alloc();
	if(!m_picture || !m_pFrameRGB){
		TRACE(_T("Could not allocate video frame\n"));
		return -1;
	}
 
	m_PicBytes = 0;
	m_PicBuf = NULL;
	m_pImgCtx = NULL;
 
	return 0;
}
 
int CVPPMediaPlayer::video_recv_poll()
{
	char buf[RTP_RECV_BUF_LEN];
	char h264_buf[DECODE_BUF_SIZE];
	int sockfd, max_fd;
	int ret = 0;
	int len, out_len, addr_len;
	int line;
 
	struct sockaddr_in local_addr;
	struct sockaddr_in remote_addr;
 
	fd_set rset;
	struct timeval timeout = {RTP_RECV_TIMEOUT, 0};
 
	RTP_FIXED_HEADER* p_rtp_head;
	int rtp_head_len = sizeof(RTP_FIXED_HEADER);
 
	memset(&local_addr, 0, sizeof(local_addr));
	local_addr.sin_family = AF_INET;
	local_addr.sin_addr.s_addr = inet_addr(g_vpp_call.lines[m_VcId].meida_info.local_ip);
	local_addr.sin_port = htons(g_vpp_call.lines[m_VcId].meida_info.local_video_port);
 
	sockfd = socket(PF_INET, SOCK_DGRAM, 0);
	if(sockfd <= 0){
		TRACE(_T("Create socket for video recv failed\n"));
		return -1;
	}
 
	if(bind(sockfd, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0){
		TRACE(_T("bind failed\n"));
		closesocket(sockfd);
		return -1;
	}
 
	while(1)
	{
		timeout.tv_sec = RTP_RECV_TIMEOUT; 
		timeout.tv_usec = 0;
		FD_ZERO(&rset);
		FD_SET(sockfd, &rset);
		max_fd = sockfd + 1;
		ret = select(max_fd, &rset, NULL, NULL, &timeout);
		if(g_vpp_call.lines[m_VcId].status == CALL_STATUS_IDLE){
			TRACE(_T("Thread for recv video rtp end!\n"));
			PostMessage(WM_CLOSE_MEDIA_PLAYER);
			closesocket(sockfd);
			return 0;
		}
 
		if(ret < 0){
			TRACE(_T("select error"));
			PostMessage(WM_CLOSE_MEDIA_PLAYER);
			closesocket(sockfd);
			return -1;
		}else if(ret == 0){
			//TRACE(_T("select timeout %d s"), RTP_RECV_TIMEOUT);
			continue;
		}else{
			memset(buf, 0, sizeof(buf));
			memset(h264_buf, 0, sizeof(h264_buf));
			addr_len = sizeof(struct sockaddr);
			len = recvfrom(sockfd, buf, RTP_RECV_BUF_LEN, 0, (struct sockaddr *)&remote_addr, &addr_len);
			if(len <= 0){
				TRACE(_T("socket was shut, close it"));
				PostMessage(WM_CLOSE_MEDIA_PLAYER);
				closesocket(sockfd);
				return -1;
			}
 
			
			p_rtp_head = (RTP_FIXED_HEADER*)buf;
			TRACE(_T("recv video stream, len=%d, ssrc=0x%08x\n"), len, ntohl(p_rtp_head->ssrc));
			out_len = RTP_RECV_BUF_LEN;
			if(get_h264_data(buf, len, h264_buf, out_len)){
				h264_decode(h264_buf, out_len);
			}
		}
	}
 
	return 0;
}


BOOL CVPPMediaPlayer::get_h264_data(char* in_buf, int in_len, char* out_buf, int &out_len)
{
	int rtp_head_len;
	char nalu[4] = {0x00, 0x00, 0x00, 0x01};
	
	memcpy(out_buf, nalu, 4);
	rtp_head_len = sizeof(RTP_FIXED_HEADER);
	out_buf += 4;
	in_buf += rtp_head_len;
	memcpy(out_buf, in_buf, in_len-rtp_head_len);
	out_len = in_len-rtp_head_len+4;
 
	return TRUE;
}


int CVPPMediaPlayer::h264_decode(char* buf, int buf_len)
{
	int got, len, paser_len;
	
 
	if(buf == NULL || buf_len == 0){
		return -1;
	}
 
	paser_len = av_parser_parse2(m_pCodecParserCtx, m_pCodecCtx, &m_avpkt.data, &m_avpkt.size, (uint8_t *)buf, buf_len,
		AV_NOPTS_VALUE, AV_NOPTS_VALUE, AV_NOPTS_VALUE);
 
	if(paser_len == 0){
		switch(m_pCodecParserCtx->pict_type){
			case AV_PICTURE_TYPE_I: TRACE(_T("Type: I\n"));break;
			case AV_PICTURE_TYPE_P: TRACE(_T("Type: P\n"));break;
			case AV_PICTURE_TYPE_B: TRACE(_T("Type: B\n"));break;
			default: TRACE(_T("Type: Other\n"));break;
		}
		len = avcodec_decode_video2(m_pCodecCtx, m_picture, &got, &m_avpkt);
		if(len < 0){
			TRACE(_T("Error while decoding frame\n"));
			return -1;
		}
 
		if(got){
			TRACE(_T("Got picture\n"));
			if(m_PicBytes == 0){
				m_PicBytes = avpicture_get_size(PIX_FMT_BGR24, m_pCodecCtx->width, m_pCodecCtx->height);
				m_PicBuf = new uint8_t[m_PicBytes];
				avpicture_fill((AVPicture *)m_pFrameRGB, m_PicBuf, PIX_FMT_BGR24,
					m_pCodecCtx->width, m_pCodecCtx->height);
			}
 
			if(!m_pImgCtx){
				m_pImgCtx = sws_getContext(m_pCodecCtx->width, m_pCodecCtx->height, m_pCodecCtx->pix_fmt, m_pCodecCtx->width, m_pCodecCtx->height, PIX_FMT_BGR24, SWS_BICUBIC, NULL, NULL, NULL);
			}
 
			m_picture->data[0] += m_picture->linesize[0]*(m_pCodecCtx->height-1);
			m_picture->linesize[0] *= -1;                      
			m_picture->data[1] += m_picture->linesize[1]*(m_pCodecCtx->height/2-1);
			m_picture->linesize[1] *= -1;
			m_picture->data[2] += m_picture->linesize[2]*(m_pCodecCtx->height/2-1);
			m_picture->linesize[2] *= -1;
			sws_scale(m_pImgCtx, (const uint8_t* const*)m_picture->data, m_picture->linesize, 0, m_pCodecCtx->height, m_pFrameRGB->data, m_pFrameRGB->linesize); 
 
			display_picture(m_pFrameRGB->data[0], m_pCodecCtx->width, m_pCodecCtx->height);
		}
	}
 
	if(paser_len == 0){
		paser_len = av_parser_parse2(m_pCodecParserCtx, m_pCodecCtx, &m_avpkt.data, &m_avpkt.size, (uint8_t *)buf, buf_len,
			AV_NOPTS_VALUE, AV_NOPTS_VALUE, AV_NOPTS_VALUE);
	}
 
	return 0;
}

void CVideoStatic::display_pic(unsigned char* data, int width, int height)
{
	CRect  rc;
	HDC hdc = GetDC()->GetSafeHdc();
	GetClientRect(&rc);
 
	if(m_height != height || m_width != width){
		m_height = height;
		m_width = width;
 
		MoveWindow(0, 0, width, height, 0);
		Invalidate();
	}
 
	init_bm_head();
 
	DrawDibDraw(m_DrawDib,
		hdc,
		rc.left,
		rc.top,
		-1,	// don't stretch
		-1,
		&m_bm_info.bmiHeader, 
		(void*)data, 
		0, 
		0, 
		width, 
		height, 
		0);
}

void CVideoStatic::init_bm_head()
{
	m_bm_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	m_bm_info.bmiHeader.biWidth = m_width;
	m_bm_info.bmiHeader.biHeight = m_height;
	m_bm_info.bmiHeader.biPlanes =1;
	m_bm_info.bmiHeader.biBitCount = 24;
	m_bm_info.bmiHeader.biCompression = BI_RGB;
	m_bm_info.bmiHeader.biSizeImage = 0;
	m_bm_info.bmiHeader.biClrUsed = 0;
	m_bm_info.bmiHeader.biClrImportant = 0;
}
 
 


#endif


#endif










