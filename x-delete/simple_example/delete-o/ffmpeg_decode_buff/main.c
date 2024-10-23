#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>

#include <libavutil/imgutils.h>
 
#define DECODED_OUTPUT_FORMAT  AV_PIX_FMT_YUV420P

#define INPUT_FILE_NAME "encode_w352xh288.h264"
#define OUTPUT_FILE_NAME "decode_yuv420p_w352xh288.yuv"
#define IMAGE_WIDTH  352	// 320
#define IMAGE_HEIGHT 288	// 240


// ffplay -f rawvideo -video_size 352x288 yuv_420-352x288.yuv
// ffplay -f rawvideo -video_size 352x288 decode_yuv420p_w352xh288.yuv

// ffplay -f rawvideo -video_size 352x288 ffmpeg_decode_yuv420p_w352xh288.yuv 				// 播放yuv
// ffmpeg -c:v h264 -i encode_w352xh288.h264 ffmpeg_decode_yuv420p_w352xh288.yuv    // 解码h264为yuv


// main.c:42:9:   warning: ‘av_register_all’ is deprecated [-Wdeprecated-declarations]
// main.c:105:9:  warning: ‘avpicture_get_size’ is deprecated [-Wdeprecated-declarations]
// main.c:115:9:  warning: ‘av_init_packet’ is deprecated [-Wdeprecated-declarations]
// main.c:144:25: warning: ‘avcodec_decode_video2’ is deprecated [-Wdeprecated-declarations]
// main.c:151:33: warning: ‘avpicture_layout’ is deprecated [-Wdeprecated-declarations]
// main.c:157:25: warning: ‘av_free_packet’ is deprecated [-Wdeprecated-declarations]

int main(int argc, int *argv[])
{

	int fd, ret, video_stream;
	int i_fd, got_picture;
	AVCodec *codec;
	AVCodecContext *codec_context;
	AVCodecParserContext *codec_parser;
	AVFormatContext *format_context = NULL;
	AVFrame *frame;
	AVPacket packet;
	char *buffer;
	int size, zero_cont;
	int len = 0, rbyte;
	char buf[4096];
	char *data = buf;

	// avdevice_register_all();
	av_register_all();


	fd = open("decode_yuv420p_w352xh288.yuv", O_RDWR | O_CREAT, 0666);
	
	i_fd = open("encode_w352xh288.h264", O_RDWR, 0666);

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
	
	codec_parser = av_parser_init(AV_CODEC_ID_H264);
	
	if (!codec_parser) {
		printf("%s: <av_parser_init error>\n", __func__);
	}


	frame = av_frame_alloc();

	if (!frame) {
		printf("%s: <av_frame_alloc error>\n", __func__);
	}

	ret = avcodec_open2(codec_context, codec, NULL);

	if (ret < 0) {
		printf("%s: <avcodex_open2 error> ret = %d\n", __func__, ret);
	}

//	size = avpicture_get_size(AV_PIX_FMT_YUV420P, IMAGE_WIDTH, IMAGE_HEIGHT);
//	printf("%s: size = %d\n", __func__, size);
//	buffer = malloc(size);
	
	// size = avpicture_get_size(AV_PIX_FMT_YUV420P, IMAGE_WIDTH, IMAGE_HEIGHT);
	size = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, IMAGE_WIDTH, IMAGE_HEIGHT, 1);
	printf("%s: size = %d\n", __func__, size);

	// buffer = malloc(size);
	buffer = av_mallocz(size * sizeof(uint8_t));

	if (!buffer) {
		printf("%s: <malloc error>\n", __func__);
	}

	zero_cont = 0;
	av_init_packet(&packet);
	
	while (1) {
		rbyte = read(i_fd, buf, sizeof(buf));
		printf("%s: zero_cont = %d, len = %d, rbyte = %d\n", __func__, zero_cont, len, rbyte);
		
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
										&packet.data, &packet.size, 
										data, rbyte, 
										AV_NOPTS_VALUE, AV_NOPTS_VALUE, AV_NOPTS_VALUE);
										
			rbyte -= len;
			data  += len;
			printf("%s: zero_cont = %d, rbyte = %d, len = %d, packet.size = %d\n", __func__, zero_cont, rbyte, len, packet.size);
										
			if (packet.size == 0) {
				continue;
			}
			
			ret = avcodec_decode_video2(codec_context, frame, &got_picture, &packet);
			printf("%s: AV_PIX_FMT_YUV420P = %d, frame->width = %d, frame->height = %d, frame->format = %d\n", __func__, AV_PIX_FMT_YUV420P, frame->width, frame->height, frame->format);

			if (ret < 0) {
				printf("%s: <avcodex_open2 error> ret = %d\n", __func__, ret);
			}
			
			if (got_picture) {
			
				// correct
				avpicture_layout((AVPicture *)frame, AV_PIX_FMT_YUV420P, IMAGE_WIDTH, IMAGE_HEIGHT, buffer, size);

				ret = write(fd, buffer, size);
			
				printf("size = %d, frame->width * frame->height * 3 / 2 = %d\n", size, frame->width * frame->height * 3 / 2);
			
				
			/*	
				// error
				ret = write(fd, frame->data[0], frame->width * frame->height);
        		ret = write(fd, frame->data[1], frame->width * frame->height / 4);
        		ret = write(fd, frame->data[2], frame->width * frame->height / 4);
        	*/	
        		
        	/*	
        		// error
        		int imgSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, IMAGE_WIDTH, IMAGE_HEIGHT, 4);
        		ret = write(fd, frame->data[0], imgSize);
        	*/
			}

			// av_packet_free(&packet);
			av_free_packet(&packet);
		}
	}
	printf("%s: <00> rbyte = %d, len = %d, packet.size = %d\n", __func__, rbyte, len, packet.size);

	close(i_fd);
	close(fd);
	free(buffer);
	
	avcodec_close(codec_context);
	av_free(codec_context);
	av_frame_free(&frame);

	return 0;
}



#if 0
int main(int argc, int *argv[])
{

	int fd, ret, video_stream;
	int i_fd, got_picture;
	AVCodec *codec;
	AVCodecContext *codec_context;
	AVCodecParserContext *codec_parser;
	AVFormatContext *format_context = NULL;
	AVFrame *frame;
	AVPacket packet;
	char *buffer;
	int size, zero_cont;
	int len = 0, rbyte;
	char buf[4096];
	char *data = buf;

	// avdevice_register_all();
	av_register_all();


	fd = open("decode_yuv420p_w352xh288.yuv", O_RDWR | O_CREAT, 0666);
	
	i_fd = open("encode_w352xh288.h264", O_RDWR, 0666);
	
#if 0
	ret = avformat_open_input(&format_context, "encode_w352xh288.h264", NULL, NULL);

	if (ret < 0) {
		printf("%s: <avformat_open_input error> ret = %d\n", __func__, ret);
	}

	ret = avformat_find_stream_info(format_context, NULL);

	if (ret < 0) {
		printf("%s: <avformat_find_stream_info error> ret = %d\n", __func__, ret);
	}

	video_stream = 0;
	codec_context = format_context->streams[video_stream]->codec;

	if (!codec_context) {
		printf("%s: <codec_context error>\n", __func__);
	}
#endif

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
	
	codec_parser = av_parser_init(AV_CODEC_ID_H264);
	
	if (!codec_parser) {
		printf("%s: <av_parser_init error>\n", __func__);
	}


	frame = av_frame_alloc();

	if (!frame) {
		printf("%s: <av_frame_alloc error>\n", __func__);
	}

	ret = avcodec_open2(codec_context, codec, NULL);

	if (ret < 0) {
		printf("%s: <avcodex_open2 error> ret = %d\n", __func__, ret);
	}

	size = avpicture_get_size(AV_PIX_FMT_YUV420P, IMAGE_WIDTH, IMAGE_HEIGHT);
	printf("%s: size = %d\n", __func__, size);

	buffer = malloc(size);

	if (!buffer) {
		printf("%s: <malloc error>\n", __func__);
	}

	zero_cont = 0;
	av_init_packet(&packet);
	
	while (1) {
		rbyte = read(i_fd, buf, sizeof(buf));
		printf("%s: zero_cont = %d, len = %d, rbyte = %d\n", __func__, zero_cont, len, rbyte);
		
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
										&packet.data, &packet.size, 
										data, rbyte, 
										AV_NOPTS_VALUE, AV_NOPTS_VALUE, AV_NOPTS_VALUE);
										
			rbyte -= len;
			data  += len;
			printf("%s: zero_cont = %d, rbyte = %d, len = %d, packet.size = %d\n", __func__, zero_cont, rbyte, len, packet.size);
										
			if (packet.size == 0) {
				continue;
			}
			
			ret = avcodec_decode_video2(codec_context, frame, &got_picture, &packet);

			if (ret < 0) {
				printf("%s: <avcodex_open2 error> ret = %d\n", __func__, ret);
			}
			
			if (got_picture) {
				avpicture_layout((AVPicture *)frame, AV_PIX_FMT_YUV420P, IMAGE_WIDTH, IMAGE_HEIGHT, buffer, size);

				ret = write(fd, buffer, size);
			}

			// av_packet_free(&packet);
			av_free_packet(&packet);
		}
	}
	printf("%s: <00> rbyte = %d, len = %d, packet.size = %d\n", __func__, rbyte, len, packet.size);
	
/*
	do {
		len = av_parser_parse2(codec_parser, codec_context, 
										&packet.data, &packet.size, 
										data, rbyte, 
										AV_NOPTS_VALUE, AV_NOPTS_VALUE, AV_NOPTS_VALUE);
										
		rbyte -= len;
		data  += len;
		printf("%s: <11> rbyte = %d, len = %d, packet.size = %d\n", __func__, rbyte, len, packet.size);
										
		if (packet.size == 0) {
			continue;
		}
			
		ret = avcodec_decode_video2(codec_context, frame, &got_picture, &packet);

		if (ret < 0) {
			printf("%s: <avcodex_open2 error> ret = %d\n", __func__, ret);
		}
			
		if (got_picture) {
			avpicture_layout((AVPicture *)frame, AV_PIX_FMT_YUV420P, IMAGE_WIDTH, IMAGE_HEIGHT, buffer, size);

			ret = write(fd, buffer, size);
		}

		// av_packet_free(&packet);
		av_free_packet(&packet);
	} while (0);
*/

/*
	while (1) {
		ret = avcodec_decode_video2(codec_context, frame, &got_picture, &packet);
		
		printf("%s: ret = %d, got_picture = %d\n", __func__, ret, got_picture);

		if (ret < 0) {
			printf("%s: <avcodex_open2 error> ret = %d\n", __func__, ret);
		}
			
		if (got_picture) {
			avpicture_layout((AVPicture *)frame, AV_PIX_FMT_YUV420P, IMAGE_WIDTH, IMAGE_HEIGHT, buffer, size);

			ret = write(fd, buffer, size);
		}
	}
*/

/*
	while (av_read_frame(format_context, &packet) >= 0) {
		printf("%s: packet.stream_index = %d, packet.size = %d\n", __func__, packet.stream_index, packet.size);

		ret = avcodec_decode_video2(codec_context, frame, &got_picture, &packet);

		if (ret < 0) {
			printf("%s: <avcodex_open2 error> ret = %d\n", __func__, ret);
		}

		if (got_picture) {
			avpicture_layout((AVPicture *)frame, AV_PIX_FMT_YUV420P, IMAGE_WIDTH, IMAGE_HEIGHT, buffer, size);

			ret = write(fd, buffer, size);
		}

		av_free_packet(&packet);
	}
*/

	close(i_fd);
	close(fd);
	free(buffer);
	
	/*
	avformat_close_input(&format_context);
	free(buffer);
	av_free(frame);
	avcodec_close(codec_context);
	*/
	
	avcodec_close(codec_context);
	av_free(codec_context);
	av_frame_free(&frame);
	
	// avcodec_close(ctx.pCodecContext);
	// av_free(ctx.pCodecContext);
	// av_frame_free(&(ctx.frame));


	return 0;
}
#endif



#if 0
int main(int argc, int *argv[])
{

	int fd, ret, video_stream;
	int got_picture;
	AVCodec *codec;
	AVCodecContext *codec_context;
	AVFormatContext *format_context = NULL;
	AVFrame *frame;
	AVPacket packet;
	char *buffer;
	int size;

	// avdevice_register_all();
	av_register_all();

	fd = open("decode_yuv420p_w352xh288.yuv", O_RDWR | O_CREAT, 0666);

	ret = avformat_open_input(&format_context, "encode_w352xh288.h264", NULL, NULL);

	if (ret < 0) {
		printf("%s: <avformat_open_input error> ret = %d\n", __func__, ret);
	}

	ret = avformat_find_stream_info(format_context, NULL);

	if (ret < 0) {
		printf("%s: <avformat_find_stream_info error> ret = %d\n", __func__, ret);
	}

	video_stream = 0;
	codec_context = format_context->streams[video_stream]->codec;

	if (!codec_context) {
		printf("%s: <codec_context error>\n", __func__);
	}

	codec = avcodec_find_decoder(AV_CODEC_ID_H264);

	if (!codec) {
		printf("%s: <avcodec_find_decoder error>\n", __func__);
	}

	frame = av_frame_alloc();

	if (!frame) {
		printf("%s: <av_frame_alloc error>\n", __func__);
	}

	ret = avcodec_open2(codec_context, codec, NULL);

	if (ret < 0) {
		printf("%s: <avcodex_open2 error> ret = %d\n", __func__, ret);
	}

	size = avpicture_get_size(AV_PIX_FMT_YUV420P, IMAGE_WIDTH, IMAGE_HEIGHT);
	printf("%s: size = %d\n", __func__, size);

	buffer = malloc(size);

	if (!buffer) {
		printf("%s: <malloc error>\n", __func__);
	}

	av_init_packet(&packet);

	while (av_read_frame(format_context, &packet) >= 0) {
		printf("%s: packet.stream_index = %d, packet.size = %d\n", __func__, packet.stream_index, packet.size);

		ret = avcodec_decode_video2(codec_context, frame, &got_picture, &packet);

		if (ret < 0) {
			printf("%s: <avcodex_open2 error> ret = %d\n", __func__, ret);
		}

		if (got_picture) {
			avpicture_layout((AVPicture *)frame, AV_PIX_FMT_YUV420P, IMAGE_WIDTH, IMAGE_HEIGHT, buffer, size);

			ret = write(fd, buffer, size);
		}

		av_free_packet(&packet);
	}

	close(fd);
	avformat_close_input(&format_context);
	free(buffer);
	av_free(frame);
	avcodec_close(codec_context);


	return 0;
}

#endif











