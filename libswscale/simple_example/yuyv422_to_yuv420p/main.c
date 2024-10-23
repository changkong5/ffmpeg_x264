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

#include <libswscale/swscale.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>

// ffplay -f rawvideo -s 352x288 yuv420p_w352xh288.yuv 																// correct   // play yuv
// ffplay yuv420p_w352xh288.h264 																					// correct   // play h264
// ffmpeg -s 352x288 -i yuv420p_w352xh288.yuv -c:v h264 yuv420p_w352xh288.h264   									// correct	 // yuv ---> h264
// ffmpeg -c:v h264 -i yuv420p_w352xh288.h264 yuv420p_w352xh288.yuv    												// correct   // h264 ---> yuv


// ffplay -s 352x288 yuv420p_w352xh288.yuv 

// ffplay -pixel_format yuyv422 -s 352x288 yuyv422_w352xh288.yuv 

// ffplay -pixel_format yuv420p -s 352x288 yuv420p_w352xh288.yuv 

// ffplay -pixel_format rgb24 -s 352x288 yuv420p_w352xh288.rgb

// ffplay -pixel_format argb -s 352x288 yuv420p_w352xh288.rgb

int main(int argc, char* argv[])
{
	//Parameters	
	FILE *src_file =fopen("yuyv422_w352xh288.yuv", "rb");
	const int src_w=352,src_h=288;
	
	enum AVPixelFormat src_pixfmt=AV_PIX_FMT_YUYV422;
	int src_bpp=av_get_bits_per_pixel(av_pix_fmt_desc_get(src_pixfmt));
	printf("%s: src_bpp = %d\n", __func__, src_bpp);
 
	FILE *dst_file = fopen("yuv420p_w352xh288.yuv", "wb");
	const int dst_w=352,dst_h=288;
	
	enum AVPixelFormat dst_pixfmt=AV_PIX_FMT_YUV420P;
	int dst_bpp=av_get_bits_per_pixel(av_pix_fmt_desc_get(dst_pixfmt));
	printf("%s: dst_bpp = %d\n", __func__, dst_bpp);
 
	//Structures
	uint8_t *src_data[4];
	int src_linesize[4];
 
	uint8_t *dst_data[4];
	int dst_linesize[4];
 
	int rescale_method=SWS_BICUBIC;
	struct SwsContext *img_convert_ctx;
	uint8_t *temp_buffer=(uint8_t *)malloc(src_w*src_h*src_bpp/8);
	printf("%s: temp_buffer = %p\n", __func__, temp_buffer);
	
	int frame_idx=0;
	int ret=0;
	ret=av_image_alloc(src_data, src_linesize,src_w, src_h, src_pixfmt, 1);
	printf("%s: <1> ret = %d\n", __func__, ret);
	if (ret< 0) {
		printf( "Could not allocate source image\n");
		return -1;
	}
	
	ret = av_image_alloc(dst_data, dst_linesize,dst_w, dst_h, dst_pixfmt, 1);
	printf("%s: <2> ret = %d\n", __func__, ret);
	if (ret< 0) {
		printf( "Could not allocate destination image\n");
		return -1;
	}
	//-----------------------------	

/*		
	//Init Method 1
	img_convert_ctx =sws_alloc_context();
	printf("%s: img_convert_ctx = %p\n", __func__, img_convert_ctx);
	
	//Show AVOption
	av_opt_show2(img_convert_ctx,stdout,AV_OPT_FLAG_VIDEO_PARAM,0);
	//Set Value
	av_opt_set_int(img_convert_ctx,"sws_flags",SWS_BICUBIC|SWS_PRINT_INFO,0);
	av_opt_set_int(img_convert_ctx,"srcw",src_w,0);
	av_opt_set_int(img_convert_ctx,"srch",src_h,0);
	av_opt_set_int(img_convert_ctx,"src_format",src_pixfmt,0);
	//'0' for MPEG (Y:0-235);'1' for JPEG (Y:0-255)
	av_opt_set_int(img_convert_ctx,"src_range",1,0);
	av_opt_set_int(img_convert_ctx,"dstw",dst_w,0);
	av_opt_set_int(img_convert_ctx,"dsth",dst_h,0);
	
	av_opt_set_int(img_convert_ctx,"dst_format",dst_pixfmt,0);
	av_opt_set_int(img_convert_ctx,"dst_range",1,0);
	ret = sws_init_context(img_convert_ctx,NULL,NULL);
	printf("%s: <3> ret = %d\n", __func__, ret);
 */
 
	//Init Method 2
	img_convert_ctx = sws_getContext(src_w, src_h,src_pixfmt, dst_w, dst_h, dst_pixfmt, rescale_method, NULL, NULL, NULL); 
	//-----------------------------
	/*
	//Colorspace
	ret=sws_setColorspaceDetails(img_convert_ctx,sws_getCoefficients(SWS_CS_ITU601),0,
		sws_getCoefficients(SWS_CS_ITU709),0,
		 0, 1 << 16, 1 << 16);
	if (ret==-1) {
		printf( "Colorspace not support.\n");
		return -1;
	}
	*/
	while(1)
	{
		printf("%s: src_w = %d, src_h = %d, src_bpp = %d, src_w*src_h*src_bpp/8 = %d\n", __func__, src_w, src_h, src_bpp, src_w*src_h*src_bpp/8);
		
		if (fread(temp_buffer, 1, src_w*src_h*src_bpp/8, src_file) != src_w*src_h*src_bpp/8){
			break;
		}
		
		// AV_PIX_FMT_YUYV422
		memcpy(src_data[0],temp_buffer,src_w*src_h*2);                  //Packed
		
		sws_scale(img_convert_ctx, (const uint8_t* const*)src_data, src_linesize, 0, src_h, dst_data, dst_linesize);
		
		printf("Finish process frame %5d\n",frame_idx);
		frame_idx++;
		
		// AV_PIX_FMT_YUV420P
		fwrite(dst_data[0],1,dst_w*dst_h,dst_file);                 //Y
		fwrite(dst_data[1],1,dst_w*dst_h/4,dst_file);               //U
		fwrite(dst_data[2],1,dst_w*dst_h/4,dst_file);               //V
 
		break;
	}
 
	sws_freeContext(img_convert_ctx);
 
	free(temp_buffer);
	fclose(dst_file);
	av_freep(&src_data[0]);
	av_freep(&dst_data[0]);
 
	return 0;
}

#if 0
int main(int argc, char* argv[])
{
	//Parameters	
	// FILE *src_file =fopen("sintel_480x272_yuv420p.yuv", "rb");
	// const int src_w=480,src_h=272;
	FILE *src_file =fopen("yuv420p_w352xh288.yuv", "rb");
	const int src_w=352,src_h=288;
	
	// AVPixelFormat src_pixfmt=AV_PIX_FMT_YUV420P;
	enum AVPixelFormat src_pixfmt=AV_PIX_FMT_YUV420P;
	int src_bpp=av_get_bits_per_pixel(av_pix_fmt_desc_get(src_pixfmt));
	printf("%s: src_bpp = %d\n", __func__, src_bpp);
 
	//FILE *dst_file = fopen("sintel_1280x720_rgb24.rgb", "wb");
	//const int dst_w=1280,dst_h=720;
	FILE *dst_file = fopen("yuv420p_w352xh288.rgb", "wb");
	const int dst_w=352,dst_h=288;
	
	// AVPixelFormat dst_pixfmt=AV_PIX_FMT_RGB24;
	enum AVPixelFormat dst_pixfmt=AV_PIX_FMT_RGB24;
	int dst_bpp=av_get_bits_per_pixel(av_pix_fmt_desc_get(dst_pixfmt));
	printf("%s: dst_bpp = %d\n", __func__, dst_bpp);
 
	//Structures
	uint8_t *src_data[4];
	int src_linesize[4];
 
	uint8_t *dst_data[4];
	int dst_linesize[4];
 
	int rescale_method=SWS_BICUBIC;
	struct SwsContext *img_convert_ctx;
	uint8_t *temp_buffer=(uint8_t *)malloc(src_w*src_h*src_bpp/8);
	printf("%s: temp_buffer = %p\n", __func__, temp_buffer);
	
	int frame_idx=0;
	int ret=0;
	ret=av_image_alloc(src_data, src_linesize,src_w, src_h, src_pixfmt, 1);
	printf("%s: <1> ret = %d\n", __func__, ret);
	if (ret< 0) {
		printf( "Could not allocate source image\n");
		return -1;
	}
	
	ret = av_image_alloc(dst_data, dst_linesize,dst_w, dst_h, dst_pixfmt, 1);
	printf("%s: <2> ret = %d\n", __func__, ret);
	if (ret< 0) {
		printf( "Could not allocate destination image\n");
		return -1;
	}
	//-----------------------------	

/*		
	//Init Method 1
	img_convert_ctx =sws_alloc_context();
	printf("%s: img_convert_ctx = %p\n", __func__, img_convert_ctx);
	
	//Show AVOption
	av_opt_show2(img_convert_ctx,stdout,AV_OPT_FLAG_VIDEO_PARAM,0);
	//Set Value
	av_opt_set_int(img_convert_ctx,"sws_flags",SWS_BICUBIC|SWS_PRINT_INFO,0);
	av_opt_set_int(img_convert_ctx,"srcw",src_w,0);
	av_opt_set_int(img_convert_ctx,"srch",src_h,0);
	av_opt_set_int(img_convert_ctx,"src_format",src_pixfmt,0);
	//'0' for MPEG (Y:0-235);'1' for JPEG (Y:0-255)
	av_opt_set_int(img_convert_ctx,"src_range",1,0);
	av_opt_set_int(img_convert_ctx,"dstw",dst_w,0);
	av_opt_set_int(img_convert_ctx,"dsth",dst_h,0);
	
	av_opt_set_int(img_convert_ctx,"dst_format",dst_pixfmt,0);
	av_opt_set_int(img_convert_ctx,"dst_range",1,0);
	ret = sws_init_context(img_convert_ctx,NULL,NULL);
	printf("%s: <3> ret = %d\n", __func__, ret);
 */
 
	//Init Method 2
	img_convert_ctx = sws_getContext(src_w, src_h,src_pixfmt, dst_w, dst_h, dst_pixfmt, 
		rescale_method, NULL, NULL, NULL); 
	//-----------------------------
	/*
	//Colorspace
	ret=sws_setColorspaceDetails(img_convert_ctx,sws_getCoefficients(SWS_CS_ITU601),0,
		sws_getCoefficients(SWS_CS_ITU709),0,
		 0, 1 << 16, 1 << 16);
	if (ret==-1) {
		printf( "Colorspace not support.\n");
		return -1;
	}
	*/
	while(1)
	{
		printf("%s: src_w = %d, src_h = %d, src_bpp = %d, src_w*src_h*src_bpp/8 = %d\n", __func__, src_w, src_h, src_bpp, src_w*src_h*src_bpp/8);
		
		if (fread(temp_buffer, 1, src_w*src_h*src_bpp/8, src_file) != src_w*src_h*src_bpp/8){
			break;
		}
		
		switch(src_pixfmt){
		case AV_PIX_FMT_GRAY8:{
			memcpy(src_data[0],temp_buffer,src_w*src_h);
			break;
							  }
		case AV_PIX_FMT_YUV420P:{
			memcpy(src_data[0],temp_buffer,src_w*src_h);                    //Y
			memcpy(src_data[1],temp_buffer+src_w*src_h,src_w*src_h/4);      //U
			memcpy(src_data[2],temp_buffer+src_w*src_h*5/4,src_w*src_h/4);  //V
			break;
								}
		case AV_PIX_FMT_YUV422P:{
			memcpy(src_data[0],temp_buffer,src_w*src_h);                    //Y
			memcpy(src_data[1],temp_buffer+src_w*src_h,src_w*src_h/2);      //U
			memcpy(src_data[2],temp_buffer+src_w*src_h*3/2,src_w*src_h/2);  //V
			break;
								}
		case AV_PIX_FMT_YUV444P:{
			memcpy(src_data[0],temp_buffer,src_w*src_h);                    //Y
			memcpy(src_data[1],temp_buffer+src_w*src_h,src_w*src_h);        //U
			memcpy(src_data[2],temp_buffer+src_w*src_h*2,src_w*src_h);      //V
			break;
								}
		case AV_PIX_FMT_YUYV422:{
			memcpy(src_data[0],temp_buffer,src_w*src_h*2);                  //Packed
			break;
								}
		case AV_PIX_FMT_RGB24:{
			memcpy(src_data[0],temp_buffer,src_w*src_h*3);                  //Packed
			break;
								}
		default:{
			printf("Not Support Input Pixel Format.\n");
			break;
							  }
		}
		
		sws_scale(img_convert_ctx, (const uint8_t* const*)src_data, src_linesize, 0, src_h, dst_data, dst_linesize);
		printf("Finish process frame %5d\n",frame_idx);
		frame_idx++;
 
		switch(dst_pixfmt){
		case AV_PIX_FMT_GRAY8:{
			fwrite(dst_data[0],1,dst_w*dst_h,dst_file);	
			break;
							  }
		case AV_PIX_FMT_YUV420P:{
			fwrite(dst_data[0],1,dst_w*dst_h,dst_file);                 //Y
			fwrite(dst_data[1],1,dst_w*dst_h/4,dst_file);               //U
			fwrite(dst_data[2],1,dst_w*dst_h/4,dst_file);               //V
			break;
								}
		case AV_PIX_FMT_YUV422P:{
			fwrite(dst_data[0],1,dst_w*dst_h,dst_file);					//Y
			fwrite(dst_data[1],1,dst_w*dst_h/2,dst_file);				//U
			fwrite(dst_data[2],1,dst_w*dst_h/2,dst_file);				//V
			break;
								}
		case AV_PIX_FMT_YUV444P:{
			fwrite(dst_data[0],1,dst_w*dst_h,dst_file);                 //Y
			fwrite(dst_data[1],1,dst_w*dst_h,dst_file);                 //U
			fwrite(dst_data[2],1,dst_w*dst_h,dst_file);                 //V
			break;
								}
		case AV_PIX_FMT_YUYV422:{
			fwrite(dst_data[0],1,dst_w*dst_h*2,dst_file);               //Packed
			break;
								}
		case AV_PIX_FMT_RGB24:{
			fwrite(dst_data[0],1,dst_w*dst_h*3,dst_file);               //Packed
			break;
							  }
		default:{
			printf("Not Support Output Pixel Format.\n");
			break;
							}
		}
		
		break;
	}
 
	sws_freeContext(img_convert_ctx);
 
	free(temp_buffer);
	fclose(dst_file);
	av_freep(&src_data[0]);
	av_freep(&dst_data[0]);
 
	return 0;
}

#endif

#if 0
// ffplay -f rawvideo -s 352x288 yuv420p_w352xh288.yuv 																// correct   // play yuv
// ffplay yuv420p_w352xh288.h264 																					// correct   // play h264
// ffmpeg -s 352x288 -i yuv420p_w352xh288.yuv -c:v h264 yuv420p_w352xh288.h264   									// correct	 // yuv ---> h264
// ffmpeg -c:v h264 -i yuv420p_w352xh288.h264 yuv420p_w352xh288.yuv    												// correct   // h264 ---> yuv

int main(int argc, char *argv[])
{
	int fd, ret;
	int i_fd, got_picture;
	AVCodec *codec;
	AVCodecContext *codec_ctx;
	AVCodecParserContext *codec_parser;
	// AVFormatContext *format_context = NULL;
	AVFrame *frame;
	AVPacket *packet;
	char *buffer = NULL;
	int size, zero_cont;
	int len = 0, rbyte;
	char buf[4096];
	char *data = buf;
	int fps = 25;
	int64_t pts = 0;
	
	fd = open("yuv420p_w352xh288.h264", O_RDWR | O_CREAT, 0666);
	
	i_fd = open("yuv420p_w352xh288.yuv", O_RDWR, 0666);
	
	// codec = avcodec_find_encoder_by_name("libx264");
	// codec = avcodec_find_encoder_by_name("h264");
	codec = avcodec_find_encoder(AV_CODEC_ID_H264);

	if (!codec) {
		printf("%s: <avcodec_find_decoder error>\n", __func__);
	}
	
	codec_ctx = avcodec_alloc_context3(codec);
	
	if (!codec_ctx) {
		printf("%s: <avcodec_alloc_context3 error>\n", __func__);
	}
	
	// codec_ctx->bit_rate = 3000000;
	codec_ctx->bit_rate = 90000;
	codec_ctx->width = 352;
    codec_ctx->height = 288;
    codec_ctx->codec_id = AV_CODEC_ID_H264;
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
    
    codec_ctx->qmin = 30; // 10 - 30   // codec_ctx->qmin = 10 , picture_size = 2353227 and  codec_ctx->qmin = 30 , picture_size = 180202
    codec_ctx->qmax = 51;
    
    //codec_ctx->me_range = 16;
    //codec_ctx->max_qdiff = 4;
    codec_ctx->qcompress = 0.6;
    
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
    
	packet = av_packet_alloc();

	if (!packet) {
		printf("%s: <av_packet_alloc error>\n", __func__);
	}
	

	frame = av_frame_alloc();

	if (!frame) {
		printf("%s: <av_frame_alloc error>\n", __func__);
	}
	
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
/*        
        int need_size = av_image_fill_arrays(frame->data, frame->linesize, buffer,
                                             frame->format,
                                             frame->width, frame->height, 1);
                                             
        if (need_size != size) {
            printf("av_image_fill_arrays failed, need_size:%d, size:%d\n", need_size, size);
            break;
        }
*/        
        pts += 40;
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
	}
	
	close(i_fd);
	close(fd);
	av_freep(&buffer);
	
	av_packet_free(&packet);
	av_frame_free(&frame);
	// av_parser_close(codec_parser);
	avcodec_free_context(&codec_ctx);

	return 0;
}

#endif





