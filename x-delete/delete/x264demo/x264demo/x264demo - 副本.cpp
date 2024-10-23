// x264demo.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <string>
#include "stdint.h"
#pragma warning(disable : 4996)
#pragma comment(lib, "libx264.lib")
extern "C"
{
#include "x264shared/include/x264.h"
#include "x264shared/include/x264_config.h"
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
using namespace std;

// 读取 YUV 图像
int read_yuv_frame(FILE *fp, x264_picture_t *pic, int width, int height) {
	int y_size = width * height;
	int uv_size = y_size / 4;

	if (fread(pic->img.plane[0], 1, y_size, fp) != y_size) {
		return -1;
	}
	if (fread(pic->img.plane[1], 1, uv_size, fp) != uv_size) {
		return -1;
	}
	if (fread(pic->img.plane[2], 1, uv_size, fp) != uv_size) {
		return -1;
	}

	return 0;
}

int main(int argc, char** argv)
{
	printf("hello word.\n");

	//FILE *in_file = fopen(argv[1], "rb");
	//

	//FILE *out_file = fopen(argv[2], "wb");

	FILE* in_file = fopen("E:\\hs\\hspro\\x264demo\\bin\\yuv_420-352x288.yuv", "rb");
	FILE* out_file = fopen("E:\\hs\\hspro\\x264demo\\bin\\x.h264", "wb");
	
	if (!in_file) {
		fprintf(stderr, "Failed to open input file\n");
		return -1;
	}

	if (!out_file) {
		fprintf(stderr, "Failed to open output file\n");
		fclose(in_file);
		return -1;
	}



	x264_t *encoder;
	x264_picture_t pic_in, pic_out;
	x264_param_t param;
	x264_nal_t *nals;
	int num_nals;

	// 初始化编码参数
	x264_param_default_preset(&param, "medium", "zerolatency");
	param.i_bitdepth = 8;
	param.i_csp = X264_CSP_I420;
	param.i_width = 1280;
	param.i_height = 720;
	param.i_fps_num = 25;
	param.i_fps_den = 1;

	// 应用配置
	x264_param_apply_profile(&param, "high");

	// 打开编码器
	encoder = x264_encoder_open(&param);
	if (!encoder) {
		fprintf(stderr, "Failed to open encoder\n");
		fclose(in_file);
		fclose(out_file);
		return -1;
	}

	// 初始化输入和输出图像
	x264_picture_alloc(&pic_in, param.i_csp, param.i_width, param.i_height);
	x264_picture_init(&pic_out);

	// 编码帧
	int frame_count = 0;
	while (read_yuv_frame(in_file, &pic_in, param.i_width, param.i_height) == 0) {
		pic_in.i_pts = frame_count++;

		int frame_size = x264_encoder_encode(encoder, &nals, &num_nals, &pic_in, &pic_out);
		if (frame_size < 0) {
			fprintf(stderr, "Failed to encode frame\n");
			x264_picture_clean(&pic_in);
			x264_encoder_close(encoder);
			fclose(in_file);
			fclose(out_file);
			return -1;
		}

		for (int i = 0; i < num_nals; i++) {
			fwrite(nals[i].p_payload, 1, nals[i].i_payload, out_file);
		}
	}

	// 清理资源
	x264_picture_clean(&pic_in);
	x264_encoder_close(encoder);
	fclose(in_file);
	fclose(out_file);

	return 0;
# if 0
	int ret;
	int y_size;
	int i, j;

	FILE* fp_src = fopen("E:\\hs\\hspro\\x264demo\\bin\\yuv_420-352x288.yuv", "rb");
	FILE* fp_dst = fopen("E:\\hs\\hspro\\x264demo\\bin\\x.h264", "wb");

	//Encode 50 frame
	//if set 0, encode all frame
	int frame_num = 50;
	int csp = X264_CSP_I420;
	int width = 416, height = 240;

	int iNal = 0;
	x264_nal_t* pNals = NULL;
	x264_t* pHandle = NULL;
	x264_picture_t* pPic_in = (x264_picture_t*)malloc(sizeof(x264_picture_t));
	x264_picture_t* pPic_out = (x264_picture_t*)malloc(sizeof(x264_picture_t));
	x264_param_t* pParam = (x264_param_t*)malloc(sizeof(x264_param_t));

	//Check
	if (fp_src == NULL || fp_dst == NULL) {
		printf("Error open files.\n");
		return -1;
	}

	x264_param_default(pParam);
	pParam->i_width = width;
	pParam->i_height = height;

	//Param
	pParam->i_log_level = X264_LOG_DEBUG;
	pParam->i_threads = X264_SYNC_LOOKAHEAD_AUTO;
	pParam->i_frame_total = 0;
	pParam->i_keyint_max = 10;
	pParam->i_bframe = 5;
	pParam->b_open_gop = 0;
	pParam->i_bframe_pyramid = 0;
	pParam->rc.i_qp_constant = 0;
	pParam->rc.i_qp_max = 0;
	pParam->rc.i_qp_min = 0;
	pParam->i_bframe_adaptive = X264_B_ADAPT_TRELLIS;
	pParam->i_fps_den = 1;
	pParam->i_fps_num = 25;
	pParam->i_timebase_den = pParam->i_fps_num;
	pParam->i_timebase_num = pParam->i_fps_den;

	pParam->i_csp = csp;
	x264_param_apply_profile(pParam, x264_profile_names[5]);

	pHandle = x264_encoder_open(pParam);

	x264_picture_init(pPic_out);
	x264_picture_alloc(pPic_in, csp, pParam->i_width, pParam->i_height);

	//ret = x264_encoder_headers(pHandle, &pNals, &iNal);

	y_size = pParam->i_width * pParam->i_height;
	//detect frame number
	if (frame_num == 0) {
		fseek(fp_src, 0, SEEK_END);
		switch (csp) {
		case X264_CSP_I444:frame_num = ftell(fp_src) / (y_size * 3); break;
		case X264_CSP_I420:frame_num = ftell(fp_src) / (y_size * 3 / 2); break;
		default:printf("Colorspace Not Support.\n"); return -1;
		}
		fseek(fp_src, 0, SEEK_SET);
	}

	//Loop to Encode
	for (i = 0; i < frame_num; i++) {
		switch (csp) {
		case X264_CSP_I444: {
			fread(pPic_in->img.plane[0], y_size, 1, fp_src);         //Y
			fread(pPic_in->img.plane[1], y_size, 1, fp_src);         //U
			fread(pPic_in->img.plane[2], y_size, 1, fp_src);         //V
			break; }
		case X264_CSP_I420: {
			fread(pPic_in->img.plane[0], y_size, 1, fp_src);         //Y
			fread(pPic_in->img.plane[1], y_size / 4, 1, fp_src);     //U
			fread(pPic_in->img.plane[2], y_size / 4, 1, fp_src);     //V
			break; }
		default: {
			printf("Colorspace Not Support.\n");
			return -1; }
		}
		pPic_in->i_pts = i;

		ret = x264_encoder_encode(pHandle, &pNals, &iNal, pPic_in, pPic_out);
		if (ret < 0) {
			printf("Error.\n");
			return -1;
		}

		printf("Succeed encode frame: %5d\n", i);

		for (j = 0; j < iNal; ++j) {
			fwrite(pNals[j].p_payload, 1, pNals[j].i_payload, fp_dst);
		}
	}
	i = 0;
	//flush encoder
	while (1) {
		ret = x264_encoder_encode(pHandle, &pNals, &iNal, NULL, pPic_out);
		if (ret == 0) {
			break;
		}
		printf("Flush 1 frame.\n");
		for (j = 0; j < iNal; ++j) {
			fwrite(pNals[j].p_payload, 1, pNals[j].i_payload, fp_dst);
		}
		i++;
	}
	x264_picture_clean(pPic_in);
	x264_encoder_close(pHandle);
	pHandle = NULL;

	free(pPic_in);
	free(pPic_out);
	free(pParam);

	fclose(fp_src);
	fclose(fp_dst);
#endif 
	return 0;
}
