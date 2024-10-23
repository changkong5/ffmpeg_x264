#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>

#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
 
#define IMAGE_WIDTH  352	// 320
#define IMAGE_HEIGHT 288	// 240

// ffplay -f rawvideo -s 352x288 yuv420p_w352xh288.yuv 																// correct   // play yuv
// ffplay yuv420p_w352xh288.h264 																					// correct   // play h264
// ffmpeg -s 352x288 -i yuv420p_w352xh288.yuv -c:v h264 yuv420p_w352xh288.h264   									// correct	 // yuv ---> h264
// ffmpeg -c:v h264 -i yuv420p_w352xh288.h264 yuv420p_w352xh288.yuv    												// correct   // h264 ---> yuv
 
int main(int argc, int *argv[])
{
	int fd, ret;
	int i_fd, got_picture;
	AVCodec *codec;
	AVCodecContext *codec_context;
	AVCodecParserContext *codec_parser;
	// AVFormatContext *format_context = NULL;
	AVFrame *frame;
	AVPacket *packet;
	char *buffer = NULL;
	int size, zero_cont;
	int len = 0, rbyte;
	uint8_t buf[4096];
	char *data = buf;

	fd = open("yuv420p_w352xh288.yuv", O_RDWR | O_CREAT, 0666);
	
	i_fd = open("yuv420p_w352xh288.h264", O_RDWR, 0666);

	// codec = avcodec_find_decoder_by_name("h264");
	codec = avcodec_find_decoder(AV_CODEC_ID_H264);

	if (!codec) {
		printf("%s: <avcodec_find_decoder error>\n", __func__);
	}
	
	codec_context = avcodec_alloc_context3(codec);
	
	if (!codec_context) {
		printf("%s: <avcodec_alloc_context3 error>\n", __func__);
	}
	
	if (codec->capabilities & AV_CODEC_CAP_TRUNCATED) {
		codec_context->flags |= AV_CODEC_FLAG_TRUNCATED; // we do not send complete frames
	}
	
	codec_parser = av_parser_init(codec->id);
	
	if (!codec_parser) {
		printf("%s: <av_parser_init error>\n", __func__);
	}

	packet = av_packet_alloc();

	if (!packet) {
		printf("%s: <av_packet_alloc error>\n", __func__);
	}
	

	frame = av_frame_alloc();

	if (!frame) {
		printf("%s: <av_frame_alloc error>\n", __func__);
	}

	ret = avcodec_open2(codec_context, codec, NULL);

	if (ret < 0) {
		printf("%s: <avcodex_open2 error> ret = %d\n", __func__, ret);
	}
	
	zero_cont = 0;
	
	//size = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, IMAGE_WIDTH, IMAGE_HEIGHT, 1);
	//printf("%s: size = %d\n", __func__, size);
	//buffer = av_mallocz(size * sizeof(uint8_t));
	
	while (1) {
		rbyte = read(i_fd, buf, sizeof(buf));
		// printf("%s: zero_cont = %d, len = %d, rbyte = %d\n", __func__, zero_cont, len, rbyte);
		
		if (rbyte == 0) {
			zero_cont++;
			
			if (len == 0) {
				break;
			} else {
				if (zero_cont == 1) {
					rbyte = len;
				} else {
					break;
				}
			}
		}
		
		data = buf;
		while (rbyte > 0) {
			len = av_parser_parse2(codec_parser, codec_context, 
										&packet->data, &packet->size, 
										data, rbyte, 
										AV_NOPTS_VALUE, AV_NOPTS_VALUE, AV_NOPTS_VALUE);
										
			rbyte -= len;
			data  += len;
			printf("%s: zero_cont = %d, rbyte = %d, len = %d, packet->size = %d\n", __func__, zero_cont, rbyte, len, packet->size);
										
			if (packet->size == 0) {
				continue;
			}
			for (int i = 0; i < 9; i++) printf("0x%02x ", packet->data[i]); printf("\n");
			
			ret = avcodec_send_packet(codec_context, packet);
			
			if (ret < 0) {
				char errbuf[1024]; 
        		av_strerror(ret, errbuf, sizeof (errbuf));
				printf("%s: <avcodec_send_packet error> ret = %d\n", __func__, ret);
				return 0;
			}
			
			while (1) {
				ret = avcodec_receive_frame(codec_context, frame);
				// printf("ret = %d\n", ret);
				
				if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
					break;
        		} else if (ret < 0) {
        			char errbuf[1024]; 
            		av_strerror(ret, errbuf, sizeof (errbuf));
        			return 0;
        		}
        		printf("%s: AV_PIX_FMT_YUV420P = %d, frame->width = %d, frame->height = %d, frame->format = %d\n", __func__, AV_PIX_FMT_YUV420P, frame->width, frame->height, frame->format);

				if (1 && !buffer) {
					size = av_image_get_buffer_size(frame->format, frame->width, frame->height, 1);
					printf("%s: >>>> size = %d\n", __func__, size);
					buffer = av_mallocz(size * sizeof(uint8_t));
				}

        		av_image_copy_to_buffer(buffer, size, (const uint8_t * const *)frame->data, frame->linesize, frame->format, frame->width, frame->height, 1);
        		
        		// avpicture_layout((AVPicture *)frame, AV_PIX_FMT_YUV420P, IMAGE_WIDTH, IMAGE_HEIGHT, buffer, size);
				ret = write(fd, buffer, size);
			}
		}
	}
	printf("%s: <00> rbyte = %d, len = %d, packet->size = %d\n", __func__, rbyte, len, packet->size);

	close(i_fd);
	close(fd);
	av_freep(&buffer);

	av_packet_free(&packet);
	av_frame_free(&frame);
	av_parser_close(codec_parser);
	avcodec_free_context(&codec_context);
	
	
	return 0;
}
 
 
 
 
 
 
 
 
 
 
 
 
#if 0
#define DECODED_OUTPUT_FORMAT  AV_PIX_FMT_YUV420P

#define INPUT_FILE_NAME "encode_w352xh288.h264"
#define OUTPUT_FILE_NAME "decode_yuv420p_w352xh288.yuv"
#define IMAGE_WIDTH  352	// 320
#define IMAGE_HEIGHT 288	// 240


// ffplay -f rawvideo -video_size 352x288 yuv_420-352x288.yuv
// ffplay -f rawvideo -video_size 352x288 decode_yuv420p_w352xh288.yuv

// ffplay -f rawvideo -video_size 352x288 ffmpeg_decode_yuv420p_w352xh288.yuv 			// 播放yuv
// ffmpeg -c:v h264 -i encode_w352xh288.h264 ffmpeg_decode_yuv420p_w352xh288.yuv    	// 解码h264为yuv
// ffplay yuv420p_w352xh288.h264    // 播放h264编码文件
// ffmpeg -c:v h264 -f yuv420p -s 352x288 -i decode_yuv420p_w352xh288.yuv uv420p_w352xh288.h264  // 编码yuv为h264

// // ffmpeg -s 1920x1080 -pix_fmt yuv420p -i input.yuv -c:v libx264 -crf 20 -pix_fmt yuv420p output.mp4

// ffplay -f yuv420p -s 352x288 yuv420p_w352xh288.yuv    						// errror
// ffmpeg -f yuv420p -s 352x288 -i yuv420p_w352xh288.yuv yuv420p_w352xh288.h264 // errror

// ffplay -f rawvideo -video_size 352x288 yuv420p_w352xh288.yuv 				// correct   // play yuv
// ffplay -f rawvideo -s 352x288 yuv420p_w352xh288.yuv 							// correct 
// ffplay -f rawvideo -s 352x288 -pixel_format yuv420p yuv420p_w352xh288.yuv	// correct
// ffplay -s 352x288 -pixel_format yuv420p yuv420p_w352xh288.yuv     			// correct	
// ffmpeg -s 352x288 -pix_fmt yuv420p -i yuv420p_w352xh288.yuv -c:v h264 -pix_fmt yuv420p yuv420p_w352xh288.h264  	// correct  
// ffmpeg -s 352x288 -i yuv420p_w352xh288.yuv -vcodec libx264 yuv420p_w352xh288.h264   								// correct	 // yuv ---> h264

// decocder api
// avcodec_send_packet 
// avcodec_receive_frame

// encoder api
// avcodec_send_frame
// avcodec_receive_packet

// ffplay -f rawvideo -s 352x288 yuv420p_w352xh288.yuv 																// correct   // play yuv
// ffplay yuv420p_w352xh288.h264 																					// correct   // play h264
// ffmpeg -s 352x288 -i yuv420p_w352xh288.yuv -c:v h264 yuv420p_w352xh288.h264   									// correct	 // yuv ---> h264
// ffmpeg -c:v h264 -i yuv420p_w352xh288.h264 yuv420p_w352xh288.yuv    												// correct   // h264 ---> yuv


// ffplay -f rawvideo -s 352x288 decode_yuv420p_w352xh288.yuv 

// ffmpeg -i input.yuv422p -pix_fmt yuv420p output.yuv
// ffmpeg -i input.mp4 -vf "format=yuv420p" output.mp4

/*
// YUV422 ---> YUV420P

// AV_PIX_FMT_YUV420P
// AV_PIX_FMT_YUYV422
// AV_PIX_FMT_ARGB

#include <libswscale/swscale.h>

AVFrame *frame_422  = avcodec_alloc_frame();

AVFrame *frame_420p = avcodec_alloc_frame();
frame_420p->width = width;
frame_420p->height = height;
frame_420p->pix_fmt = AV_PIX_FMT_YUV420P;

int len = av_image_alloc(frame_420p->data, frame_420p->linesize, frame_420p->width, frame_420p->height, AV_PIX_FMT_YUV420P, 1);
av_freep(frame_420p->data);

SwsContext* swsContext = sws_getContext(frame_422->width, frame_422->height, frame_422->pix_fmt, 
										frame_420p->width, frame_420p->height, AV_PIX_FMT_YUV420P, 
										SWS_BILINEAR, NULL, NULL, NULL);
sws_scale(swsContext, frame_422->data, frame_422->linesize, 0, frame_420p->height, frame_420p->data, frame_420p->linesize);
*/

int main(int argc, int *argv[])
{

	int fd, ret, video_stream;
	int i_fd, got_picture;
	AVCodec *codec;
	AVCodecContext *codec_context;
	AVCodecParserContext *codec_parser;
	AVFormatContext *format_context = NULL;
	AVFrame *frame;
	AVPacket *packet;
	char *buffer;
	int size, zero_cont;
	int len = 0, rbyte;
	char buf[4096];
	char *data = buf;

	fd = open("yuv420p_w352xh288.yuv", O_RDWR | O_CREAT, 0666);
	
	i_fd = open("yuv420p_w352xh288.h264", O_RDWR, 0666);

	// codec = avcodec_find_decoder_by_name("h264");
	codec = avcodec_find_decoder(AV_CODEC_ID_H264);

	if (!codec) {
		printf("%s: <avcodec_find_decoder error>\n", __func__);
	}
	
	codec_context = avcodec_alloc_context3(codec);
	
	if (!codec_context) {
		printf("%s: <avcodec_alloc_context3 error>\n", __func__);
	}
	
	printf("codec_context->width = %d\n", codec_context->width);
	printf("codec_context->height = %d\n", codec_context->height);
	printf("codec_context->pix_fmt = %d\n", codec_context->pix_fmt);

/*	
	codec_context->time_base.num = 1;
	codec_context->time_base.den = 25;
	codec_context->width = IMAGE_WIDTH;
	codec_context->height = IMAGE_HEIGHT;
	codec_context->pix_fmt = AV_PIX_FMT_YUV420P;
*/	
	
	if (codec->capabilities & AV_CODEC_CAP_TRUNCATED) {
		codec_context->flags |= AV_CODEC_FLAG_TRUNCATED; // we do not send complete frames
	}
	
	codec_parser = av_parser_init(codec->id);
	// codec_parser = av_parser_init(AV_CODEC_ID_H264);
	
	if (!codec_parser) {
		printf("%s: <av_parser_init error>\n", __func__);
	}

	packet = av_packet_alloc();

	if (!packet) {
		printf("%s: <av_packet_alloc error>\n", __func__);
	}
	

	frame = av_frame_alloc();

	if (!frame) {
		printf("%s: <av_frame_alloc error>\n", __func__);
	}

	ret = avcodec_open2(codec_context, codec, NULL);

	if (ret < 0) {
		printf("%s: <avcodex_open2 error> ret = %d\n", __func__, ret);
	}

	zero_cont = 0;
	
	size = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, IMAGE_WIDTH, IMAGE_HEIGHT, 1);
	printf("%s: size = %d\n", __func__, size);

	buffer = av_mallocz(size * sizeof(uint8_t));
	
	while (1) {
		rbyte = read(i_fd, buf, sizeof(buf));
		// printf("%s: zero_cont = %d, len = %d, rbyte = %d\n", __func__, zero_cont, len, rbyte);
		
		if (rbyte == 0) {
			zero_cont++;
			
			if (len == 0) {
				break;
			} else {
				if (zero_cont == 1) {
					rbyte = len;
				} else {
					break;
				}
			}
		}
		
		data = buf;
		while (rbyte > 0) {
			len = av_parser_parse2(codec_parser, codec_context, 
										&packet->data, &packet->size, 
										data, rbyte, 
										AV_NOPTS_VALUE, AV_NOPTS_VALUE, AV_NOPTS_VALUE);
										
			rbyte -= len;
			data  += len;
			printf("%s: zero_cont = %d, rbyte = %d, len = %d, packet->size = %d\n", __func__, zero_cont, rbyte, len, packet->size);
										
			if (packet->size == 0) {
				continue;
			}
			
			ret = avcodec_send_packet(codec_context, packet);
			
			if (ret < 0) {
				char errbuf[1024]; 
        		av_strerror(ret, errbuf, sizeof (errbuf));
				printf("%s: <avcodec_send_packet error> ret = %d\n", __func__, ret);
				return 0;
			}
			
			while (1) {
				ret = avcodec_receive_frame(codec_context, frame);
				// printf("ret = %d\n", ret);
				
				if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
					break;
        		} else if (ret < 0) {
        			char errbuf[1024]; 
            		av_strerror(ret, errbuf, sizeof (errbuf));
        			return 0;
        		}
        		printf("%s: AV_PIX_FMT_YUV420P = %d, frame->width = %d, frame->height = %d, frame->format = %d\n", __func__, AV_PIX_FMT_YUV420P, frame->width, frame->height, frame->format);


        		av_image_copy_to_buffer(buffer, size, (const uint8_t * const *)frame->data, frame->linesize, frame->format, frame->width, frame->height, 1);
        		
        		// avpicture_layout((AVPicture *)frame, AV_PIX_FMT_YUV420P, IMAGE_WIDTH, IMAGE_HEIGHT, buffer, size);
				ret = write(fd, buffer, size);
	
        		// av_frame_unref(frame);
			}
		}
	}
	printf("%s: <00> rbyte = %d, len = %d, packet->size = %d\n", __func__, rbyte, len, packet->size);

	close(i_fd);
	close(fd);
	
	av_freep(&buffer);
	// free(buffer);

	av_packet_free(&packet);
	av_frame_free(&frame);
	av_parser_close(codec_parser);
	avcodec_free_context(&codec_context);

	return 0;
}

#endif











