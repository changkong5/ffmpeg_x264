#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <lvgl/lvgl.h>

#include "initcall.h"

static const char *getenv_default(const char *name, const char *dflt)
{
    return getenv(name) ? : dflt;
}

#if LV_USE_LINUX_FBDEV
static void lv_linux_disp_init(void)
{
    const char *device = getenv_default("LV_LINUX_FBDEV_DEVICE", "/dev/fb0");
    lv_display_t * disp = lv_linux_fbdev_create();

    lv_linux_fbdev_set_file(disp, device);
}
#elif LV_USE_LINUX_DRM
static void lv_linux_disp_init(void)
{
    const char *device = getenv_default("LV_LINUX_DRM_CARD", "/dev/dri/card0");
    lv_display_t * disp = lv_linux_drm_create();

    lv_linux_drm_set_file(disp, device, -1);
}
#elif LV_USE_WAYLAND
static void lv_linux_disp_init(void)
{
    const int width = atoi(getenv("LV_WAYLAND_VIDEO_WIDTH") ? : "640");
    const int height = atoi(getenv("LV_WAYLAND_VIDEO_HEIGHT") ? : "480");

    lv_disp_t *disp = lv_wayland_window_create(width, height, "Window Title", NULL);
    printf("=====================> %s: disp = %p\n", __func__, disp);

    // lv_wayland_window_set_fullscreen(disp, true);
}
#elif LV_USE_SDL
static void lv_linux_disp_init(void)
{
    const int width = atoi(getenv("LV_SDL_VIDEO_WIDTH") ? : "800");
    const int height = atoi(getenv("LV_SDL_VIDEO_HEIGHT") ? : "480");

    lv_sdl_window_create(width, height);
}
#else
#error Unsupported configuration
#endif


static int lvgl_init(void)
{
    /* lvgl init */
	lv_init();

	/* Linux display device init */
    lv_linux_disp_init();

    return 0;
}
fn_initcall_sort(lvgl_init, 0, 0);

static int lvgl_run = 0;

void lvgl_exit(void)
{
	lvgl_run = 0;
}

static void *lvgl_thread(void *arg)
{
	lvgl_run = 1;

    while (lvgl_run) {
        // usleep(5000);

        /* Run until the last window closes */
        if (!lv_wayland_window_is_open(NULL)) {
            break;
        }

        bool ret = lv_wayland_timer_handler();
        // printf("%s: ret = %d\n", __func__, ret);
    }

    return NULL;
}
thread_initcall(lvgl_thread);














