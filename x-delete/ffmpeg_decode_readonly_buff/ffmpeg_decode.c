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
 
void
error_handle(const char *errorInfo ){
	printf("%s error!\n",errorInfo);
	exit(EXIT_FAILURE);
}

/*
2013-12-11 - 29c83d2 / b9fb59d,409a143 / 9431356,44967ab / d7b3ee9 - lavc 55.45.101 / 55.28.1 - avcodec.h
  av_frame_alloc(), av_frame_unref() and av_frame_free() now can and should be
  used instead of avcodec_alloc_frame(), avcodec_get_frame_defaults() and
  avcodec_free_frame() respectively. The latter three functions are deprecated.
*/
 
#define avcodec_alloc_frame   av_frame_alloc

// ffplay -f rawvideo -video_size 352x288 yuv_420-352x288.yuv

// ffplay -f rawvideo -video_size 352x288 decode_yuv420p_w352xh288.yuv
 
int main(int argc,char ** argv){
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
 
	decodedFrame=avcodec_alloc_frame();
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


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



#include <stdio.h>
#include <stdlib.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>

int main1111() {
    // 初始化 FFmpeg
    av_register_all();
    avcodec_register_all();

    // 创建解码器上下文
    AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec) {
        fprintf(stderr, "Codec not found\n");
        return -1;
    }

    AVCodecContext *codec_context = avcodec_alloc_context3(codec);
    if (!codec_context) {
        fprintf(stderr, "Could not allocate codec context\n");
        return -1;
    }

    // 打开解码器
    if (avcodec_open2(codec_context, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        avcodec_free_context(&codec_context);
        return -1;
    }

    // 假设 data 是您的缓存数据，size 是数据大小
    // 这里用随机数据作为示例，实际使用时请替换为您的 H.264 数据
    uint8_t *data = malloc(1024); // 示例数据
    size_t size = 1024; // 示例数据大小
    // 填充 data 以模拟 H.264 数据...

    AVPacket packet;
    av_init_packet(&packet);
    packet.data = data; // 缓存数据
    packet.size = size; // 数据大小

    // 发送数据到解码器
    if (avcodec_send_packet(codec_context, &packet) == 0) {
        AVFrame *frame = av_frame_alloc();
        if (!frame) {
            fprintf(stderr, "Could not allocate frame\n");
            avcodec_free_context(&codec_context);
            free(data);
            return -1;
        }

        // 接收解码后的帧
        while (avcodec_receive_frame(codec_context, frame) == 0) {
            printf("Decoded frame: %ld\n", frame->pts);
            // 处理解码后的帧，例如转换为 YUV
            // 这里可以将 frame->data 传递给其他处理函数
        }

        av_frame_free(&frame);
    } else {
        fprintf(stderr, "Error sending packet for decoding\n");
    }

    // 释放资源
    avcodec_free_context(&codec_context);
    free(data);

    return 0;
}



























