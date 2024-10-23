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
#include "gdbus/gdbus.h"

extern lv_color32_t img_star_map_xx[][640];

void vedio_canvas_update_iii(void);

/*
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
*/

struct yuyv_422 {
    uint8_t y1;
    uint8_t u;
    uint8_t y2;
    uint8_t v;
};

struct rgb_888 {
    union {
        struct {
            uint8_t b;
            uint8_t g;
            uint8_t r;
            uint8_t a;
        };
        uint32_t rgb;
    } rgb1;
    
    union {
        struct {
            uint8_t b;
            uint8_t g;
            uint8_t r;
            uint8_t a;
        };
        uint32_t rgb;
    } rgb2;
};

static void read_camera_yuyv422_file(void)
{
    int y1, u1, v1, y2, u2, v2;
    int r1, g1, b1, r2, g2, b2;
    int rgb1, rgb2;
    int rows ,cols;

    struct rgb_888 *rgb_format = NULL;
    struct yuyv_422 *pix_format = NULL;

    int min_w = 640;
    int min_h = 480;
    //int rgb32_len = min_w * min_h * 4;
    int yuyv_len = min_w * min_h * 2;
    char yuyv_buf[yuyv_len];
    //char rgb32_buf[rgb32_len];

    int fd = open("camera_yuyv422_w640_h480.yuv", O_RDWR | O_CREAT, 0644);
    read(fd, yuyv_buf, yuyv_len);
    close(fd);

    //memset(rgb32_buf, 0, rgb32_len);
    //rgb_format = (struct rgb_888 *)rgb32_buf;   

    memset(img_star_map_xx, 0, 640*480*4);
    rgb_format = (struct rgb_888 *)img_star_map_xx;   
    pix_format = (struct yuyv_422 *)yuyv_buf;


    for(rows = 0; rows < min_h; rows++) {
        rgb_format = (struct rgb_888 *)img_star_map_xx[rows];

        for(cols = 0; cols < min_w; cols += 2) {

            // 32 bit YUYV data
            y1 = pix_format->y1;
            u1 = pix_format->u - 128;
            v1 = pix_format->v - 128;
            
            y2 = pix_format->y2;
            u2 = pix_format->u - 128;
            v2 = pix_format->v - 128;

            // YUB turn to RGB
            r1 = y1 + v1 + ((v1 * 103) >> 8);
            g1 = y1 - ((u1 * 88) >> 8) - ((v1 * 183) >> 8);
            b1 = y1 + u1 + ((u1 * 198) >> 8);
            
            r2 = y2 + v2 + ((v2 * 103) >> 8);
            g2 = y2 - ((u2 * 88) >> 8) - ((v2 * 183) >> 8);
            b2 = y2 + u2 + ((u2 * 198) >> 8);
            
            // the range from 0 to 255 (i.e [0, 255])
            r1 = r1 > 255 ? 255 : (r1 < 0 ? 0 : r1);
            g1 = g1 > 255 ? 255 : (g1 < 0 ? 0 : g1);
            b1 = b1 > 255 ? 255 : (b1 < 0 ? 0 : b1);
            
            r2 = r2 > 255 ? 255 : (r2 < 0 ? 0 : r2);
            g2 = g2 > 255 ? 255 : (g2 < 0 ? 0 : g2);
            b2 = b2 > 255 ? 255 : (b2 < 0 ? 0 : b2);
            
            // storge 32 bit RGB data
            rgb_format->rgb1.a = 0xff;
            rgb_format->rgb1.r = r1;
            rgb_format->rgb1.g = g1;
            rgb_format->rgb1.b = b1;
            
            rgb_format->rgb2.a = 0xff;
            rgb_format->rgb2.r = r2;
            rgb_format->rgb2.g = g2;
            rgb_format->rgb2.b = b2;
            
            rgb_format++;
            pix_format++;
        }
    }

    vedio_canvas_update_iii();
}


static gboolean cancel_fire_3(gpointer data)
{
    g_print("%s: cancel_fire_3() quit \n", __FILE__);
 
    read_camera_yuyv422_file();

    return G_SOURCE_REMOVE;
}

static int main_init(void)
{
    printf("-----main_init------ \n");

    g_timeout_add(1000, cancel_fire_3, NULL);
    
    return 0;
}
// fn_initcall(main_init);




