#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#include <libavcodec/avcodec.h>
#include <libavutil/time.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>


// ffplay -f rawvideo -s 352x288 yuv420p_w352xh288.yuv 																// correct   // play yuv
// ffplay yuv420p_w352xh288.h264 																					// correct   // play h264
// ffmpeg -s 352x288 -i yuv420p_w352xh288.yuv -c:v h264 yuv420p_w352xh288.h264   									// correct	 // yuv ---> h264
// ffmpeg -c:v h264 -i yuv420p_w352xh288.h264 yuv420p_w352xh288.yuv    												// correct   // h264 ---> yuv

// ffmpeg -h encoder=mjpeg
// ffmpeg -h encoder=mpeg4
// ffmpeg -h encoder=png

// lsb@lsb:ffmpeg_encode$ ffmpeg -h encoder=png
// Supported pixel formats: rgb24 rgba rgb48be rgba64be pal8 gray ya8 gray16be ya16be monob

// lsb@lsb:ffmpeg_encode$ ffmpeg -h encoder=mpeg4
// Supported pixel formats: yuv420p

// lsb@lsb:ffmpeg_encode$ ffmpeg -h encoder=mjpeg
// Supported pixel formats: yuvj420p yuvj422p yuvj444p

// lsb@lsb:~$ ffmpeg -h encoder=gif
// Supported pixel formats: rgb8 bgr8 rgb4_byte bgr4_byte gray pal8


// A..... flac                 FLAC (Free Lossless Audio Codec)
// A..X.. opus                 Opus


// ffplay yuv420p_w352xh288.mp4

int main(int argc, char *argv[])
{
	int fd, ret;
	int i_fd, got_picture;
	AVCodec *codec;
	AVCodecContext *codec_ctx;
	AVFrame *frame;
	AVPacket *packet;
	char *buffer = NULL;
	int size, zero_cont;
	int len = 0, rbyte;
	char buf[4096];
	char *data = buf;
	int fps = 25;
	int64_t pts = 0;
	
	// fd = open("yuv420p_w352xh288.h264", O_RDWR | O_CREAT, 0666);
	
	// fd = open("yuv420p_w352xh288.jpg", O_RDWR | O_CREAT, 0666);
	
	fd = open("yuv420p_w352xh288.mp4", O_RDWR | O_CREAT, 0666);
	
	i_fd = open("yuv420p_w352xh288.yuv", O_RDWR, 0666);
	// i_fd = open("rgb24_w352xh288.rgb", O_RDWR, 0666);
	
	// codec = avcodec_find_encoder_by_name("libx264");
	// codec = avcodec_find_encoder_by_name("h264");
	// codec = avcodec_find_encoder(AV_CODEC_ID_H264);
	
	// codec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
	// codec = avcodec_find_encoder(AV_CODEC_ID_PNG);
	
	// avcodec_find_encoder(AV_CODEC_ID_AAC)  
	// avcodec_find_decoder(AV_CODEC_ID_AAC)
	
	codec = avcodec_find_encoder(AV_CODEC_ID_MPEG4);

	if (!codec) {
		printf("%s: <avcodec_find_decoder error>\n", __func__);
	}
	
	codec_ctx = avcodec_alloc_context3(codec);
	
	if (!codec_ctx) {
		printf("%s: <avcodec_alloc_context3 error>\n", __func__);
	}
	
	// pCodecCtx->bit_rate = 400000;
	// codec_ctx->bit_rate = 3000000;
	// codec_ctx->bit_rate = 90000;
	codec_ctx->bit_rate = 400000;
	codec_ctx->width = 352;
    codec_ctx->height = 288;
    // codec_ctx->codec_id = AV_CODEC_ID_H264;
    
    codec_ctx->codec_id = AV_CODEC_ID_MPEG4;
    codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
    codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    
    fps = 25;
    codec_ctx->framerate.num = fps;  //帧率
    codec_ctx->framerate.den = 1;
    codec_ctx->time_base.num = 1;
    codec_ctx->time_base.den = fps;  //time_base一般是帧率的倒数，但不总是
 
    // codec_ctx->time_base = (AVRational){1, 25};
    // codec_ctx->framerate = (AVRational){25, 1};
    codec_ctx->gop_size = 10;
    codec_ctx->max_b_frames = 1;
    
    codec_ctx->qmin = 20; // 10 - 30   // codec_ctx->qmin = 10 , picture_size = 2353227 and  codec_ctx->qmin = 30 , picture_size = 180202
    codec_ctx->qmax = 51;
    
    //codec_ctx->me_range = 16;
    //codec_ctx->max_qdiff = 4;
    codec_ctx->qcompress = 0.6;
 /*   
    if (0 && codec->id == AV_CODEC_ID_H264) {
        ret = av_opt_set(codec_ctx->priv_data, "preset", "medium", 0);
        if(ret != 0) {
            printf("av_opt_set preset failed\n");
        }
        
        ret = av_opt_set(codec_ctx->priv_data, "profile", "main", 0); 
        if(ret != 0) {
            printf("av_opt_set profile failed\n");
        }
        
        ret = av_opt_set(codec_ctx->priv_data, "tune","zerolatency",0);
        if(ret != 0) {
            printf("av_opt_set tune failed\n");
        }
    }
    
    if (codec->id == AV_CODEC_ID_H264) {
        av_opt_set(codec_ctx->priv_data, "preset", "slow", 0);
    }
*/    
	packet = av_packet_alloc();

	if (!packet) {
		printf("%s: <av_packet_alloc error>\n", __func__);
	}
	

	frame = av_frame_alloc();

	if (!frame) {
		printf("%s: <av_frame_alloc error>\n", __func__);
	}
	
	// frame->format = AV_PIX_FMT_RGB24;
	
	frame->format = codec_ctx->pix_fmt;
    frame->width  = codec_ctx->width;
    frame->height = codec_ctx->height;
    ret = av_frame_get_buffer(frame, 0);
    
    if (ret != 0) {
     	printf("Could not allocate the video frame data\n");
    }
    
	ret = avcodec_open2(codec_ctx, codec, NULL);

	if (ret < 0) {
		printf("%s: <avcodex_open2 error> ret = %d\n", __func__, ret);
	}

	zero_cont = 0;

	size = av_image_get_buffer_size(frame->format, frame->width, frame->height, 1);
	printf("%s: size = %d\n", __func__, size);
	buffer = av_mallocz(size * sizeof(uint8_t));
	
	int need_size = av_image_fill_arrays(frame->data, frame->linesize, buffer,
                                             frame->format, frame->width, frame->height, 1);
                                             
    printf("%s: size = %d, need_size = %d\n", __func__, size, need_size);                                         
	
	while (1) {
		rbyte = read(i_fd, buffer, size);
		printf("%s: zero_cont = %d, len = %d, rbyte = %d\n", __func__, zero_cont, len, rbyte);
		
		if (rbyte == 0) {
			zero_cont++;
			
			if (zero_cont == 1) {
				printf(">>>>>>>>>>>>>>>>>>>>>>> <flush_encode> \n");
			} else {
				break;
			}
		}

		ret = av_frame_make_writable(frame);
		
		if (ret != 0) {
            printf("av_frame_make_writable failed, ret = %d\n", ret);
            break;
        }
       
        // pts += 40;
        pts += 1;
        frame->pts = pts;
        printf("Send frame %ld \n", frame->pts);

		ret = avcodec_send_frame(codec_ctx, frame);     
        // printf("%s: <avcodec_send_frame> ret = %d\n", __func__, ret);
        
        if (ret < 0) {
		    fprintf(stderr, "Error sending a frame for encoding\n");
		    break;
		}
        
        while (1) {
        	ret = avcodec_receive_packet(codec_ctx, packet);
        	// printf("%s: <avcodec_receive_packet> ret = %d, AVERROR(EAGAIN) = %d, AVERROR_EOF = %d\n", __func__, ret, AVERROR(EAGAIN), AVERROR_EOF);
        	
        	 if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            	break;
        	} else if (ret < 0) {
            	return 0;
        	}
        	
        	printf("%s: <avcodec_send_frame> packet->size = %d\n", __func__, packet->size);
        	for (int i = 0; i < 9; i++) printf("0x%02x ", packet->data[i]); printf("\n");
        	
        	ret = write(fd, packet->data, packet->size);
        }
        
        // break;
	}
	
	close(i_fd);
	close(fd);
	av_freep(&buffer);
	
	av_packet_free(&packet);
	av_frame_free(&frame);
	avcodec_free_context(&codec_ctx);

	return 0;
}







