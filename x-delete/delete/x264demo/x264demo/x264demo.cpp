#include <iostream>
#include <string>
#include "stdint.h"
#pragma warning(disable : 4996) // 禁用4996号警告
#pragma comment(lib, "libx264.lib") // 链接x264库文件
extern "C"
{
#include "x264shared/include/x264.h"
#include "x264shared/include/x264_config.h"
}

// 读取 YUV 图像
int read_yuv_frame(FILE *fp, x264_picture_t *pic, int width, int height) {
	int y_size = width * height;  // 计算Y分量大小
	int uv_size = y_size / 4;     // 计算U和V分量大小，YUV 4:2:0格式下为Y的四分之一

	// 从文件中读取Y分量数据
	if (fread(pic->img.plane[0], 1, y_size, fp) != y_size) {
		return -1; // 如果读取失败，返回-1
	}
	// 从文件中读取U分量数据
	if (fread(pic->img.plane[1], 1, uv_size, fp) != uv_size) {
		return -1; // 如果读取失败，返回-1
	}
	// 从文件中读取V分量数据
	if (fread(pic->img.plane[2], 1, uv_size, fp) != uv_size) {
		return -1; // 如果读取失败，返回-1
	}

	return 0; // 成功读取返回0
}

int main(int argc, char** argv)
{
	printf("hello world.\n"); // 打印测试信息

	// 打开输入YUV文件
	FILE* in_file = fopen("E:\\hs\\hspro\\x264demo\\bin\\yuv_420-352x288.yuv", "rb");
	// 打开输出H.264文件
	FILE* out_file = fopen("E:\\hs\\hspro\\x264demo\\bin\\x.h264", "wb");

	if (!in_file) {
		fprintf(stderr, "Failed to open input file\n"); // 如果输入文件打开失败，打印错误信息
		return -1;
	}

	if (!out_file) {
		fprintf(stderr, "Failed to open output file\n"); // 如果输出文件打开失败，打印错误信息
		fclose(in_file); // 关闭已打开的输入文件
		return -1;
	}

	x264_t *encoder;              // x264编码器
	x264_picture_t pic_in, pic_out; // 输入和输出图像
	x264_param_t param;           // 编码参数
	x264_nal_t *nals;             // NAL单元
	int num_nals;                 // NAL单元数量

	// 初始化编码参数，使用“medium”预设和“zerolatency”选项
	x264_param_default_preset(&param, "medium", "zerolatency");
	param.i_bitdepth = 8;         // 位深度设置为8位
	param.i_csp = X264_CSP_I420;  // 色彩空间设置为I420
	param.i_width = 352;          // 视频宽度设置为352
	param.i_height = 288;         // 视频高度设置为288
	param.i_fps_num = 25;         // 帧率分子设置为25
	param.i_fps_den = 1;          // 帧率分母设置为1

	// 应用高质量的编码配置文件
	x264_param_apply_profile(&param, "high");

	// 打开x264编码器，使用设置好的参数
	encoder = x264_encoder_open(&param);
	if (!encoder) {
		fprintf(stderr, "Failed to open encoder\n"); // 如果编码器打开失败，打印错误信息
		fclose(in_file); // 关闭已打开的输入文件
		fclose(out_file); // 关闭已打开的输出文件
		return -1;
	}

	// 分配输入图像内存
	x264_picture_alloc(&pic_in, param.i_csp, param.i_width, param.i_height);
	// 初始化输出图像
	x264_picture_init(&pic_out);

	int frame_count = 0; // 初始化帧计数器
	// 从文件中读取一帧YUV图像并进行编码
	while (read_yuv_frame(in_file, &pic_in, param.i_width, param.i_height) == 0) {
		pic_in.i_pts = frame_count++; // 设置图像的时间戳（PTS）

		// 使用x264编码器编码图像，生成NAL单元
		int frame_size = x264_encoder_encode(encoder, &nals, &num_nals, &pic_in, &pic_out);
		if (frame_size < 0) {
			fprintf(stderr, "Failed to encode frame\n"); // 如果编码失败，打印错误信息
			x264_picture_clean(&pic_in); // 清理输入图像内存
			x264_encoder_close(encoder); // 关闭编码器
			fclose(in_file); // 关闭输入文件
			fclose(out_file); // 关闭输出文件
			return -1;
		}

		// 写入编码后的NAL单元到输出文件
		for (int i = 0; i < num_nals; i++) {
			fwrite(nals[i].p_payload, 1, nals[i].i_payload, out_file);
		}
	}

	// 清理输入图像内存
	x264_picture_clean(&pic_in);
	// 关闭编码器
	x264_encoder_close(encoder);
	// 关闭输入文件
	fclose(in_file);
	// 关闭输出文件
	fclose(out_file);

	return 0; // 程序成功结束
}
