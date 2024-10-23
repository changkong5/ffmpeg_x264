#include <stdio.h>
#include <string.h>
#include <stdint.h>


#include "x264.h"
#include "x264_config.h"

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

#if 0
typedef struct x264_nal_t
{
    int i_ref_idc;  /* nal_priority_e */
    int i_type;     /* nal_unit_type_e */
    int b_long_startcode;
    int i_first_mb; /* If this NAL is a slice, the index of the first MB in the slice. */
    int i_last_mb;  /* If this NAL is a slice, the index of the last MB in the slice. */

    /* Size of payload (including any padding) in bytes. */
    int     i_payload;
    /* If param->b_annexb is set, Annex-B bytestream with startcode.
     * Otherwise, startcode is replaced with a 4-byte size.
     * This size is the size used in mp4/similar muxing; it is equal to i_payload-4 */
    uint8_t *p_payload;

    /* Size of padding in bytes. */
    int i_padding;
} x264_nal_t;
#endif

/*
#查看图片，需要注意的是YUV图像的信息中并没有存储宽和高，所以在打开时需要指定图像的宽和高。
$ffplay [-f rawvideo] -video_size WxH dest.yuv
 
#将图像转化为YUV格式
$ffmpeg -i src.xxx -s WxH [-pix_fmt yuv420p] dest.yuv
*/

// ffplay -f rawvideo -video_size 352x288 yuv_420-352x288.yuv
// ffmpeg -i x.h264 -vcodec libx264 -hex   // error

int main(int argc, char** argv)
{
	printf("hello world.\n"); // 打印测试信息

	// 打开输入YUV文件
	FILE* in_file = fopen("./yuv_420-352x288.yuv", "rb");
	// 打开输出H.264文件
	FILE* out_file = fopen("./x.h264", "wb+");

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
		printf("%s: frame_count = %d, param.i_width = %d, param.i_height = %d, total = %d\n", __func__, frame_count, param.i_width, param.i_height, (param.i_width*param.i_height*3/2));

		// 使用x264编码器编码图像，生成NAL单元
		int frame_size = x264_encoder_encode(encoder, &nals, &num_nals, &pic_in, &pic_out);
		printf("%s: frame_size = %d, num_nals = %d\n", __func__, frame_size, num_nals);
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
			// printf("%s: i = %d, num_nals = %d, nals[i].i_payload = %d， nals[i].i_type = %d\n", __func__, i, num_nals, nals[i].i_payload, nals[i].i_type);
			fwrite(nals[i].p_payload, 1, nals[i].i_payload, out_file);
		}
		
		
		for (int i = 0; i < num_nals; i++) {
			printf("%s: i = %d, nals[i].i_ref_idc = %d, nals[i].i_type = %d, nals[i].b_long_startcode = %d, nals[i].i_first_mb = %d, nals[i].i_last_mb = %d, nals[i].i_payload = %d\n", 
					__func__, i, nals[i].i_ref_idc, nals[i].i_type, nals[i].b_long_startcode, nals[i].i_first_mb, nals[i].i_last_mb, nals[i].i_payload);
					
			for (int k = 0; k < nals[i].i_payload; k++) {
				if (k > 0 && (k % 16) == 0) {
					printf("\n");
				}
				printf("0x%02x ", nals[i].p_payload[k]);
			}
			printf("\n");
		}
		
		if (frame_count == 4) {
			break;
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
