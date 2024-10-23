#include <stdio.h>
#include <malloc.h>
#include <stdint.h>
#include <inttypes.h>
#include <memory.h>

#include "x264.h"
#include "x264_config.h"

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

#define width  640  
#define height 360  

int  main(int argc, char **argv)
{
	int i;
	int j;
	int frame_num = 100;
	int csp = X264_CSP_I420;
	int y_size = width*height;
	int n_nal;
	int ret;
	x264_param_t* param = (x264_param_t*)malloc(sizeof(x264_param_t));
	x264_picture_t* pic_in = (x264_picture_t*)malloc(sizeof(x264_picture_t));
	x264_picture_t* pic_out = (x264_picture_t*)malloc(sizeof(x264_picture_t));
	x264_nal_t* nal = (x264_nal_t *)malloc(sizeof(x264_nal_t));
	x264_t *encoder;

    FILE* read_filename = fopen("test_yuv420p.yuv", "rb");
    FILE* write_filename = fopen("test.h264", "wb");
    if (read_filename == NULL || write_filename == NULL)
    {
		printf("Error open files.\n");
	    return -1;
	}
	
	x264_param_default(param);
	param->i_width = width;    
	param->i_height = height;   
	//param->i_fps_num = 25;  
	//param->i_fps_den = 1;   
	//param->i_frame_total = 0;  
	//param->i_threads = X264_SYNC_LOOKAHEAD_AUTO;  
	//param->i_log_level = X264_LOG_DEBUG;      
	//param->i_bframe = 5;
	//param->b_open_gop = 0;
	//param->i_bframe_adaptive = X264_B_ADAPT_TRELLIS;
	//param->i_bframe_pyramid = 0;        
	//param->i_timebase_den = param->i_fps_num;
	//param->i_timebase_num = param->i_fps_den;
	//param->i_keyint_max = 25;
	//param->rc.i_bitrate = 96;   
	//param->rc.i_qp_max = 0;
	//param->rc.i_qp_min = 0;
	//param->b_annexb = true;
	//param->b_intra_refresh = 1;
	x264_param_apply_profile(param, "baseline");   
	param->i_csp = csp;
	x264_picture_init(pic_out);         
	x264_picture_alloc(pic_in, csp, param->i_width, param->i_height);

	encoder = x264_encoder_open(param);

	if (frame_num == 0)
	{
		fseek(read_filename, 0, SEEK_END);
		frame_num = ftell(read_filename) / (width * height * 3 / 2);
		fseek(read_filename, 0, SEEK_SET);
	}
	for (i = 0; i < frame_num; i++)
	{
		fread(pic_in->img.plane[0], y_size, 1, read_filename);
		fread(pic_in->img.plane[1], y_size / 4, 1, read_filename);
		fread(pic_in->img.plane[2], y_size / 4, 1, read_filename);

		pic_in->i_pts = i;
		ret = x264_encoder_encode(encoder, &nal, &n_nal, pic_in, pic_out);
		if (ret < 0)
		{
			printf("Error.\n");
			return -1;
		}
		printf("Succeed encode frame: %5d\n", i);
		for (j = 0; j < n_nal; ++j)
		{
			fwrite(nal[j].p_payload, 1, nal[j].i_payload, write_filename);
		}
	}
	i = 0;
	while (1)
	{
		ret = x264_encoder_encode(encoder, &nal, &n_nal, NULL, pic_out);
		if (ret == 0)
		{
			break;
		}
		printf("Flush 1 frame.\n");
		for (j = 0; j < n_nal; ++j)
		{
			fwrite(nal[j].p_payload, 1, nal[j].i_payload, write_filename);
		}
		i++;
	}

	x264_picture_clean(pic_in);
	x264_encoder_close(encoder);
	encoder = NULL;
	free(pic_in);
	free(pic_out);
	free(param);
	fclose(read_filename);
	fclose(write_filename);
	return 0;
}


