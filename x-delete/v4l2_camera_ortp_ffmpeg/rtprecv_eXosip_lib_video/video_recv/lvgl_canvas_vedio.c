#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <lvgl/lvgl.h>

#include "initcall.h"

#define CANVAS_WIDTH  640
#define CANVAS_HEIGHT 480

struct rgb_format {
    uint8_t b;
    uint8_t g;
    uint8_t r;
    uint8_t a;
};

lv_color32_t img_star_map_xx[CANVAS_HEIGHT][CANVAS_WIDTH];
// lv_color32_t img_star_map_xx[CANVAS_WIDTH * CANVAS_HEIGHT];

/*
// correct
const lv_image_dsc_t img_star_xx = {
  .header.cf = LV_COLOR_FORMAT_ARGB8888,
  .header.magic = LV_IMAGE_HEADER_MAGIC,
  .header.w = CANVAS_WIDTH,
  .header.h = CANVAS_HEIGHT,  
  .data = (const uint8_t *)img_star_map_xx,
  .data_size = sizeof(img_star_map_xx),
};
*/

// corror
const lv_image_dsc_t img_star_xx = {
    .header.w = CANVAS_WIDTH,
    .header.h = CANVAS_HEIGHT,
    .header.stride = CANVAS_WIDTH * 4,
    .header.cf = LV_COLOR_FORMAT_ARGB8888,
    .data = (const uint8_t *)img_star_map_xx,
    .data_size = sizeof(img_star_map_xx),
};

struct vedio_canvas {
	int width;
	int height;
	int pix;
	lv_color_format_t cf;
	lv_obj_t *canvas;
	lv_layer_t layer;
	lv_draw_image_dsc_t dsc;
} vedio[1];


void vedio_canvas_init(void)
{
	/*Create a buffer for the canvas*/
    static uint8_t cbuf[LV_CANVAS_BUF_SIZE(CANVAS_WIDTH, CANVAS_HEIGHT, 32, LV_DRAW_BUF_STRIDE_ALIGN)];

	vedio->pix = 32;
	vedio->width = CANVAS_HEIGHT;
	vedio->height = CANVAS_HEIGHT;
	vedio->cf = LV_COLOR_FORMAT_ARGB8888;
	
	/*Create a canvas and initialize its palette*/
    vedio->canvas = lv_canvas_create(lv_screen_active());
    lv_canvas_set_buffer(vedio->canvas, cbuf, vedio->width, vedio->height, vedio->cf);
    lv_canvas_fill_bg(vedio->canvas, lv_color_hex3(0xccc), LV_OPA_COVER);
    lv_obj_center(vedio->canvas);
    
    lv_canvas_init_layer(vedio->canvas, &vedio->layer);

    lv_draw_image_dsc_init(&vedio->dsc);
    vedio->dsc.src = &img_star_xx;
    
    lv_area_t coords = {0, 0, img_star_xx.header.w - 1, img_star_xx.header.h - 1};
    
    lv_draw_image(&vedio->layer, &vedio->dsc, &coords);
    lv_canvas_finish_layer(vedio->canvas, &vedio->layer);
}

void vedio_canvas_update_iii(void)
{
    lv_area_t coords = {0, 0, img_star_xx.header.w - 1, img_star_xx.header.h - 1};
    
    vedio->dsc.src = &img_star_xx;
    lv_draw_image(&vedio->layer, &vedio->dsc, &coords);
    lv_canvas_finish_layer(vedio->canvas, &vedio->layer);
}

void vedio_canvas_update(void)
{
	static int count = 0;
	for (int h = 0; h < CANVAS_HEIGHT; h++) {
        for (int w = 0; w < CANVAS_WIDTH; w++) {
        
        	switch (count) {
        	case 0:
                img_star_map_xx[h][w] = lv_color32_make(0xFF, 0x00, 0x00, 0xFF);
        		// img_star_map_xx[h * CANVAS_WIDTH + w] = lv_color32_make(0xFF, 0x00, 0x00, 0xFF);
        		break;
        		
        	case 1:
                img_star_map_xx[h][w] = lv_color32_make(0x00, 0xFF, 0x00, 0xFF);
        		// img_star_map_xx[h * CANVAS_WIDTH + w] = lv_color32_make(0x00, 0xFF, 0x00, 0xFF);
        		break;
        		
        	case 2:
                img_star_map_xx[h][w] = lv_color32_make(0x00, 0x00, 0xFF, 0xFF);
        		// img_star_map_xx[h * CANVAS_WIDTH + w] = lv_color32_make(0x00, 0x00, 0xFF, 0xFF);
        		break;

        	default:
        		count = 0;
        		break;
        	}
        }
    }
    
    count ++;

    lv_area_t coords = {0, 0, img_star_xx.header.w - 1, img_star_xx.header.h - 1};
    
    vedio->dsc.src = &img_star_xx;
    lv_draw_image(&vedio->layer, &vedio->dsc, &coords);
    lv_canvas_finish_layer(vedio->canvas, &vedio->layer);
}

void video_update_cb_i(lv_timer_t * timer)
{
	// vedio_canvas_update();
}

lv_timer_t *timer;


// vedio_canvas_init();
//    timer = lv_timer_create(video_update_cb_i, 500, NULL);

static int lv_example_canvas_init(void)
{

	vedio_canvas_init();

	timer = lv_timer_create(video_update_cb_i, 500, NULL);

	return 0;
}
fn_initcall(lv_example_canvas_init);











