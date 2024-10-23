#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <linux/fb.h>
#include <time.h>
#include <linux/rtc.h>
#include <sys/time.h>

#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include <stdint.h>

/*
#include "lvgl/lvgl.h"

#include "initcall.h"
#include "gdbus/gdbus.h"

extern lv_color32_t img_star_map_xx[];

void vedio_canvas_update_iii(void);
*/

#define pr_err   printf
#define pr_info   printf

struct yuyv_422 {
    uint8_t y1;
    uint8_t u;
    uint8_t y2;
    uint8_t v;
};

struct rgb_888 {
    uint32_t rgb1;
    uint32_t rgb2;
};


#define FRAMEBUFFER_COUNT 3 // 帧缓冲数量

/*** 摄像头像素格式及其描述信息 ***/
typedef struct camera_format {
    unsigned char description[32]; 		// 字符串描述信息
    unsigned int pixelformat;      		// 像素格式
} cam_fmt;
 
/*** 描述一个帧缓冲的信息 ***/
typedef struct cam_buf_info {
    unsigned short *start; 				// 帧缓冲起始地址
    unsigned long length;  				// 帧缓冲长度
} cam_buf_info;

unsigned char  timeBUuffer[1920][1080 * 4];

static int width;                          // LCD宽度
static int height;                         // LCD高度
static int fb_fd = -1;                     // LCD设备文件描述符
static int v4l2_fd = -1;                   // 摄像头设备文件描述符
static cam_buf_info buf_infos[FRAMEBUFFER_COUNT];
static cam_fmt cam_fmts[10];
static int frm_width, frm_height; // 视频帧宽度和高度


static int v4l2_dev_init(const char *device)
{
    struct v4l2_capability cap = {0};
 
    v4l2_fd = open(device, O_RDWR);

    if (v4l2_fd < 0) {
        pr_err("open error: %s: %s\n", device, strerror(errno));
        return -1;
    }
 
    ioctl(v4l2_fd, VIDIOC_QUERYCAP, &cap);

    pr_info("%s: cap.driver       = %s\n", __func__, cap.driver);
    pr_info("%s: cap.card         = %s\n", __func__, cap.card);
    pr_info("%s: cap.bus_info     = %s\n", __func__, cap.bus_info);
    pr_info("%s: cap.version      = 0x%08x\n", __func__, cap.version);
    pr_info("%s: cap.capabilities = 0x%08x, V4L2_CAP_VIDEO_CAPTURE = 0x%08x\n", __func__, cap.capabilities, V4L2_CAP_VIDEO_CAPTURE);
 
    if (!(V4L2_CAP_VIDEO_CAPTURE & cap.capabilities)) {
        pr_err("Error: %s: No capture video device!\n", device);
        close(v4l2_fd);
        return -1;
    }
 
    return 0;
}

static void v4l2_enum_formats(void)
{
    int ret = 0;
    struct v4l2_fmtdesc fmtdesc = {0};
    memset(&fmtdesc, 0, sizeof(fmtdesc));

    printf("========1=========\n");
 
    fmtdesc.index = 0;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    while (ioctl(v4l2_fd, VIDIOC_ENUM_FMT, &fmtdesc) == 0) {

        pr_info("%s: fmtdesc.index       = %d\n", __func__, fmtdesc.index);
        pr_info("%s: fmtdesc.type        = %d, V4L2_BUF_TYPE_VIDEO_CAPTURE = %d\n", __func__, fmtdesc.type, V4L2_BUF_TYPE_VIDEO_CAPTURE);
        pr_info("%s: fmtdesc.flags       = %d (0x%08x)\n", __func__, fmtdesc.flags, fmtdesc.flags);
        pr_info("%s: fmtdesc.description = %s\n", __func__, fmtdesc.description);
        pr_info("%s: fmtdesc.pixelformat = %d (0x%08x)\n", __func__, fmtdesc.pixelformat, fmtdesc.pixelformat);

        cam_fmts[fmtdesc.index].pixelformat = fmtdesc.pixelformat;
        strcpy(cam_fmts[fmtdesc.index].description, fmtdesc.description);

        fmtdesc.index++;
    }
}

#if 0
struct v4l2_frmsizeenum {
	__u32			index;		/* Frame size number */
	__u32			pixel_format;	/* Pixel format */
	__u32			type;		/* Frame size type the device supports. */

	union {					/* Frame size */
		struct v4l2_frmsize_discrete	discrete;
		struct v4l2_frmsize_stepwise	stepwise;
	};

	__u32   reserved[2];			/* Reserved space for future use */
};

struct v4l2_frmsize_discrete {
	__u32			width;		/* Frame width [pixel] */
	__u32			height;		/* Frame height [pixel] */
};

struct v4l2_frmivalenum {
	__u32			index;		/* Frame format index */
	__u32			pixel_format;	/* Pixel format */
	__u32			width;		/* Frame width */
	__u32			height;		/* Frame height */
	__u32			type;		/* Frame interval type the device supports. */

	union {					/* Frame interval */
		struct v4l2_fract		discrete;
		struct v4l2_frmival_stepwise	stepwise;
	};

	__u32	reserved[2];			/* Reserved space for future use */
};
#endif


static void v4l2_print_formats(void)
{
    struct v4l2_frmsizeenum frmsize = {0};
    struct v4l2_frmivalenum frmival = {0};

    for (int i = 0; cam_fmts[i].pixelformat; i++) {
 
        pr_info("format <0x%x>, description <%s>\n", cam_fmts[i].pixelformat, cam_fmts[i].description);
 
        frmsize.index = 0;
        frmsize.pixel_format = cam_fmts[i].pixelformat;
        frmsize.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        while (ioctl(v4l2_fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) == 0) {

        	pr_info("%s: frmsize.index       	 = %d\n", __func__, frmsize.index);
        	pr_info("%s: frmsize.pixel_format 	 = %d (0x%08x)\n", __func__, frmsize.pixel_format, frmsize.pixel_format);
        	pr_info("%s: frmsize.type        	 = %d, V4L2_BUF_TYPE_VIDEO_CAPTURE = %d\n", __func__, frmsize.type, V4L2_BUF_TYPE_VIDEO_CAPTURE);
        	pr_info("%s: frmsize.discrete.width  = %d\n", __func__, frmsize.discrete.width);
        	pr_info("%s: frmsize.discrete.height = %d\n", __func__, frmsize.discrete.height);
 
            pr_info("size<%d*%d> ", frmsize.discrete.width, frmsize.discrete.height);
            frmsize.index++;
 
            /* 获取摄像头视频采集帧率 */
            frmival.index = 0;
            frmival.pixel_format = cam_fmts[i].pixelformat;
            frmival.width = frmsize.discrete.width;
            frmival.height = frmsize.discrete.height;
            frmival.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

            while (ioctl(v4l2_fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival) == 0) {
 
                pr_info("<%dfps>", frmival.discrete.denominator / frmival.discrete.numerator);

                frmival.index++;
            }

            pr_info("\n");
        }
        pr_info("\n");
    }
}

#if 0
struct v4l2_format {
	__u32	 type;
	union {
		struct v4l2_pix_format		pix;     /* V4L2_BUF_TYPE_VIDEO_CAPTURE */
		struct v4l2_pix_format_mplane	pix_mp;  /* V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE */
		struct v4l2_window		win;     /* V4L2_BUF_TYPE_VIDEO_OVERLAY */
		struct v4l2_vbi_format		vbi;     /* V4L2_BUF_TYPE_VBI_CAPTURE */
		struct v4l2_sliced_vbi_format	sliced;  /* V4L2_BUF_TYPE_SLICED_VBI_CAPTURE */
		struct v4l2_sdr_format		sdr;     /* V4L2_BUF_TYPE_SDR_CAPTURE */
		struct v4l2_meta_format		meta;    /* V4L2_BUF_TYPE_META_CAPTURE */
		__u8	raw_data[200];                   /* user-defined */
	} fmt;
};

struct v4l2_pix_format {
	__u32			width;
	__u32			height;
	__u32			pixelformat;
	__u32			field;		/* enum v4l2_field */
	__u32			bytesperline;	/* for padding, zero if unused */
	__u32			sizeimage;
	__u32			colorspace;	/* enum v4l2_colorspace */
	__u32			priv;		/* private data, depends on pixelformat */
	__u32			flags;		/* format flags (V4L2_PIX_FMT_FLAG_*) */
	union {
		/* enum v4l2_ycbcr_encoding */
		__u32			ycbcr_enc;
		/* enum v4l2_hsv_encoding */
		__u32			hsv_enc;
	};
	__u32			quantization;	/* enum v4l2_quantization */
	__u32			xfer_func;	/* enum v4l2_xfer_func */
};

/*	Stream type-dependent parameters
 */
struct v4l2_streamparm {
	__u32	 type;			/* enum v4l2_buf_type */
	union {
		struct v4l2_captureparm	capture;
		struct v4l2_outputparm	output;
		__u8	raw_data[200];  /* user-defined */
	} parm;
};

struct v4l2_captureparm {
	__u32		   capability;	  /*  Supported modes */
	__u32		   capturemode;	  /*  Current mode */
	struct v4l2_fract  timeperframe;  /*  Time per frame in seconds */
	__u32		   extendedmode;  /*  Driver-specific extensions */
	__u32              readbuffers;   /*  # of buffers for read */
	__u32		   reserved[4];
};

struct v4l2_fract {
	__u32   numerator;
	__u32   denominator;
};
#endif

// #define V4L2_PIX_FMT_RGB565  v4l2_fourcc('R', 'G', 'B', 'P') /* 16  RGB-5-6-5 
// #define V4L2_PIX_FMT_YUYV    v4l2_fourcc('Y', 'U', 'Y', 'V') /* 16  YUV 4:2:2     */

static int v4l2_set_format(void)
{
    struct v4l2_format fmt = {0};
    struct v4l2_streamparm streamparm = {0};
 
    /* 设置帧格式 */
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; // type类型
    fmt.fmt.pix.width = width;              // 视频帧宽度
    fmt.fmt.pix.height = height;            // 视频帧高度
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;//V4L2_PIX_FMT_RGB565;
    // V4L2_PIX_FMT_RGB565; // 像素格式
    // fmt.fmt.pix.field = V4L2_FIELD_ANY;

    if (ioctl(v4l2_fd, VIDIOC_S_FMT, &fmt) < 0) {
        pr_err("ioctl error: VIDIOC_S_FMT: %s\n", strerror(errno));
        return -1;
    }
 
    /*** 判断是否已经设置为我们要求的RGB565像素格式
    如果没有设置成功表示该设备不支持RGB565像素格式 */
    if (V4L2_PIX_FMT_RGB565 != fmt.fmt.pix.pixelformat) {
        pr_err("Error: the device does not support RGB565 format!\n");
        // return -1;
    }
 
    frm_width = fmt.fmt.pix.width;   // 获取实际的帧宽度
    frm_height = fmt.fmt.pix.height; // 获取实际的帧高度
    pr_info("视频帧大小<%d * %d>\n", frm_width, frm_height);
 
    /* 获取streamparm */
    streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(v4l2_fd, VIDIOC_G_PARM, &streamparm);
 
    /** 判断是否支持帧率设置 **/
    if (V4L2_CAP_TIMEPERFRAME & streamparm.parm.capture.capability) {
        streamparm.parm.capture.timeperframe.numerator = 1;
        streamparm.parm.capture.timeperframe.denominator = 30; // 30fps

        if (ioctl(v4l2_fd, VIDIOC_S_PARM, &streamparm) < 0) {
            pr_err("ioctl error: VIDIOC_S_PARM: %s\n", strerror(errno));
            // return -1;
        }
    }
 
    return 0;
}

static int v4l2_init_buffer(void)
{
    struct v4l2_requestbuffers reqbuf = {0};
    struct v4l2_buffer buf = {0};
 
    /* 申请帧缓冲 */
    reqbuf.count = FRAMEBUFFER_COUNT; // 帧缓冲的数量
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory = V4L2_MEMORY_MMAP;

    if (ioctl(v4l2_fd, VIDIOC_REQBUFS, &reqbuf) < 0) {
        pr_err("ioctl error: VIDIOC_REQBUFS: %s\n", strerror(errno));
        return -1;
    }
 
    /* 建立内存映射 */
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    for (buf.index = 0; buf.index < FRAMEBUFFER_COUNT; buf.index++) {
 
        ioctl(v4l2_fd, VIDIOC_QUERYBUF, &buf);
        buf_infos[buf.index].length = buf.length;
        buf_infos[buf.index].start = mmap(NULL, buf.length,
                                          PROT_READ | PROT_WRITE, MAP_SHARED,
                                          v4l2_fd, buf.m.offset);

        if (buf_infos[buf.index].start == MAP_FAILED) {
            perror("mmap error");
            return -1;
        }
    }
 
    /* 入队 */
    for (buf.index = 0; buf.index < FRAMEBUFFER_COUNT; buf.index++) {
        ioctl(v4l2_fd, VIDIOC_QUERYBUF, &buf);
 
        if (ioctl(v4l2_fd, VIDIOC_QBUF, &buf) < 0) {
            pr_err("ioctl error: VIDIOC_QBUF: %s\n", strerror(errno));
            return -1;
        }
    }
 
    return 0;
}

static int v4l2_stream_on(void)
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
 
    if (ioctl(v4l2_fd, VIDIOC_STREAMON, &type) < 0) {
        pr_err("ioctl error: VIDIOC_STREAMON: %s\n", strerror(errno));
        return -1;
    }
 
    return 0;
}

static int v4l2_stream_off(void)
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
 
    if (ioctl(v4l2_fd, VIDIOC_STREAMOFF, &type) < 0) {
        pr_err("ioctl error: VIDIOC_STREAMOFF: %s\n", strerror(errno));
        return -1;
    }
 
    return 0;
}

int p422_420p_packet_pipe_write(uint8_t *data, int len);

static void v4l2_read_data(void)
{
    struct v4l2_buffer buf = {0};
    unsigned short *base;
    unsigned short *start;
    int min_w, min_h;
    int j,i, count = 0;


    int y1, u1, v1, y2, u2, v2;
    int r1, g1, b1, r2, g2, b2;
    int rgb1, rgb2;
    int rows ,cols;

    struct rgb_888 *rgb_format = (struct rgb_888 *)timeBUuffer;
    struct yuyv_422 *pix_format = (struct yuyv_422 *)NULL;

 
    if (width > frm_width) {
        min_w = frm_width;
    } else {
        min_w = width;
    }

    if (height > frm_height) {
        min_h = frm_height;
    } else {
        min_h = height;
    }
    
    int o_fd = open("yuyv422_w640xh480.yuv", O_RDWR | O_CREAT, 0666);

    // min_w = 640, min_h = 480
    pr_info("min_w = %d, min_h = %d\n", min_w, min_h);
 
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    for ( ; ; ) {
 
        for(buf.index = 0; buf.index < FRAMEBUFFER_COUNT; buf.index++) {
 
            ioctl(v4l2_fd, VIDIOC_DQBUF, &buf);     //出队
            
            p422_420p_packet_pipe_write((void *)buf_infos[buf.index].start, buf_infos[buf.index].length);
            
            printf("%s: buf.index = %d, buf_infos[buf.index].length = %ld\n", __func__, buf.index, buf_infos[buf.index].length);
            
            int wbyte = write(o_fd, (void *)buf_infos[buf.index].start, buf_infos[buf.index].length);
        	fsync(o_fd);
        	printf("%s: wbyte = %d, buf.index = %d, buf_infos[buf.index].length = %ld\n", __func__, wbyte, buf.index, buf_infos[buf.index].length);

            // memset(timeBUuffer, 0, sizeof(timeBUuffer));
            // rgb_format = (struct rgb_888 *)timeBUuffer;
#if 0
            memset(img_star_map_xx, 0, 640*480*4);
            rgb_format = (struct rgb_888 *)img_star_map_xx;

            
            pix_format = (struct yuyv_422 *)buf_infos[buf.index].start;

            if (0 && count++ == 10) {
                int fd = open("camera_yuyv422_w640_h480.yuv", O_RDWR | O_CREAT, 0644);
                write(fd, pix_format, buf_infos[buf.index].length);
                close(fd);
                printf("==========================>\n");
            }
            

            for(rows = 0; rows < min_h; rows++) {
                // rgb_format = (struct rgb_888 *)&timeBUuffer[rows];


                for(cols = 0; cols < min_w; cols += 2) {
                    y1 = pix_format->y1;
                    u1 = pix_format->u - 128;
                    v1 = pix_format->v - 128;

                    r1 = y1 + v1 + ((v1 * 103) >> 8);
                    g1 = y1 - ((u1 * 88) >> 8) - ((v1 * 183) >> 8);
                    b1 = y1 + u1 + ((u1 * 198) >> 8);
                    
                    r1 = r1 > 255 ? 255 : (r1 < 0 ? 0 : r1);
                    g1 = g1 > 255 ? 255 : (g1 < 0 ? 0 : g1);
                    b1 = b1 > 255 ? 255 : (b1 < 0 ? 0 : b1);

                    y2 = pix_format->y2;
                    u2 = pix_format->u - 128;
                    v2 = pix_format->v - 128;
                    
                    r2 = y2 + v2 + ((v2 * 103) >> 8);
                    g2 = y2 - ((u2 * 88) >> 8) - ((v2 * 183) >> 8);
                    b2 = y2 + u2 + ((u2 * 198) >> 8);
                    
                    r2 = r2 > 255 ? 255 : (r2 < 0 ? 0 : r2);
                    g2 = g2 > 255 ? 255 : (g2 < 0 ? 0 : g2);
                    b2 = b2 > 255 ? 255 : (b2 < 0 ? 0 : b2);
                    
                    rgb1 = (0xFF << 24) | (r1 << 16) | (g1 << 8) | (b1 << 0);
                    rgb2 = (0xFF << 24) | (r2 << 16) | (g2 << 8) | (b2 << 0);

                    //rgb1 = (0xAA << 24) | (r1 << 16) | (g1 << 8) | (b1 << 0);
                    //rgb2 = (0xAA << 24) | (r2 << 16) | (g2 << 8) | (b2 << 0);
                    
                    rgb_format->rgb1 = rgb1;
                    rgb_format->rgb2 = rgb2;
                    
                    rgb_format++;
                    pix_format++;
                }
            }

            vedio_canvas_update_iii();
#endif

            // 数据处理完之后、再入队、往复
            ioctl(v4l2_fd, VIDIOC_QBUF, &buf);

            usleep(1000);
        }  
    }
    
    close(o_fd);
}

#define VIDEO_FRAME_WIDTH	640
#define VIDEO_FRAME_HEIGHT	480

//static 
void *vedio_thread(void *arg)
{
    width = VIDEO_FRAME_WIDTH;
    height = VIDEO_FRAME_HEIGHT;

    v4l2_dev_init("/dev/video0");

    v4l2_enum_formats();
    v4l2_print_formats();

    v4l2_set_format();

    v4l2_init_buffer();

    v4l2_stream_on();

    v4l2_read_data();

    return NULL;
}
// thread_initcall(vedio_thread);


/*
format<0x56595559>, description<YUYV 4:2:2>
size<640*480> <30fps>
size<320*240> <30fps>
size<720*480> <30fps>
size<1280*720> <30fps>
size<1920*1080> <30fps>
size<2592*1944> <15fps>
*/
































