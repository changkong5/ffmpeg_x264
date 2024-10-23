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


// main.c:35:9:   warning: ‘av_register_all’ is deprecated [-Wdeprecated-declarations]
// main.c:52:9:   warning: ‘codec’ is deprecated [-Wdeprecated-declarations]					//   52 |         codec_context = format_context->streams[video_stream]->codec;
// main.c:76:9:   warning: ‘avpicture_get_size’ is deprecated [-Wdeprecated-declarations]
// main.c:85:9:   warning: ‘av_init_packet’ is deprecated [-Wdeprecated-declarations]
// main.c:90:17:  warning: ‘avcodec_decode_video2’ is deprecated [-Wdeprecated-declarations]
// main.c:97:25:  warning: ‘avpicture_layout’ is deprecated [-Wdeprecated-declarations]
// main.c:102:17: warning: ‘av_free_packet’ is deprecated [-Wdeprecated-declarations]


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












