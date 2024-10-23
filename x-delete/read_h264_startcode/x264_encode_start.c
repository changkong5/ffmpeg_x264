#include <stdint.h>
#include <x264.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "x264.h"
#include "x264_config.h"


struct h264_nalu_header {
	uint8_t id[4];
	union {
		struct {
			uint8_t forbidden     : 1;
			uint8_t nal_ref_idc   : 2;
			uint8_t nal_unit_type : 5;
		};
		uint8_t header;
	};
};


struct encode_i {

	char *start;
	char *next;
	char *end;
	char *from;
	char *to;
};

struct encode_i encode_i[1];

/*
#include <stdio.h>
FILE *fopen(const char *path, const char *mode);
int fclose(FILE *fp);

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(void *ptr, size_t size, size_t nmemb, FILE *stream);
long ftell(FILE *stream);

int fseek(FILE *stream, long offset, int whence);
whence：有三个选项：SEEK_SET（文件首部）/SEET_CUR（当前位置）/SEEK_END
*/


void split_nalu(const uint8_t *data, size_t size)
{
    const uint8_t *p = data;
    const uint8_t *end = data + size;

    p = data + 3;

    while (p < end) {
        if (p[-1] > 1) {
        	p += 3;
        }
        else if (p[-2]) {
        	p += 2;
        } else if (p[-3]|(p[-1]-1)) {
        	p++;
        }  else {
        	printf("hdr_index = %ld, index = %ld\n", p - data, p - 3 - 1 - data);
            // nalus.emplace_back(p - 3 - data);
            p++;
        }
    }
    return;
}

int main(int argc, char *argv[])
{
	int ret = 0, total;
	char buf[4096];
	int rbyte;
	struct stat stat_i[1];
	char *file = "encode_w352xh288.h264";

	FILE *fp = fopen(file, "rb");
	
	ret = stat(file, stat_i);
	total = stat_i->st_size;
	printf("ret = %d, stat_i->st_size = %ld\n", ret, stat_i->st_size);
	
	
	while (total > 0) {
	
		rbyte = fread(buf, 1, sizeof(buf), fp);
		
		// split_nalu(buf, rbyte);
		
		encode_i->from = buf;
		encode_i->to   = buf + sizeof(buf);
		
		encode_i->start = buf;
		encode_i->end = buf + sizeof(buf);
		
		while ((encode_i->start + 3) < encode_i->end) {
		
			if (encode_i->start[0] == 0 && encode_i->start[1] == 0 && encode_i->start[2] == 0 && encode_i->start[3] == 1) {
				
			}
		}
	
		total -= rbyte;
		printf("total = %d, rbyte = %d\n", total, rbyte);
	}
	
	

	fclose(fp);

	return 0;
}


/*
	fseek(fp, 0L, SEEK_END);
	fileSize = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
*/

#if 0
#include <stdio.h>
#include <stdlib.h>
 
//读取一帧H264数据
int read_one_frame(FILE *fp, unsigned char *ptr){
	int size=0;
	static unsigned char ts[4]={0};
	printf("read one frame\n");
    //防止文件数据错误
	if(fread(ptr,1,4,fp)<4){
        printf("read start error\n");
        return size;
    }
 
	if((*ptr==0x00) && (*(ptr+1)==0x00) && (*(ptr+2)==0x00) && (*(ptr+3)==0x01)){
		 
		size=4;
		while(1){
			if(fread(ptr+size,1,1,fp)){
			
				ts[0]=ts[1];
				ts[1]=ts[2];
				ts[2]=ts[3];
				ts[3]=*(ptr+size);
				size++;
				if((ts[0]==0x00) && (ts[1]==0x00) && (ts[2]==0x00) && (ts[3]==0x01) ){
					//读取到下一帧的起始码，即表示这帧读完了，后移文件指针
                    size-=4;
					fseek(fp,-4,SEEK_CUR);
					//printf("read one frame end \n");
					break;
				}
			}else
				break;//读完文件退出循环
		}
	}
    //读取到的数据大小
	return size;
}
 
int main(void)
{
	int readbytes;
	int totalReadSize=0;
	int length=1024*1024*10;
	unsigned char *ptr;
	ptr=malloc(length);
	FILE *fp=fopen("encode_w352xh288.h264", "rb");
	int fileSize;
	fseek(fp, 0L, SEEK_END);
	fileSize = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	printf("fileSize=0x%x\n", fileSize);
	
	while(totalReadSize<fileSize){
		readbytes=read_one_frame(fp, ptr);
		if(readbytes==0){
			printf("read end\n");
			break;
		}else{
			for(int i=0;i<readbytes;i++)
				printf("%x ", *(ptr+i));
		}
		totalReadSize+=readbytes;
		printf("readbytes: 0x%X    total read size: 0x%X \r\n",readbytes, totalReadSize);
	}
 
	return 0;
	
}

#endif







/*
void split_nalu(const uint8_t *data, size_t size, vector<int>& nalus)
{
    const uint8_t *p = data;
    const uint8_t *end = data + size;

    p = data + 3;

    while (p < end) {
        if      (p[-1] > 1      ) p += 3;
        else if (p[-2]          ) p += 2;
        else if (p[-3]|(p[-1]-1)) p++;
        else {
            nalus.emplace_back(p - 3 - data);
            p++;
        }
    }
    return;
}

*/


















#if 0
 
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
		x264_nal_t *nals = encoder->nal;
		
		for (int i = 0; 0 && i < n_nal; i++) {
			printf("%s: i = %d, nals[i].i_ref_idc = %d, nals[i].i_type = %d, nals[i].b_long_startcode = %d, nals[i].i_first_mb = %d, nals[i].i_last_mb = %d, nals[i].i_payload = %d\n", 
					__func__, i, nals[i].i_ref_idc, nals[i].i_type, nals[i].b_long_startcode, nals[i].i_first_mb, nals[i].i_last_mb, nals[i].i_payload);
					
			for (int k = 0; k < nals[i].i_payload; k++) {
				if (k > 0 && (k % 16) == 0) {
					//printf("\n");
				}
				//printf("0x%02x ", nals[i].p_payload[k]);
			}
			//printf("\n");
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
#endif


