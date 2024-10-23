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


#if 0
static void encode(AVCodecContext *ctx, AVFrame *frame, AVPacket *pkt, FILE *f) {
    int ret = -1;

    // 向编码器提供视频帧
    ret = avcodec_send_frame(ctx, frame);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to send frame to encoder : %s \n", av_err2str(ret));
    }

    while (ret >= 0) {
        // 从编码器读取编码数据
        ret = avcodec_receive_packet(ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return;
        } else if (ret < 0) {
            exit(-1);
        }
        fwrite(pkt->data, 1, pkt->size, f);
        av_packet_unref(pkt);
    }
}

// avcodec_find_encoder_by_name("h264")
// avcodec_find_encoder_by_name("libx264")
// AVCodecID codec_id = AV_CODEC_ID_H264;

#if 0
AVCodec ff_h264_decoder = {  
    .name                  = "h264",  
    .long_name             = NULL_IF_CONFIG_SMALL("H.264 / AVC / MPEG-4 AVC / MPEG-4 part 10"),  
    .type                  = AVMEDIA_TYPE_VIDEO,  
    .id                    = AV_CODEC_ID_H264,  
    .priv_data_size        = sizeof(H264Context),  
    .init                  = ff_h264_decode_init,  
    .close                 = h264_decode_end,  
    .decode                = h264_decode_frame,  
    .capabilities          = /*CODEC_CAP_DRAW_HORIZ_BAND |*/ CODEC_CAP_DR1 |  
                             CODEC_CAP_DELAY | CODEC_CAP_SLICE_THREADS |  
                             CODEC_CAP_FRAME_THREADS,  
    .flush                 = flush_dpb,  
    .init_thread_copy      = ONLY_IF_THREADS_ENABLED(decode_init_thread_copy),  
    .update_thread_context = ONLY_IF_THREADS_ENABLED(ff_h264_update_thread_context),  
    .profiles              = NULL_IF_CONFIG_SMALL(profiles),  
    .priv_class            = &h264_class,  
}; 
#endif

int main(int argc, char *argv[])
{
    char *dst = NULL;
    char *codecName = NULL;
    AVCodec *codec = NULL;
    AVCodecContext *ctx = NULL;
    int ret = -1;
    FILE *f = NULL;
    AVFrame *frame = NULL;
    AVPacket *pkt = NULL;
   //处理参数
    av_log_set_level(AV_LOG_DEBUG);
    if (argc < 3)
    {
        av_log(NULL, AV_LOG_ERROR, "arguments must be more than 3\n");
        exit(-1);
    }
    //输入编码视频名称和编码器名称
    // dst = argv[1];
    // codecName = argv[2];
    //寻找编码器
    // codec = avcodec_find_encoder_by_name(codecName);
    
    dst = "yuv420p_w352xh288.yuv";
    codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    
    if (!codec)
    {
        av_log(NULL, AV_LOG_ERROR, "don't find codec : %s", codecName);
        goto ERROR;
    }
    //创建编码器上下文
    ctx = avcodec_alloc_context3(codec);
    if (!ctx)
    {
        av_log(NULL, AV_LOG_ERROR, "NO MEMORY\n");
        goto ERROR;
    }
   //初始化编码器上下文，设置分辨率，帧率，时间基，帧率，GOP大小，是否存在b帧，像素格式
    ctx->width = 640;
    ctx->height = 480;
    ctx->bit_rate = 500000;
    ctx->time_base = (AVRational){1, 25};
    ctx->framerate = (AVRational){25, 1};
    ctx->gop_size = 10;
    ctx->max_b_frames = 1;
    ctx->pix_fmt = AV_PIX_FMT_YUV422P;

    if (codec->id == AV_CODEC_ID_H264)
    {
        av_opt_set(ctx->priv_data, "preset", "slow", 0);
    }
    //绑定编码器和编码器上下文
   //这个函数实际打开编码器，并使用上面的设置
    ret = avcodec_open2(ctx, codec, NULL);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "don't open codec : %s\n", av_err2str(ret));
        goto ERROR;
    }
    //打开目标文件
    f = fopen(dst, "wb");
    if (!f)
    {
        av_log(NULL, AV_LOG_ERROR, "don't open file : %s\n", dst);
        goto ERROR;
    }
   //创建AVFrame
    frame = av_frame_alloc();
    if (!frame)
    {
        av_log(NULL, AV_LOG_ERROR, "NO MEMORY\n");
        goto ERROR;
    }
    //在调用下面的分配缓存区函数之前必须设置的参数
    frame->width=ctx->width;
    frame->height=ctx->height;
    frame->format=ctx->pix_fmt;
    //为音频或者视频数据分配缓冲区
    ret = av_frame_get_buffer(frame, 0);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "could not allocate the video frame\n");
        goto ERROR;
    }
   //创建AVPacket
    pkt = av_packet_alloc();
    if (!pkt)
    {
        av_log(NULL, AV_LOG_ERROR, "NO MEMORY\n");
        goto ERROR;
    }
    //生成1秒钟视频，25帧
    for (int i = 0; i < 25; i++)
    {
       //确保帧数据可写，如果帧可写，则不执行任何操作，如果不可写，则分配新缓冲区并复制数据。
        ret = av_frame_make_writable(frame);
        if (ret < 0)
        {
            break;
        }
        //Y分量
        for(int y=0;y<ctx->height;y++){
            for(int x=0;x<ctx->width;x++){
                //frame->linesize表示每个图像行的大小
                frame->data[0][y*frame->linesize[0]+x]=x+y+i*3;
            }
        }
        //UV分量
        for(int y=0;y<ctx->height/2;y++){
            for(int x=0;x<ctx->width/2;x++){
                frame->data[1][y*frame->linesize[1]+x]=128+y+i*2;
                frame->data[2][y*frame->linesize[2]+x]=64+x+i*5;
            }
        }
        //时间戳
        frame->pts=i;

        encode(ctx,frame,pkt,f);
    }

    encode(ctx,NULL,pkt,f);

ERROR:
    if (ctx)
    {
        avcodec_free_context(&ctx);
    }
    if (frame)
    {
        av_frame_free(&frame);
    }
    if (pkt)
    {
        av_packet_free(&pkt);
    }
    if (f)
    {
        fclose(f);
    }
    return 0;
}
#endif

#if 1
/**
* @projectName   08-02-encode_video
* @brief         视频编码，从本地读取YUV数据进行H264编码
* @author        Liao Qingfu
* @date          2020-04-16
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#include <libavcodec/avcodec.h>
#include <libavutil/time.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
 
int64_t get_time()
{
    return av_gettime_relative() / 1000;  // 换算成毫秒
}

static int encode(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt, FILE *outfile)
{
    int ret;
 
    /* send the frame to the encoder */
    if (frame)
        printf("Send frame %3"PRId64"\n", frame->pts);
    /* 通过查阅代码，使用x264进行编码时，具体缓存帧是在x264源码进行，
     * 不会增加avframe对应buffer的reference*/
    ret = avcodec_send_frame(enc_ctx, frame);
    if (ret < 0)
    {
        fprintf(stderr, "Error sending a frame for encoding\n");
        return -1;
    }
 
    while (ret >= 0)
    {
        ret = avcodec_receive_packet(enc_ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return 0;
        } else if (ret < 0) {
            fprintf(stderr, "Error encoding audio frame\n");
            return -1;
        }
 
        if(pkt->flags & AV_PKT_FLAG_KEY)
            printf("Write packet flags:%d pts:%3"PRId64" dts:%3"PRId64" (size:%5d)\n",
               pkt->flags, pkt->pts, pkt->dts, pkt->size);
        if(!pkt->flags)
            printf("Write packet flags:%d pts:%3"PRId64" dts:%3"PRId64" (size:%5d)\n",
               pkt->flags, pkt->pts, pkt->dts, pkt->size);
        fwrite(pkt->data, 1, pkt->size, outfile);
    }
    return 0;
}
/**
 * @brief 提取测试文件：ffmpeg -i test_1280x720.flv -t 5 -r 25 -pix_fmt yuv420p yuv420p_1280x720.yuv
 *           参数输入: yuv420p_1280x720.yuv yuv420p_1280x720.h264 libx264
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char **argv)
{
    int ret = 0;
    //const char *in_yuv_file = "yuv420p_1280x720.yuv";      // 输入YUV文件
    //const char *out_h264_file = "yuv420p_1280x720.h264";
    
    const char *in_yuv_file = "yuv420p_w352xh288.yuv";      // 输入YUV文件
    const char *out_h264_file = "yuv420p_w352xh288.h264";
    
    const char *codec_name = "libx264";
 
    // 1.查找编码器
    const AVCodec *codec = avcodec_find_encoder_by_name(codec_name);
    if (!codec) {
        fprintf(stderr, "Codec '%s' not found\n", codec_name);
        exit(1);
    }
 
    // 2.分配内存
    AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        fprintf(stderr, "Could not allocate video codec context\n");
        exit(1);
    }
 
    // 3.设置编码参数
    /* 设置分辨率*/
    //codec_ctx->width = 1280;
    //codec_ctx->height = 720;
    codec_ctx->width = 352;
    codec_ctx->height = 288;
    /* 设置time base */
    codec_ctx->time_base = (AVRational){1, 25};
    codec_ctx->framerate = (AVRational){25, 1};
    /* 设置I帧间隔
     * 如果frame->pict_type设置为AV_PICTURE_TYPE_I, 则忽略gop_size的设置，一直当做I帧进行编码
     */
    codec_ctx->gop_size = 25;   // I帧间隔
    codec_ctx->max_b_frames = 2; // 如果不想包含B帧则设置为0
    codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    //
    if (codec->id == AV_CODEC_ID_H264) {
        // 相关的参数可以参考libx264.c的 AVOption options
        ret = av_opt_set(codec_ctx->priv_data, "preset", "medium", 0);
        if(ret != 0) {
            printf("av_opt_set preset failed\n");
        }
        ret = av_opt_set(codec_ctx->priv_data, "profile", "main", 0); // 默认是high
        if(ret != 0) {
            printf("av_opt_set profile failed\n");
        }
        ret = av_opt_set(codec_ctx->priv_data, "tune","zerolatency",0); // 直播是才使用该设置
//        ret = av_opt_set(codec_ctx->priv_data, "tune","film",0); //  画质film
        if(ret != 0) {
            printf("av_opt_set tune failed\n");
        }
    }
 
    /* 设置bitrate */
    codec_ctx->bit_rate = 3000000;
//    codec_ctx->rc_max_rate = 3000000;
//    codec_ctx->rc_min_rate = 3000000;
//    codec_ctx->rc_buffer_size = 2000000;
//    codec_ctx->thread_count = 4;  // 开了多线程后也会导致帧输出延迟, 需要缓存thread_count帧后再编程。
//    codec_ctx->thread_type = FF_THREAD_FRAME; // 并 设置为FF_THREAD_FRAME
 
    //对于H264 AV_CODEC_FLAG_GLOBAL_HEADER  设置则只包含I帧，此时sps pps需要从codec_ctx->extradata读取 不设置则每个I帧都带 sps pps sei
    //codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER; // 存本地文件时不要去设置
 
    // 4.将codec_ctx和codec进行绑定
    ret = avcodec_open2(codec_ctx, codec, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not open codec: %s\n", av_err2str(ret));
        exit(1);
    }
    printf("thread_count: %d, thread_type:%d\n", codec_ctx->thread_count, codec_ctx->thread_type);
 
    // 5.打开输入和输出文件
    FILE *infile = fopen(in_yuv_file, "rb");
    if (!infile) {
        fprintf(stderr, "Could not open %s\n", in_yuv_file);
        exit(1);
    }
    FILE *outfile = fopen(out_h264_file, "wb");
    if (!outfile) {
        fprintf(stderr, "Could not open %s\n", out_h264_file);
        exit(1);
    }
 
    // 6.分配pkt和frame
    AVPacket *pkt = av_packet_alloc();
    if (!pkt) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }
    AVFrame *frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }
 
    // 7.为frame分配buffer
    frame->format = codec_ctx->pix_fmt;
    frame->width  = codec_ctx->width;
    frame->height = codec_ctx->height;
    ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate the video frame data\n");
        exit(1);
    }
    // 计算出每一帧的数据 像素格式 * 宽 * 高
    // 1382400
    int frame_bytes = av_image_get_buffer_size(frame->format, frame->width,
                                               frame->height, 1);
    printf("frame_bytes %d\n", frame_bytes);
    uint8_t *yuv_buf = (uint8_t *)malloc(frame_bytes);
    if(!yuv_buf) {
        printf("yuv_buf malloc failed\n");
        return 1;
    }
    int64_t begin_time = get_time();
    int64_t end_time = begin_time;
    int64_t all_begin_time = get_time();
    int64_t all_end_time = all_begin_time;
    int64_t pts = 0;
 
    // 8.循环读取数据
    printf("start enode\n");
    for (;;) {
        memset(yuv_buf, 0, frame_bytes);
        size_t read_bytes = fread(yuv_buf, 1, frame_bytes, infile);
        if(read_bytes <= 0) {
            printf("read file finish\n");
            break;
        }
        /* 确保该frame可写, 如果编码器内部保持了内存参考计数，则需要重新拷贝一个备份
            目的是新写入的数据和编码器保存的数据不能产生冲突
        */
        int frame_is_writable = 1;
        if(av_frame_is_writable(frame) == 0) { // 这里只是用来测试
            printf("the frame can't write, buf:%p\n", frame->buf[0]);
            if(frame->buf && frame->buf[0])        // 打印referenc-counted，必须保证传入的是有效指针
                printf("ref_count1(frame) = %d\n", av_buffer_get_ref_count(frame->buf[0]));
            frame_is_writable = 0;
        }
        ret = av_frame_make_writable(frame);
        if(frame_is_writable == 0) {  // 这里只是用来测试
            printf("av_frame_make_writable, buf:%p\n", frame->buf[0]);
            if(frame->buf && frame->buf[0])        // 打印referenc-counted，必须保证传入的是有效指针
                printf("ref_count2(frame) = %d\n", av_buffer_get_ref_count(frame->buf[0]));
        }
        if(ret != 0) {
            printf("av_frame_make_writable failed, ret = %d\n", ret);
            break;
        }
        int need_size = av_image_fill_arrays(frame->data, frame->linesize, yuv_buf,
                                             frame->format,
                                             frame->width, frame->height, 1);
        if(need_size != frame_bytes) {
            printf("av_image_fill_arrays failed, need_size:%d, frame_bytes:%d\n",
                   need_size, frame_bytes);
            break;
        }
         pts += 40;
        // 设置pts
        frame->pts = pts;       // 使用采样率作为pts的单位，具体换算成秒 pts*1/采样率
        begin_time = get_time();
        ret = encode(codec_ctx, frame, pkt, outfile);
        end_time = get_time();
        printf("encode time:%ldms\n", end_time - begin_time);
        if(ret < 0) {
            printf("encode failed\n");
            break;
        }
    }
 
    // 9.冲刷编码器
    encode(codec_ctx, NULL, pkt, outfile);
    all_end_time = get_time();
    printf("all encode time:%ldms\n", all_end_time - all_begin_time);

/* 
    // 10.结束
    fclose(infile);
    fclose(outfile);
 
    // 释放内存
    if(yuv_buf) {
        free(yuv_buf);
    }
 
    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_free_context(&codec_ctx);
 */
    //printf("main finish, please enter Enter and exit\n");
    //getchar();
    return 0;
}
#endif



#if 0
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

#endif



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











