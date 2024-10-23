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

#include <libavutil/imgutils.h>

/*
extern "C" {
    #include <libavcodec/avcodec.h>
}
*/

// main.c:30:5:  warning: ‘av_register_all’ is deprecated [-Wdeprecated-declarations]
// main.c:83:9:  warning: ‘av_init_packet’ is deprecated [-Wdeprecated-declarations]
// main.c:103:9: warning: ‘avcodec_encode_video2’ is deprecated [-Wdeprecated-declarations]

int main(int argc, char* argv[]) {
    //初始化AVCodecContext和AVFrame
    AVCodecContext *pCodecCtx = NULL;
    AVFrame *pFrame = NULL;
    int i, ret, got_output;
    FILE *fp_in;
    FILE *fp_out;
    AVPacket pkt;
    av_register_all();
    //打开输入文件，读取原始YUV数据
    fp_in = fopen("decode_yuv420p_w352xh288.yuv", "rb");
    if (!fp_in) {
        printf("Could not open input file.");
        return -1;
    }
    //打开输出文件，准备写入H.264编码后的数据
    fp_out = fopen("output.h264", "wb");
    if (!fp_out) {
        printf("Could not open output file.");
        return -1;
    }
  	//创建一个编码器上下文并设置参数
  	AVCodec *pCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
  	if (!pCodec) {
  		printf("Codec not found.");
  		return -1;
  	}
  
  	pCodecCtx = avcodec_alloc_context3(pCodec);
  	if (!pCodecCtx) {
  		printf("Could not allocate video codec context.");
  		return -1;
  	}
  
  	// pCodecCtx->bit_rate = 400000;  //设置比特率为400kbps
  	pCodecCtx->width = 352;       //视频宽度为640像素
  	pCodecCtx->height = 288;      //视频高度为480像素
  	pCodecCtx->time_base.num = 1; //帧率为25帧每秒
  	pCodecCtx->time_base.den = 25;
  	pCodecCtx->gop_size = 10;     //设置I帧间隔（GOP大小）
  	pCodecCtx->max_b_frames = 1;  //最大B帧数为1
  	pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    avcodec_open2(pCodecCtx, pCodec, NULL);
    //分配AVFrame并设置其属性
    pFrame = av_frame_alloc();
    if (!pFrame) {
        printf("Could not allocate video frame.");
        return -1;
    }
  
  	pFrame->width = pCodecCtx->width;
  	pFrame->height = pCodecCtx->height;
  	pFrame->format = pCodecCtx->pix_fmt;
  
    ret = av_image_alloc(pFrame->data, pFrame->linesize, pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, 32);
    if (ret < 0) {
        printf("Could not allocate raw picture buffer.");
        return -1;
    }
    //从输入文件中读取原始数据并编码输出
    for (i=0; i<25; i++) {      //循环25次，模拟25帧视频
        av_init_packet(&pkt);
        pkt.data = NULL; 
        pkt.size = 0;
      	//读取一帧YUV数据到AVFrame中
      	if (fread(pFrame->data[0], 1, pCodecCtx->width*pCodecCtx->
      		height, fp_in) != (unsigned)pCodecCtx->width*pCodecCtx->height) {
      		break;
      	}
      	if (fread(pFrame->data[1], 1, pCodecCtx->width/2*pCodecCtx->
      		height/2, fp_in) != (unsigned)pCodecCtx->width/2*pCodecCtx->
      		height/2) {
      		break;
      	}
      	if (fread(pFrame->data[2], 1, pCodecCtx->width/2*pCodecCtx->
      		height/2, fp_in) != (unsigned)pCodecCtx->width/2*pCodecCtx->
      		height/2) {
      		break;
      	}
        pFrame->pts = i;
        //编码一帧视频并将其输出到文件中
        ret = avcodec_encode_video2(pCodecCtx, &pkt, pFrame, &got_output);
        if (ret < 0) {
            printf("Error encoding video frame.");
            return -1;
        }
        if (got_output) {
            fwrite(pkt.data, 1, pkt.size, fp_out);
            av_packet_unref(&pkt);
        }
    }
    //清理工作
    fclose(fp_in);
    fclose(fp_out);
    avcodec_close(pCodecCtx);
    av_free(pFrame);
    avcodec_free_context(&pCodecCtx);
    return 0;
}



#if 0
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











