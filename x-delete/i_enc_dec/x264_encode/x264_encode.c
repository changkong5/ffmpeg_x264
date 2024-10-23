#include <stdint.h>
#include <x264.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include "x264.h"
#include "x264_config.h"
 
#define DEBUG 0
 
#define CLEAR(x) (memset((&x),0,sizeof(x)))
#define IMAGE_WIDTH  352	// 320
#define IMAGE_HEIGHT 288	// 240
#define ENCODER_PRESET "veryfast"
#define ENCODER_TUNE   "zerolatency"
#define ENCODER_PROFILE  "baseline"
#define ENCODER_COLORSPACE X264_CSP_I420
 
typedef struct my_x264_encoder{
	x264_param_t  * x264_parameter;
	char parameter_preset[20];
	char parameter_tune[20];
	char parameter_profile[20];
	x264_t  * x264_encoder;
	x264_picture_t * yuv420p_picture;
	long colorspace;
	unsigned char *yuv;
	x264_nal_t * nal;
} my_x264_encoder;
 
char *read_filename="yuv420p_w352xh288.yuv";
char *write_filename="encode_w352xh288.h264";
 
int
main(int argc ,char **argv){
	int ret;
	int fd_read,fd_write;
	my_x264_encoder * encoder=(my_x264_encoder *)malloc(sizeof(my_x264_encoder));
	if(!encoder){
		printf("cannot malloc my_x264_encoder !\n");
		exit(EXIT_FAILURE);
	}
	CLEAR(*encoder);
 
 
	/****************************************************************************
	 * Advanced parameter handling functions
	 ****************************************************************************/
 
	/* These functions expose the full power of x264's preset-tune-profile system for
	 * easy adjustment of large numbers //free(encoder->yuv420p_picture);of internal parameters.
	 *
	 * In order to replicate x264CLI's option handling, these functions MUST be called
	 * in the following order:
	 * 1) x264_param_default_preset
	 * 2) Custom user options (via param_parse or directly assigned variables)
	 * 3) x264_param_apply_fastfirstpass
	 * 4) x264_param_apply_profile
	 *
	 * Additionally, x264CLI does not apply step 3 if the preset chosen is "placebo"
	 * or --slow-firstpass is set. */
	strcpy(encoder->parameter_preset,ENCODER_PRESET);
	strcpy(encoder->parameter_tune,ENCODER_TUNE);
 
	encoder->x264_parameter=(x264_param_t *)malloc(sizeof(x264_param_t));
	if(!encoder->x264_parameter){
		printf("malloc x264_parameter error!\n");
		exit(EXIT_FAILURE);
	}
	CLEAR(*(encoder->x264_parameter));
	x264_param_default(encoder->x264_parameter);
 
	if((ret=x264_param_default_preset(encoder->x264_parameter,encoder->parameter_preset,encoder->parameter_tune))<0){
		printf("x264_param_default_preset error!\n");
		exit(EXIT_FAILURE);
	}
 
	encoder->x264_parameter->i_fps_den 		 =1;
	encoder->x264_parameter->i_fps_num 		 =25;
	encoder->x264_parameter->i_width 		 =IMAGE_WIDTH;
	encoder->x264_parameter->i_height		 =IMAGE_HEIGHT;
	encoder->x264_parameter->i_threads		 =1;
	encoder->x264_parameter->i_keyint_max    =25;
	encoder->x264_parameter->b_intra_refresh =1;
	encoder->x264_parameter->b_annexb		 =1;
 
	strcpy(encoder->parameter_profile,ENCODER_PROFILE);
	if((ret=x264_param_apply_profile(encoder->x264_parameter,encoder->parameter_profile))<0){
		printf("x264_param_apply_profile error!\n");
		exit(EXIT_FAILURE);
	}
 
#if DEBUG
	printf("Line --------%d\n",__LINE__);
#endif
 
	encoder->x264_encoder=x264_encoder_open(encoder->x264_parameter);
 
	encoder->colorspace=ENCODER_COLORSPACE;
 
#if DEBUG
	printf("Line --------%d\n",__LINE__);
#endif
 
	encoder->yuv420p_picture=(x264_picture_t *)malloc(sizeof(x264_picture_t ));
	if(!encoder->yuv420p_picture){
		printf("malloc encoder->yuv420p_picture error!\n");
		exit(EXIT_FAILURE);
	}
	if((ret=x264_picture_alloc(encoder->yuv420p_picture,encoder->colorspace,IMAGE_WIDTH,IMAGE_HEIGHT))<0){
		printf("ret=%d\n",ret);
		printf("x264_picture_alloc error!\n");
		exit(EXIT_FAILURE);
	}
 
	encoder->yuv420p_picture->img.i_csp=encoder->colorspace;
	encoder->yuv420p_picture->img.i_plane=3;
	encoder->yuv420p_picture->i_type=X264_TYPE_AUTO;
 
 
 
#if DEBUG
	printf("Line --------%d\n",__LINE__);
#endif
 
	encoder->yuv=(uint8_t *)malloc(IMAGE_WIDTH*IMAGE_HEIGHT*3/2);
	if(!encoder->yuv){
		printf("malloc yuv error!\n");
		exit(EXIT_FAILURE);
	}
	CLEAR(*(encoder->yuv));
 
#if DEBUG
	printf("Line --------%d\n",__LINE__);
#endif
 
	encoder->yuv420p_picture->img.plane[0]=encoder->yuv;
	encoder->yuv420p_picture->img.plane[1]=encoder->yuv+IMAGE_WIDTH*IMAGE_HEIGHT;
	encoder->yuv420p_picture->img.plane[2]=encoder->yuv+IMAGE_WIDTH*IMAGE_HEIGHT+IMAGE_WIDTH*IMAGE_HEIGHT/4;
 
	if((fd_read=open(read_filename,O_RDONLY))<0){
		printf("cannot open input file!\n");
		exit(EXIT_FAILURE);
	}
 
	if((fd_write=open(write_filename,O_WRONLY | O_APPEND | O_CREAT,0777))<0){
		printf("cannot open output file!\n");
		exit(EXIT_FAILURE);
	}
 
#if DEBUG
	printf("Line --------%d\n",__LINE__);
#endif
	int n_nal;
	x264_picture_t pic_out;
	x264_nal_t *my_nal;
	encoder->nal=(x264_nal_t *)malloc(sizeof(x264_nal_t ));
	if(!encoder->nal){
		printf("malloc x264_nal_t error!\n");
		exit(EXIT_FAILURE);
	}
	CLEAR(*(encoder->nal));
 
	while(read(fd_read,encoder->yuv,IMAGE_WIDTH*IMAGE_HEIGHT*3/2)>0){
		encoder->yuv420p_picture->i_pts++;
		if((ret=x264_encoder_encode(encoder->x264_encoder,&encoder->nal,&n_nal,encoder->yuv420p_picture,&pic_out))<0){
			printf("x264_encoder_encode error!\n");
			exit(EXIT_FAILURE);
		}
 
		unsigned int length=0;
		for(my_nal=encoder->nal;my_nal<encoder->nal+n_nal;++my_nal){
			write(fd_write,my_nal->p_payload,my_nal->i_payload);
			length+=my_nal->i_payload;
		}
		printf("length=%d\n",length);
	}
 
	/*clean_up functions*/
	//x264_picture_clean(encoder->yuv420p_picture);
	//free(encoder->nal);//???? confused conflict with x264_encoder_close(encoder->x264_encoder);
 
	free(encoder->yuv);
	free(encoder->yuv420p_picture);
	free(encoder->x264_parameter);
	x264_encoder_close(encoder->x264_encoder);
	free(encoder);
	close(fd_read);
	close(fd_write);
 
	return 0;
}
