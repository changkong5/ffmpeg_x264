#include <stdio.h>
#include <malloc.h>
#include <stdint.h>
#include <inttypes.h>
#include <memory.h>

#include "x264.h"
#include "x264_config.h"

#define	BUF_SIZE	32 * 1024 * 1024

#define ENCODE_STREAM_SAVE	1
#define RECON_YUV_SAVE 1


void plane_copy_deinterleave(uint8_t* dsta, intptr_t i_dsta, uint8_t* dstb, intptr_t i_dstb,
	uint8_t* src, intptr_t i_src, int w, int h)
{
	for (int y = 0; y < h; y++, dsta += i_dsta, dstb += i_dstb, src += i_src)
		for (int x = 0; x < w; x++)
		{
			dsta[x] = src[2 * x];
			dstb[x] = src[2 * x + 1];
		}

}

int main(int argc, int** argv)
{
	FILE* fp_in_yuv;
	FILE* fp_out_h264;
	FILE* fp_out_recon;
	int width, height;
	int frames_to_encode;
	x264_param_t param;
	x264_picture_t pic_in;
	x264_picture_t pic_out;
	x264_t* h;
	int i_frame = 0;
	int i_frame_size;
	x264_nal_t* nals;
	x264_nal_t* nal;
	int i_nal;

	// set default param
	x264_param_default(&param);

	/* Get default params for preset/tuning */
	if (x264_param_default_preset(&param, "ultrafast", "zerolatency") < 0)
	{
		printf("set preset failed\n");
		return -1;
	}

	width = 1280;
	height = 720;

	/* Configure non-default params */
	param.i_bitdepth = 8;
	param.i_csp = X264_CSP_I420;
	param.i_width = width;
	param.i_height = height;
	param.b_vfr_input = 0;
	param.b_repeat_headers = 1;
	param.b_annexb = 1;
	param.b_full_recon = 1;

	/* Apply profile restrictions. */
	if (x264_param_apply_profile(&param, "high") < 0)
	{
		printf("apply profile failed\n");
		return -1;
	}

	h = x264_encoder_open(&param);
	if (!h)
	{
		printf("open encoder failed\n");
		return -1;
	}

	if (x264_picture_alloc(&pic_in, param.i_csp, param.i_width, param.i_height) < 0)
	{
		printf("alloc picture failed\n");
		return -1;
	}

	uint8_t* data_buf = (uint8_t*)malloc(BUF_SIZE * sizeof(uint8_t));
	if (!data_buf)
	{
		printf("malloc data buf failed\n");
		return -1;
	}

	fp_in_yuv = fopen("crew.yuv", "rb");
	if (!fp_in_yuv)
	{
		printf("open file failed\n");
		return -1;
	}

#if ENCODE_STREAM_SAVE
	fp_out_h264 = fopen("crew.h264", "wb");
	if (!fp_out_h264)
	{
		printf("open output file failed\b");
		return -1;
	}
#endif
#if RECON_YUV_SAVE
	fp_out_recon = fopen("crew_recon.yuv", "wb");
	if (!fp_out_recon)
	{
		printf("open recon file failed\n");
		return -1;
	}
#endif
	int rsz = 0; // read size
	int size = width * height;
	uint8_t* p = data_buf;
	int frame_cnt = 0;
	/* Encode frames */
	for (;; i_frame++)
	{
		rsz = fread(p, 1, width * height * 3 / 2, fp_in_yuv);
		printf("read size:%d\n", rsz);
		if (!rsz)
		{
			printf("end of file\n");
			goto end;
		}

		// read input data
		memcpy(pic_in.img.plane[0], p, size);
		memcpy(pic_in.img.plane[1], p + size, size / 4);
		memcpy(pic_in.img.plane[2], p + size + size / 4, size / 4);
		pic_in.i_pts = i_frame;

		i_frame_size = x264_encoder_encode(h, &nals, &i_nal, &pic_in, &pic_out);
		if (i_frame_size < 0)
		{
			printf("x264 encode failed\n");
			goto end;
		}
		else if (i_frame_size)
		{
#if ENCODE_STREAM_SAVE
			for (nal = nals; nal < nals + i_nal; nal++) 
			{
				fwrite(nal->p_payload, sizeof(uint8_t), nal->i_payload, fp_out_h264);
			}
#endif
			
#if RECON_YUV_SAVE
			// write recon yuv
			for (int y = 0; y < height; y++)
			{
				fwrite(&pic_out.img.plane[0][y * pic_out.img.i_stride[0]], sizeof(uint8_t), width, fp_out_recon);
			}

			/*
			img.plane[1]中存储uv分量的方式为交错存储，即UV UV UV....的方式
					0		1		2		3...	w_uv - 1
			0		uv		uv		uv		uv		uv
			1		uv		uv		uv		uv		uv
			2		uv		uv		uv		uv		uv
			3		uv		uv		uv		uv		uv
			...
			h_uv - 1	uv	uv		uv		uv		uv
			*/
			int cw = width >> 1;
			int ch = height >> 1;
			uint8_t* planeu = (uint8_t*)malloc(2 * (cw * ch * sizeof(uint8_t) + 32));
			if (planeu)
			{
				uint8_t* planev = planeu + cw * ch + 32 / sizeof(uint8_t);
				plane_copy_deinterleave(planeu, cw, planev, cw, pic_out.img.plane[1], pic_out.img.i_stride[1], cw, ch);
				fwrite(planeu, 1, cw * ch * sizeof(uint8_t), fp_out_recon);
				fwrite(planev, 1, cw * ch * sizeof(uint8_t), fp_out_recon);
				free(planeu);
			}
#endif
		}
		printf("frame_cnt:%d\n", frame_cnt++);
	}

end:
	x264_encoder_close(h);
	x264_picture_clean(&pic_in);
	
	fclose(fp_in_yuv);
#if ENCODE_STREAM_SAVE
	fclose(fp_out_h264);
#endif
#if RECON_YUV_SAVE
	fclose(fp_out_recon);
#endif
	free(data_buf);
	return 0;
}


