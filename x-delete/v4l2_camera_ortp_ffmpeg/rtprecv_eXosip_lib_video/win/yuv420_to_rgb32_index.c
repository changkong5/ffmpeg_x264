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

struct yuyv_420 {
    uint8_t y;
    uint8_t u;
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
    } rgb;
};

/*

  --------------------------------------------------------------------
  |  Y0      Y1   |   Y2      Y3   |   Y4      Y5   |  Y6      Y7   |

  |     U0 V0     |      U1 V1     |      U2 V2     |    U3 V3      |

  |  Y8      Y9   |   Y10     Y11  |   Y12     Y13  |  Y14     Y15  |
  --------------------------------------------------------------------

  PIX0 = Y0U0V0
  PIX1 = Y1U0V0

  PIX2 = Y2U1V1
  PIX3 = Y3U1V1

  PIX4 = Y4U2V2
  PIX5 = Y5U2V2

  PIX6 = Y6U3V3
  PIX7 = Y7U3V3

  PIX8 = Y8U0V0
  PIX9 = Y9U0V0

  PIX10 = Y10U1V1
  PIX11 = Y11U1V1

  PIX12 = Y12U2V2
  PIX13 = Y13U2V2

  PIX14 = Y14U3V3
  PIX15 = Y14U3V3
*/

// YUV420P 格式存储，即先存储Y，然后再存储U，最后在存储V

static void read_camera_yuyv420_file(void)
{
    int y1, u1, v1, y2, u2, v2;
    int r1, g1, b1, r2, g2, b2;
    int rgb1, rgb2;
    int rows ,cols;

    struct rgb_888 *rgb_format = NULL;
    struct yuyv_420 pix_format[1];

    int min_w = 352;
    int min_h = 288;
    //int rgb32_len = min_w * min_h * 4;
    int yuyv_len = min_w * min_h * 3 / 2;
    char yuyv_buf[yuyv_len];
    //char rgb32_buf[rgb32_len];

    int ylen = min_h * min_w;
    char *ydata = &yuyv_buf[0];
    char *udata = &yuyv_buf[ylen];
    char *vdata = &yuyv_buf[ylen * 5 / 4];

    int fd = open("yuv_420-w352xh288.yuv", O_RDWR | O_CREAT, 0644);
    read(fd, yuyv_buf, yuyv_len);
    close(fd);

    //memset(rgb32_buf, 0, rgb32_len);
    //rgb_format = (struct rgb_888 *)rgb32_buf;   

    memset(img_star_map_xx, 0, 640*480*4);
    rgb_format = (struct rgb_888 *)img_star_map_xx;


    for(rows = 0; rows < min_h; rows++) {
        rgb_format = (struct rgb_888 *)img_star_map_xx[rows];

        for(cols = 0; cols < min_w; cols += 1) {

            pix_format->y = ydata[rows * min_w + cols];
            //pix_format->u = udata[rows * min_w / 4 + cols / 2];   
            //pix_format->v = vdata[rows * min_w / 4 + cols / 2];  
            pix_format->u = udata[(rows / 2) * (min_w / 2) + cols / 2];   
            pix_format->v = vdata[(rows / 2) * (min_w / 2) + cols / 2];       

            y1 = pix_format->y;
            u1 = pix_format->u - 128;
            v1 = pix_format->v - 128;

            // YUB turn to RGB
            r1 = y1 + v1 + ((v1 * 103) >> 8);
            g1 = y1 - ((u1 * 88) >> 8) - ((v1 * 183) >> 8);
            b1 = y1 + u1 + ((u1 * 198) >> 8);
            
            // the range from 0 to 255 (i.e [0, 255])
            r1 = r1 > 255 ? 255 : (r1 < 0 ? 0 : r1);
            g1 = g1 > 255 ? 255 : (g1 < 0 ? 0 : g1);
            b1 = b1 > 255 ? 255 : (b1 < 0 ? 0 : b1);
            
            // storge 32 bit RGB data
            rgb_format->rgb.a = 0xff;
            rgb_format->rgb.r = r1;
            rgb_format->rgb.g = g1;
            rgb_format->rgb.b = b1;
            
            rgb_format++;
        }
    }

    vedio_canvas_update_iii();
}


static gboolean cancel_fire_3(gpointer data)
{
    g_print("%s: cancel_fire_3() quit \n", __FILE__);
 
    read_camera_yuyv420_file();

    return G_SOURCE_REMOVE;
}

static int main_init(void)
{
    printf("-----main_init------ \n");

    g_timeout_add(1000, cancel_fire_3, NULL);
    
    return 0;
}
// fn_initcall(main_init);




/*
YUV420SP

struct uv {
    uint8_t u;
    uint8_t v;
};

struct uv uv;

pix_format->y = ydata[rows * min_w + cols];
pix_format->uv = udata[rows * min_w / 4 + cols / 2];
// pix_format->uv = udata[rows * min_w / 2 + cols / 2];

*/
