#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>
 
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
#include <libavutil/imgutils.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#include <libavutil/frame.h>
#include <libavutil/mem.h>
 
#include <libavcodec/avcodec.h>
 
#define VIDEO_INBUF_SIZE 20480
#define VIDEO_REFILL_THRESH 4096



#define DECODED_OUTPUT_FORMAT  AV_PIX_FMT_YUV420P

#define INPUT_FILE_NAME "encode_w352xh288.h264"
#define OUTPUT_FILE_NAME "decode_yuv420p_w352xh288.yuv"
#define IMAGE_WIDTH  352	// 320
#define IMAGE_HEIGHT 288	// 240


 
static char err_buf[128] = {0};
static char* av_get_err(int errnum)
{
    av_strerror(errnum, err_buf, 128);
    return err_buf;
}
 
static void print_video_format(const AVFrame *frame)
{
    printf("width: %u\n", frame->width);
    printf("height: %u\n", frame->height);
    printf("format: %u\n", frame->format);// 格式需要注意
}
 
static void decode(AVCodecContext *dec_ctx, AVPacket *pkt, AVFrame *frame,
                   FILE *outfile)
{
    int ret;
    /* send the packet with the compressed data to the decoder */
    ret = avcodec_send_packet(dec_ctx, pkt);
    if(ret == AVERROR(EAGAIN))
    {
        fprintf(stderr, "Receive_frame and send_packet both returned EAGAIN, which is an API violation.\n");
    }
    else if (ret < 0)
    {
        fprintf(stderr, "Error submitting the packet to the decoder, err:%s, pkt_size:%d\n",
                av_get_err(ret), pkt->size);
        return;
    }
    
    int size = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, IMAGE_WIDTH, IMAGE_HEIGHT, 1);
	printf("%s: size = %d\n", __func__, size);

	char *buffer = av_mallocz(size * sizeof(uint8_t));
 
    /* read all the output frames (infile general there may be any number of them */
    while (ret >= 0)
    {
        // 对于frame, avcodec_receive_frame内部每次都先调用
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0)
        {
            fprintf(stderr, "Error during decoding\n");
            exit(1);
        }
        static int s_print_format = 0;
        if(s_print_format == 0)
        {
            s_print_format = 1;
            print_video_format(frame);
        }
 
 		av_image_copy_to_buffer(buffer, size, (const uint8_t * const *)frame->data, frame->linesize, frame->format, frame->width, frame->height, 1);
 		
 		fwrite(buffer, 1, size, outfile);
 		
 #if 0
        // 一般H264默认为 AV_PIX_FMT_YUV420P, 具体怎么强制转为 AV_PIX_FMT_YUV420P 在音视频合成输出的时候讲解
        //  写入y分量
        fwrite(frame->data[0], 1, frame->width * frame->height,  outfile);//Y
        // 写入u分量
        fwrite(frame->data[1], 1, (frame->width) *(frame->height)/4,outfile);//U:宽高均是Y的一半
        //  写入v分量
        fwrite(frame->data[2], 1, (frame->width) *(frame->height)/4,outfile);//V：宽高均是Y的一半
#endif
    }
    
    av_freep(&buffer);
}

//#define INPUT_FILE_NAME "encode_w352xh288.h264"
//#define OUTPUT_FILE_NAME "decode_yuv420p_w352xh288.yuv"
// 提取H264: ffmpeg -i source.200kbps.768x320_10s.flv -vcodec libx264 -an -f h264 source.200kbps.768x320_10s.h264
// 提取MPEG2: ffmpeg -i source.200kbps.768x320_10s.flv -vcodec mpeg2video -an -f mpeg2video source.200kbps.768x320_10s.mpeg2
// 播放：ffplay -pixel_format yuv420p -video_size 768x320 -framerate 25  source.200kbps.768x320_10s.yuv
int main(int argc, char **argv)
{
    const char *outfilename;
    const char *filename;
    const AVCodec *codec;
    AVCodecContext *codec_ctx= NULL;
    AVCodecParserContext *parser = NULL;
    int len = 0;
    int ret = 0;
    FILE *infile = NULL;
    FILE *outfile = NULL;
    // AV_INPUT_BUFFER_PADDING_SIZE 在输入比特流结尾的要求附加分配字节的数量上进行解码
    uint8_t inbuf[VIDEO_INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    uint8_t *data = NULL;
    size_t   data_size = 0;
    AVPacket *pkt = NULL;
    AVFrame *decoded_frame = NULL;
 
 /*
    if (argc <= 2)
    {
        fprintf(stderr, "Usage: %s <input file> <output file>\n", argv[0]);
        exit(0);
    }
    filename    = argv[1];
    outfilename = argv[2];
*/
    
    filename    = "encode_w352xh288.h264";
    outfilename = "decode_yuv420p_w352xh288.yuv";
 
    pkt = av_packet_alloc();
    enum AVCodecID video_codec_id = AV_CODEC_ID_H264;
    if(strstr(filename, "264") != NULL)
    {
        video_codec_id = AV_CODEC_ID_H264;
    }
    else if(strstr(filename, "mpeg2") != NULL)
    {
        video_codec_id = AV_CODEC_ID_MPEG2VIDEO;
    }
    else
    {
        printf("default codec id:%d\n", video_codec_id);
    }
 
    // 查找解码器
    codec = avcodec_find_decoder(video_codec_id);  // AV_CODEC_ID_AAC
    if (!codec) {
        fprintf(stderr, "Codec not found\n");
        exit(1);
    }
    // 获取裸流的解析器 AVCodecParserContext(数据)  +  AVCodecParser(方法)
    parser = av_parser_init(codec->id);
    if (!parser) {
        fprintf(stderr, "Parser not found\n");
        exit(1);
    }
    // 分配codec上下文
    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        fprintf(stderr, "Could not allocate audio codec context\n");
        exit(1);
    }
    codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
 
    // 将解码器和解码器上下文进行关联
    if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        exit(1);
    }
 
    // 打开输入文件
    infile = fopen(filename, "rb");
    if (!infile) {
        fprintf(stderr, "Could not open %s\n", filename);
        exit(1);
    }
    // 打开输出文件
    outfile = fopen(outfilename, "wb");
    if (!outfile) {
        av_free(codec_ctx);
        exit(1);
    }
 
    // 读取文件进行解码
    data      = inbuf;
    data_size = fread(inbuf, 1, VIDEO_INBUF_SIZE, infile);
 
    while (data_size > 0)
    {
        if (!decoded_frame)
        {
            if (!(decoded_frame = av_frame_alloc()))
            {
                fprintf(stderr, "Could not allocate audio frame\n");
                exit(1);
            }
        }
 
        ret = av_parser_parse2(parser, codec_ctx, &pkt->data, &pkt->size,
                               data, data_size,
                               AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
        if (ret < 0)
        {
            fprintf(stderr, "Error while parsing\n");
            exit(1);
        }
        data      += ret;   // 跳过已经解析的数据
        data_size -= ret;   // 对应的缓存大小也做相应减小
 
        if (pkt->size)
            decode(codec_ctx, pkt, decoded_frame, outfile);
 
        if (data_size < VIDEO_REFILL_THRESH)    // 如果数据少了则再次读取
        {
            memmove(inbuf, data, data_size);    // 把之前剩的数据拷贝到buffer的起始位置
            data = inbuf;
            // 读取数据 长度: VIDEO_INBUF_SIZE - data_size
            len = fread(data + data_size, 1, VIDEO_INBUF_SIZE - data_size, infile);
            if (len > 0)
                data_size += len;
        }
    }
 
    /* 冲刷解码器 */
    pkt->data = NULL;   // 让其进入drain mode
    pkt->size = 0;
    decode(codec_ctx, pkt, decoded_frame, outfile);
 
    fclose(outfile);
    fclose(infile);
 
    avcodec_free_context(&codec_ctx);
    av_parser_close(parser);
    av_frame_free(&decoded_frame);
    av_packet_free(&pkt);
 
    printf("main finish, please enter Enter and exit\n");
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
	
	i_fd = open("encode_w352xh288.h264", O_RDWR, 0666);

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
        		
        		ret = write(fd, frame->data[0], frame->linesize[0] * frame->height);
        		ret = write(fd, frame->data[1], frame->linesize[1] * frame->height >> 1);
        		ret = write(fd, frame->data[2], frame->linesize[2] * frame->height >> 1);
        		
        		
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












