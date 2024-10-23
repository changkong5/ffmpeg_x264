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
	AVPacket *packet;
	char *buffer;
	int size, zero_cont;
	int len = 0, rbyte;
	char buf[4096];
	char *data = buf;

	fd = open("decode_yuv420p_w352xh288.yuv", O_RDWR | O_CREAT, 0666);
	
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
	
	// codec_parser = av_parser_init(codec->id);
	codec_parser = av_parser_init(AV_CODEC_ID_H264);
	
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

#if 0
		    	if (frame->data[0] && frame->data[1] && frame->data[2]) {
		    		ret = write(fd, frame->data[0], frame->width * frame->height);
		    		ret = write(fd, frame->data[1], frame->width * frame->height / 4);
		    		ret = write(fd, frame->data[2], frame->width * frame->height / 4);
        		} else {
        			printf("=====================\n");
        			return 0;
        		}
#else
        		av_image_copy_to_buffer(buffer, size, (const uint8_t * const *)frame->data, frame->linesize, frame->format, frame->width, frame->height, 1);
        		
        		// avpicture_layout((AVPicture *)frame, AV_PIX_FMT_YUV420P, IMAGE_WIDTH, IMAGE_HEIGHT, buffer, size);
				ret = write(fd, buffer, size);
#endif        		
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
	AVPacket *packet;
	char *buffer;
	int size, zero_cont;
	int len = 0, rbyte;
	char buf[4096];
	char *data = buf;

	// avdevice_register_all();
	// av_register_all();


	fd = open("decode_yuv420p_w352xh288.yuv", O_RDWR | O_CREAT, 0666);
	
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
		// codec_context->flags |= AV_CODEC_FLAG_TRUNCATED; // we do not send complete frames
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

/*
	size = avpicture_get_size(AV_PIX_FMT_YUV420P, IMAGE_WIDTH, IMAGE_HEIGHT);
	printf("%s: size = %d\n", __func__, size);

	buffer = malloc(size);

	if (!buffer) {
		printf("%s: <malloc error>\n", __func__);
	}
*/

	zero_cont = 0;
//	av_init_packet(&packet);
	
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
										&packet->data, &packet->size, 
										data, rbyte, 
										AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
										
			rbyte -= len;
			data  += len;
			printf("%s: zero_cont = %d, rbyte = %d, len = %d, packet->size = %d\n", __func__, zero_cont, rbyte, len, packet->size);
										
			if (packet->size == 0) {
				continue;
			}
			
			ret = avcodec_send_packet(codec_context, packet);
			
			if (ret < 0) {
				printf("%s: <avcodec_send_packet error> ret = %d\n", __func__, ret);
			}
			
			while (1) {
				ret = avcodec_receive_frame(codec_context, frame);
				printf("ret = %d\n", ret);
				
				if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
					break;
            		// return 0;
        		} else if (ret < 0) {
        			break;
            		// ERRBUF(ret);
            		// qDebug() << "avcodec_receive_frame error:" << errbuf;
            		// return ret;
        		}
        		
        		ret = write(fd, frame->data[0], frame->width * frame->height);
        		ret = write(fd, frame->data[1], frame->width * frame->height / 4);
        		ret = write(fd, frame->data[2], frame->width * frame->height / 4);
        		
        		
#if 0        		
        		// 将解码后的数据写入文件
        		int imgSize = av_image_get_buffer_size(ctx->pix_fmt, ctx->width, ctx->height, 1);
        		outFile.write((char *) frame->data[0], imgSize);
        		
        		// 将解码后的数据写入文件
				// 写入Y平面数据
				outFile.write((const char *)frame->data[0], frame->linesize[0] * frame->height);
				// 写入U平面数据
				outFile.write((const char *)frame->data[1], frame->linesize[1] * frame->height >> 1);
				// 写入V平面数据
				outFile.write((const char *)frame->data[2], frame->linesize[2] * frame->height >> 1);
#endif
			}
/*			
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
*/
		}
	}
	printf("%s: <00> rbyte = %d, len = %d, packet->size = %d\n", __func__, rbyte, len, packet->size);

	close(i_fd);
	close(fd);
	// free(buffer);
	
	//avcodec_close(codec_context);
	//av_free(codec_context);
	//av_frame_free(&frame);
	
	av_packet_free(&packet);
	av_frame_free(&frame);
	av_parser_close(codec_parser);
	avcodec_free_context(&codec_context);

	return 0;
}
#endif








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

	close(i_fd);
	close(fd);
	free(buffer);
	
	avcodec_close(codec_context);
	av_free(codec_context);
	av_frame_free(&frame);

	return 0;
}
#endif












