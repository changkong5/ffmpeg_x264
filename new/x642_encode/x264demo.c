#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "x264.h"
#include "x264_config.h"

// ffplay -f rawvideo -s 352x288 yuv420p_w352xh288.yuv 																// correct   // play yuv
// ffplay yuv420p_w352xh288.h264 																					// correct   // play h264
// ffmpeg -s 352x288 -i yuv420p_w352xh288.yuv -c:v h264 yuv420p_w352xh288.h264   									// correct	 // yuv ---> h264
// ffmpeg -c:v h264 -i yuv420p_w352xh288.h264 yuv420p_w352xh288.yuv    												// correct   // h264 ---> yuv

int read_yuv_frame(FILE *fp, x264_picture_t *pic, int width, int height) 
{
	int y_size = width * height;
	int uv_size = y_size / 4;
	
	if (fread(pic->img.plane[0], 1, y_size, fp) != y_size) {
		return EXIT_FAILURE;
	}
	
	if (fread(pic->img.plane[1], 1, uv_size, fp) != uv_size) {
		return EXIT_FAILURE;
	}
	
	if (fread(pic->img.plane[2], 1, uv_size, fp) != uv_size) {
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
	FILE* in_file = fopen("yuv420p_w352xh288.yuv", "rb");
	FILE* out_file = fopen("yuv420p_w352xh288.h264", "wb+");

	if (!in_file) {
		fprintf(stderr, "Failed to open input file\n"); 
		return EXIT_FAILURE;
	}

	if (!out_file) {
		fprintf(stderr, "Failed to open output file\n"); 
		fclose(in_file);
		return EXIT_FAILURE;
	}

	x264_t *encoder;            
	x264_picture_t pic_in, pic_out; 
	x264_param_t param;       
	x264_nal_t *nals;       
	int num_nals;             


	x264_param_default_preset(&param, "medium", "zerolatency");
	param.i_bitdepth = 8;     
	param.i_csp = X264_CSP_I420;  
	param.i_width = 352;   
	param.i_height = 288;    
	param.i_fps_num = 25;    
	param.i_fps_den = 1;      
	x264_param_apply_profile(&param, "high");

	encoder = x264_encoder_open(&param);
	
	if (!encoder) {
		fprintf(stderr, "Failed to open encoder\n"); 
		fclose(in_file);
		fclose(out_file);
		return EXIT_FAILURE;
	}
	
	x264_picture_alloc(&pic_in, param.i_csp, param.i_width, param.i_height);
	x264_picture_init(&pic_out);

	int frame_count = 0; 
	
	uint8_t *packet;
	int packet_len;
	int zero;

	while (read_yuv_frame(in_file, &pic_in, param.i_width, param.i_height) == 0) {
		pic_in.i_pts = frame_count++; 
		printf("%s: frame_count = %d, param.i_width = %d, param.i_height = %d, total = %d\n", __func__, frame_count, param.i_width, param.i_height, (param.i_width*param.i_height*3/2));


		int frame_size = x264_encoder_encode(encoder, &nals, &num_nals, &pic_in, &pic_out);
		
		printf("%s: frame_size = %d, num_nals = %d\n", __func__, frame_size, num_nals);
		if (frame_size < 0) {
			fprintf(stderr, "Failed to encode frame\n"); 
			x264_picture_clean(&pic_in); 
			x264_encoder_close(encoder); 
			fclose(in_file); 
			fclose(out_file);
			return EXIT_FAILURE;
		}

		for (int i = 0; i < num_nals; i++) {
		// printf("%s: i = %d, num_nals = %d, nals[i].i_payload = %d， nals[i].i_type = %d\n", __func__, i, num_nals, nals[i].i_payload, nals[i].i_type);
			printf("%s: i = %d, num_nals = %d, nals[i].i_payload = %d, nals[i].i_type = %d (0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x)\n", __func__, 
			i, num_nals, nals[i].i_payload, nals[i].i_type, nals[i].p_payload[0], nals[i].p_payload[1], nals[i].p_payload[2], nals[i].p_payload[3], nals[i].p_payload[4], nals[i].p_payload[5]);
			
			for (zero = 0; zero < 4; zero++) {
				if (nals[i].p_payload[zero] != 0) {
					break;
				}
			}
			printf("%s: zero = %d\n", __func__, zero);
			
			if (zero == 2) {
				// nalu start code [0x00 0x00 0x01]
				packet_len = nals[i].i_payload;
				packet = &nals[i].p_payload[0];
			} else {
				// nalu start code [0x00 0x00 0x00 0x01]
				packet_len = nals[i].i_payload - 1;
				packet = &nals[i].p_payload[1];
			}
			
			fwrite(packet, 1, packet_len, out_file);
			// fwrite(nals[i].p_payload, 1, nals[i].i_payload, out_file);
		}
	}


	x264_picture_clean(&pic_in);
	x264_encoder_close(encoder);

	fclose(in_file);
	fclose(out_file);

	return EXIT_SUCCESS;
}




