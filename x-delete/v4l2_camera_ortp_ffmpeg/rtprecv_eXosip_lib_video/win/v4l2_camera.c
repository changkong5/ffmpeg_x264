/***************************************************************
 Copyright ? ALIENTEK Co., Ltd. 1998-2021. All rights reserved.
 文件名 : v4l2_camera.c
 作者 : 邓涛
 版本 : V1.0
 描述 : V4L2摄像头应用编程实战
 其他 : 无
 论坛 : www.openedv.com
 日志 : 初版 V1.0 2021/7/09 邓涛创建
 ***************************************************************/
 
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
#include "lvgl/lvgl.h"

#include "initcall.h"


#if 0
extern lv_color32_t img_star_map_xx[];

int main11(void);
void vedio_canvas_update_iii(void);
 
//#define FB_DEV "/dev/dri/card0"
#define FB_DEV "/dev/fb0"   // LCD设备节点
#define FRAMEBUFFER_COUNT 3 // 帧缓冲数量
#define RTC_DEV "/dev/rtc0" // RTC设备节点，fd名称
/*** 摄像头像素格式及其描述信息 ***/
typedef struct camera_format
{
    unsigned char description[32]; // 字符串描述信息
    unsigned int pixelformat;      // 像素格式
} cam_fmt;
 
/*** 描述一个帧缓冲的信息 ***/
typedef struct cam_buf_info
{
    unsigned short *start; // 帧缓冲起始地址
    unsigned long length;  // 帧缓冲长度
} cam_buf_info;
 
//unsigned short timeBUuffer[1024 * 64 * 2]; // 存放时间的像素点信息

// unsigned short timeBUuffer[1080 * 1920 * 4];

unsigned char  timeBUuffer[1920][1080 * 4];
 
static int width;                          // LCD宽度
static int height;                         // LCD高度
static unsigned short *screen_base = NULL; // LCD显存基地址
static int fb_fd = -1;                     // LCD设备文件描述符
static int v4l2_fd = -1;                   // 摄像头设备文件描述符
static cam_buf_info buf_infos[FRAMEBUFFER_COUNT];
static cam_fmt cam_fmts[10];
static int frm_width, frm_height; // 视频帧宽度和高度

unsigned long screen_size;
 
int main_fb0_info(void)
{
  int fp = 0;
  struct fb_var_screeninfo vinfo;
  struct fb_fix_screeninfo finfo;

  fp = open ("/dev/fb0", O_RDWR);
 
  if (fp < 0) {
    printf("Error : Can not open framebuffer device\n");
    exit(1);
  }
 
  if (ioctl(fp, FBIOGET_FSCREENINFO, &finfo)) {
    printf("Error reading fixed information\n");
    exit(2);
  }
 
  if (ioctl(fp, FBIOGET_VSCREENINFO, &vinfo)) {
    printf("Error reading variable information\n");
    exit(3);
  }

  close (fp);

  printf("finfo.smem_start  = 0x%08x\n", finfo.smem_start); 
  printf("finfo.smem_len    = %d\n", finfo.smem_len);  
  printf("finfo.mmio_start  = 0x%08x\n", finfo.mmio_start); 
  printf("finfo.mmio_len    = %d\n", finfo.mmio_len);
  printf("finfo.id          = %s\n", finfo.id);
  printf("finfo.type        = %d\n", finfo.type);
  printf("finfo.line_length = %d\n", finfo.line_length);


  printf("vinfo.xres           = %d\n", vinfo.xres);
  printf("vinfo.yres           = %d\n", vinfo.yres);
  printf("vinfo.xres_virtual   = %d\n", vinfo.xres_virtual);
  printf("vinfo.yres_virtual   = %d\n", vinfo.yres_virtual);

  printf("vinfo.xoffset        = %d\n", vinfo.xoffset);
  printf("vinfo.yoffset        = %d\n", vinfo.yoffset);

  printf("vinfo.height         = %d\n", vinfo.height);
  printf("vinfo.width          = %d\n", vinfo.width);
  printf("vinfo.bits_per_pixel = %d\n", vinfo.bits_per_pixel);

  printf("vinfo.pixclock       = %d\n", vinfo.pixclock);
  printf("vinfo.sync           = %d\n", vinfo.sync);  
  printf("vinfo.vmode          = %d\n", vinfo.vmode);
  printf("vinfo.rotate         = %d\n", vinfo.rotate); 

  printf("vinfo.hsync_len      = %d\n", vinfo.hsync_len);
  printf("vinfo.vsync_len      = %d\n", vinfo.vsync_len); 

  printf("vinfo.upper_margin   = %d\n", vinfo.upper_margin);
  printf("vinfo.lower_margin   = %d\n", vinfo.lower_margin);  
  printf("vinfo.left_margin    = %d\n", vinfo.left_margin);
  printf("vinfo.right_margin   = %d\n", vinfo.right_margin); 

  
  return 0;
}

void i_1(void)
{
    int *buf = (int *)screen_base;

    for (int i = 0; i < screen_size/4; i++) {

        // buf[i] = 0xFFFFFFFF;

        // buf[i] = 0x00FF0000;
        // buf[i] = 0x0000FF00;
        buf[i] = 0x000000FF;
    }
}

/*
format<0x56595559>, description<YUYV 4:2:2>
size<640*480> <30fps>
size<320*240> <30fps>
size<720*480> <30fps>
size<1280*720> <30fps>
size<1920*1080> <30fps>
size<2592*1944> <15fps>
*/

static int fb_dev_init(void)
{
    struct fb_var_screeninfo fb_var = {0};
    struct fb_fix_screeninfo fb_fix = {0};
    // unsigned long screen_size;
 
    /* 打开framebuffer设备 */
    fb_fd = open(FB_DEV, O_RDWR);
    if (0 > fb_fd)
    {
        fprintf(stderr, "open error: %s: %s\n", FB_DEV, strerror(errno));
        return -1;
    }
 
    /* 获取framebuffer设备信息 */
    ioctl(fb_fd, FBIOGET_VSCREENINFO, &fb_var);
    ioctl(fb_fd, FBIOGET_FSCREENINFO, &fb_fix);
 
    screen_size = fb_fix.line_length * fb_var.yres;
    width = fb_var.xres;
    height = fb_var.yres;
    printf("fb_var.xres = %d\n", fb_var.xres);
    printf("fb_var.yres = %d\n", fb_var.yres);
    printf("screen_size = %d, fb_fix.line_length = %d\n", screen_size, fb_fix.line_length);

    // width = 320;
    // height = 240;

    width = 640;
    height = 480;

    //width = 720;
    //height = 480;
 
    /* 内存映射 */
    screen_base = mmap(NULL, screen_size, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
    if (MAP_FAILED == (void *)screen_base)
    {
        perror("mmap error");
        close(fb_fd);
        return -1;
    }
 
    // i_1();
    /* LCD背景刷白 */
    //memset(screen_base, 0xF800, screen_size);
    return 0;
}

/*
struct v4l2_capability
{
u8 driver[16]; // 驱动名字
u8 card[32]; // 设备名字
u8 bus_info[32]; // 设备在系统中的位置
u32 version;// 驱动版本号
u32 capabilities;// 设备支持的操作
u32 reserved[4]; // 保留字段
};

*/

#if 0
   476  /* Values for 'capabilities' field */
   477  #define V4L2_CAP_VIDEO_CAPTURE      0x00000001  /* Is a video capture device */
   478  #define V4L2_CAP_VIDEO_OUTPUT       0x00000002  /* Is a video output device */
   479  #define V4L2_CAP_VIDEO_OVERLAY      0x00000004  /* Can do video overlay */
   480  #define V4L2_CAP_VBI_CAPTURE        0x00000010  /* Is a raw VBI capture device */
   481  #define V4L2_CAP_VBI_OUTPUT     0x00000020  /* Is a raw VBI output device */
   482  #define V4L2_CAP_SLICED_VBI_CAPTURE 0x00000040  /* Is a sliced VBI capture device */
   483  #define V4L2_CAP_SLICED_VBI_OUTPUT  0x00000080  /* Is a sliced VBI output device */
   484  #define V4L2_CAP_RDS_CAPTURE        0x00000100  /* RDS data capture */
   485  #define V4L2_CAP_VIDEO_OUTPUT_OVERLAY   0x00000200  /* Can do video output overlay */
   486  #define V4L2_CAP_HW_FREQ_SEEK       0x00000400  /* Can do hardware frequency seek  */
   487  #define V4L2_CAP_RDS_OUTPUT     0x00000800  /* Is an RDS encoder */

// v4l2_dev_init: cap.capabilities = 0x84200001, V4L2_CAP_VIDEO_CAPTURE = 0x0000001
#endif


static int v4l2_dev_init(const char *device)
{
    struct v4l2_capability cap = {0};
 
    /* 打开摄像头 */
    v4l2_fd = open(device, O_RDWR);
    if (0 > v4l2_fd)
    {
        fprintf(stderr, "open error: %s: %s\n", device, strerror(errno));
        return -1;
    }
 
    /* 查询设备功能 */
    ioctl(v4l2_fd, VIDIOC_QUERYCAP, &cap);

    printf("%s: cap.driver       = %s\n", __func__, cap.driver);
    printf("%s: cap.card         = %s\n", __func__, cap.card);
    printf("%s: cap.bus_info     = %s\n", __func__, cap.bus_info);
    printf("%s: cap.version      = 0x%08x\n", __func__, cap.version);
    printf("%s: cap.capabilities = 0x%08x, V4L2_CAP_VIDEO_CAPTURE = 0x%08x\n", __func__, cap.capabilities, V4L2_CAP_VIDEO_CAPTURE);
 
    /* 判断是否是视频采集设备 */
    if (!(V4L2_CAP_VIDEO_CAPTURE & cap.capabilities))
    {
        fprintf(stderr, "Error: %s: No capture video device!\n", device);
        close(v4l2_fd);
        return -1;
    }
 
    return 0;
}

#if 0
   801  struct v4l2_fmtdesc {
   802      __u32           index;             /* Format number      */
   803      __u32           type;              /* enum v4l2_buf_type */
   804      __u32               flags;
   805      __u8            description[32];   /* Description string */
   806      __u32           pixelformat;       /* Format fourcc      */
   807      __u32           reserved[4];
   808  };
#endif
 
static void v4l2_enum_formats(void)
{
    int ret = 0;
    struct v4l2_fmtdesc fmtdesc = {0};
    memset(&fmtdesc, 0, sizeof(fmtdesc));

    printf("========1=========\n");
 
    /* 枚举摄像头所支持的所有像素格式以及描述信息 */
    fmtdesc.index = 0;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    printf("========1======v4l2_fd = %d===ret = %d\n, errno = %d, strerror(errno) = %s\n", v4l2_fd, ioctl(v4l2_fd, VIDIOC_ENUM_FMT, &fmtdesc), errno, strerror(errno));

    while (0 == ioctl(v4l2_fd, VIDIOC_ENUM_FMT, &fmtdesc))
    {
        printf("========2=========\n");



        // 将枚举出来的格式以及描述信息存放在数组中
        cam_fmts[fmtdesc.index].pixelformat = fmtdesc.pixelformat;
        strcpy(cam_fmts[fmtdesc.index].description, fmtdesc.description);
        fmtdesc.index++;
    }
}
 
static void v4l2_print_formats(void)
{
    int i;
    struct v4l2_frmsizeenum frmsize = {0};
    struct v4l2_frmivalenum frmival = {0};
    
 
    frmsize.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    frmival.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    for (i = 0; cam_fmts[i].pixelformat; i++)
    {
 
        printf("format<0x%x>, description<%s>\n", cam_fmts[i].pixelformat,
               cam_fmts[i].description);
 
        /* 枚举出摄像头所支持的所有视频采集分辨率 */
        frmsize.index = 0;
        frmsize.pixel_format = cam_fmts[i].pixelformat;
        frmival.pixel_format = cam_fmts[i].pixelformat;
        while (0 == ioctl(v4l2_fd, VIDIOC_ENUM_FRAMESIZES, &frmsize))
        {
 
            printf("size<%d*%d> ",
                   frmsize.discrete.width,
                   frmsize.discrete.height);
            frmsize.index++;
 
            /* 获取摄像头视频采集帧率 */
            frmival.index = 0;
            frmival.width = frmsize.discrete.width;
            frmival.height = frmsize.discrete.height;
            while (0 == ioctl(v4l2_fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival))
            {
 
                printf("<%dfps>", frmival.discrete.denominator /
                                      frmival.discrete.numerator);
                frmival.index++;
            }
            printf("\n");
        }
        printf("\n");
    }
}
 
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
    if (0 > ioctl(v4l2_fd, VIDIOC_S_FMT, &fmt))
    {
        fprintf(stderr, "ioctl error: VIDIOC_S_FMT: %s\n", strerror(errno));
        return -1;
    }
 
    /*** 判断是否已经设置为我们要求的RGB565像素格式
    如果没有设置成功表示该设备不支持RGB565像素格式 */
    if (V4L2_PIX_FMT_RGB565 != fmt.fmt.pix.pixelformat)
    {
        fprintf(stderr, "Error: the device does not support RGB565 format!\n");
        // return -1;
    }
 
    frm_width = fmt.fmt.pix.width;   // 获取实际的帧宽度
    frm_height = fmt.fmt.pix.height; // 获取实际的帧高度
    printf("视频帧大小<%d * %d>\n", frm_width, frm_height);
 
    /* 获取streamparm */
    streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(v4l2_fd, VIDIOC_G_PARM, &streamparm);
 
    /** 判断是否支持帧率设置 **/
    if (V4L2_CAP_TIMEPERFRAME & streamparm.parm.capture.capability)
    {
        streamparm.parm.capture.timeperframe.numerator = 1;
        streamparm.parm.capture.timeperframe.denominator = 30; // 30fps
        if (0 > ioctl(v4l2_fd, VIDIOC_S_PARM, &streamparm))
        {
            fprintf(stderr, "ioctl error: VIDIOC_S_PARM: %s\n", strerror(errno));
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
    if (0 > ioctl(v4l2_fd, VIDIOC_REQBUFS, &reqbuf))
    {
        fprintf(stderr, "ioctl error: VIDIOC_REQBUFS: %s\n", strerror(errno));
        return -1;
    }
 
    /* 建立内存映射 */
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    for (buf.index = 0; buf.index < FRAMEBUFFER_COUNT; buf.index++)
    {
 
        ioctl(v4l2_fd, VIDIOC_QUERYBUF, &buf);
        buf_infos[buf.index].length = buf.length;
        buf_infos[buf.index].start = mmap(NULL, buf.length,
                                          PROT_READ | PROT_WRITE, MAP_SHARED,
                                          v4l2_fd, buf.m.offset);
        if (MAP_FAILED == buf_infos[buf.index].start)
        {
            perror("mmap error");
            return -1;
        }
    }
 
    /* 入队 */
    for (buf.index = 0; buf.index < FRAMEBUFFER_COUNT; buf.index++)
    {
        ioctl(v4l2_fd, VIDIOC_QUERYBUF, &buf);
 
        if (0 > ioctl(v4l2_fd, VIDIOC_QBUF, &buf))
        {
            fprintf(stderr, "ioctl error: VIDIOC_QBUF: %s\n", strerror(errno));
            return -1;
        }
    }
 
    return 0;
}
 
static int v4l2_stream_on(void)
{
    /* 打开摄像头、摄像头开始采集数据 */
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
 
    if (0 > ioctl(v4l2_fd, VIDIOC_STREAMON, &type))
    {
        fprintf(stderr, "ioctl error: VIDIOC_STREAMON: %s\n", strerror(errno));
        return -1;
    }
 
    return 0;
}

// format<0x56595559>, description<YUYV 4:2:2>

//    fmt.fmt.pix.width = width;              // 视频帧宽度
//    fmt.fmt.pix.height = height;            // 视频帧高度

//    frm_width = fmt.fmt.pix.width;   // 获取实际的帧宽度
//    frm_height = fmt.fmt.pix.height; // 获取实际的帧高度

//     width = 640;
//     height = 480;
// 640 * 480
 
static void v4l2_read_data_3333(void)
{
    struct v4l2_buffer buf = {0};
    unsigned short *base;
    unsigned short *start;
    int min_w, min_h;
    int j,i;
 
    if (width > frm_width)
        min_w = frm_width;
    else
        min_w = width;

    if (height > frm_height)
        min_h = frm_height;
    else
        min_h = height;
 
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    for ( ; ; ) {
 
        for(buf.index = 0; buf.index < FRAMEBUFFER_COUNT; buf.index++) {
 
            ioctl(v4l2_fd, VIDIOC_DQBUF, &buf);     //出队

            for (j = 0, base=screen_base, start=buf_infos[buf.index].start; j < min_h; j++) {
 
                memcpy(base, start, min_w * 2); //RGB565 一个像素占2个字节
                #if 0
                for (i = 0; i < 1024;i++)
                {
                    printf("%d\r\n", base[i]);
                }
                #endif
                base += width; // LCD显示指向下一行
                start += frm_width;//指向下一行数据

                // printf("-------------------yyy---\n");
            }

            // 数据处理完之后、再入队、往复
            ioctl(v4l2_fd, VIDIOC_QBUF, &buf);
        }

        
    }
}

#include <stdint.h>

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

// timeBUuffer

static void v4l2_read_data(void)
{
    struct v4l2_buffer buf = {0};
    unsigned short *base;
    unsigned short *start;
    int min_w, min_h;
    int j,i;


    int y1, u1, v1, y2, u2, v2;
    int r1, g1, b1, r2, g2, b2;
    int rgb1, rgb2;
    int rows ,cols;
    struct rgb_888 *rgb_format = (struct rgb_888 *)timeBUuffer;
    struct yuyv_422 *pix_format = (struct yuyv_422 *)NULL;

 
    if (width > frm_width)
        min_w = frm_width;
    else
        min_w = width;
    if (height > frm_height)
        min_h = frm_height;
    else
        min_h = height;

    // min_w = 640, min_h = 480
    printf("min_w = %d, min_h = %d\n", min_w, min_h);
 
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    for ( ; ; ) {
 
        for(buf.index = 0; buf.index < FRAMEBUFFER_COUNT; buf.index++) {
 
            ioctl(v4l2_fd, VIDIOC_DQBUF, &buf);     //出队

            // memset(timeBUuffer, 0, sizeof(timeBUuffer));
            // rgb_format = (struct rgb_888 *)timeBUuffer;

            memset(img_star_map_xx, 0, 640*480*4);
            rgb_format = (struct rgb_888 *)img_star_map_xx;

            
            pix_format = (struct yuyv_422 *)buf_infos[buf.index].start;

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
            // memcpy(screen_base, timeBUuffer, 1080 * 1920 * 4);

            // 数据处理完之后、再入队、往复
            ioctl(v4l2_fd, VIDIOC_QBUF, &buf);

            usleep(1000);
        }

        
    }
}


 
// int main(int argc, char *argv[])
static void *vedio_thread(void *arg)
{
//    main_fb0_info();

//    main11();
    width = 640;
    height = 480;

 printf("-----------1-------------\n");
    /* 初始化LCD */
 //   if (fb_dev_init())
 //       exit(EXIT_FAILURE);
 printf("-----------2-------------\n");
    /* 初始化摄像头 */
    if (v4l2_dev_init("/dev/video0"))
        exit(EXIT_FAILURE);
 printf("-----------3-------------\n");
    /* 枚举所有格式并打印摄像头支持的分辨率及帧率 */
    v4l2_enum_formats();
    v4l2_print_formats();
printf("-----------4-------------\n"); 
    /* 设置格式 */
    if (v4l2_set_format())
        exit(EXIT_FAILURE);
printf("-----------5-------------\n"); 
    /* 初始化帧缓冲：申请、内存映射、入队 */
    if (v4l2_init_buffer())
        exit(EXIT_FAILURE);
printf("-----------6-------------\n"); 
    /* 开启视频采集 */
    if (v4l2_stream_on())
        exit(EXIT_FAILURE);
printf("-----------7-------------\n"); 
    /* 读取数据：出队 */
    v4l2_read_data(); // 在函数内循环采集数据、将其显示到LCD屏
printf("-----------8-------------\n"); 
    //exit(EXIT_SUCCESS);

    return NULL;
}
thread_initcall(vedio_thread);

#if 0 
int main(int argc, char *argv[])
{
 
    if (2 != argc)
    {
        fprintf(stderr, "Usage: %s <video_dev>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
 
    /* 初始化LCD */
    if (fb_dev_init())
        exit(EXIT_FAILURE);
 
    /* 初始化摄像头 */
    if (v4l2_dev_init(argv[1]))
        exit(EXIT_FAILURE);
 
    /* 枚举所有格式并打印摄像头支持的分辨率及帧率 */
    v4l2_enum_formats();
    v4l2_print_formats();
 
    /* 设置格式 */
    if (v4l2_set_format())
        exit(EXIT_FAILURE);
 
    /* 初始化帧缓冲：申请、内存映射、入队 */
    if (v4l2_init_buffer())
        exit(EXIT_FAILURE);
 
    /* 开启视频采集 */
    if (v4l2_stream_on())
        exit(EXIT_FAILURE);
 
    /* 读取数据：出队 */
    v4l2_read_data(); // 在函数内循环采集数据、将其显示到LCD屏
 
    exit(EXIT_SUCCESS);
}
#endif

#endif


