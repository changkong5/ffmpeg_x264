#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
// #include "lvgl/src/drivers/wayland/lv_wayland.h"

#if 0
//#define CANVAS_WIDTH  500
//#define CANVAS_HEIGHT  500

#define CANVAS_WIDTH  50
#define CANVAS_HEIGHT  50

#undef  LV_BUILD_EXAMPLES
#define LV_BUILD_EXAMPLES 1


/*
*dump.c
*/
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<signal.h>      /*forsignal*/
#include<execinfo.h>    /*forbacktrace()*/

#define BACKTRACE_SIZE 16

void dump(void)
{
    int j, nptrs;
    void *buffer[BACKTRACE_SIZE];
    char **strings;

    nptrs = backtrace(buffer, BACKTRACE_SIZE);

    printf("backtrace() returned %d addresses\n", nptrs);

    strings=backtrace_symbols(buffer, nptrs);

    if(strings == NULL){
        perror("backtrace_symbols");
        exit(EXIT_FAILURE);
    }

    for(j=0; j<nptrs; j++)
        printf("[%02d]%s\n",j,strings[j]);

    free(strings);
}

void signal_handler(int signo)
{

#if 1
    char buff[64]={0x00};

    sprintf(buff,"cat/proc/%d/maps",getpid());

    system((const char*)buff);
#endif

    printf("\n=========>>>catch signal %d<<<=========\n",signo);

    printf("Dump stack start...\n");
    dump();
    printf("Dump stack end...\n");

    signal(signo,SIG_DFL);      /*恢复信号默认处理*/
    raise(signo);               /*重新发送信号*/
}

extern void dump(void);
extern void signal_handler(int signo);
extern int add(int num);

//////////////////////////////////////////////////////////////////////////


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


#if 0
static void event_cb(lv_event_t * e)
{
    // LV_LOG_USER("Clicked");

    static uint32_t cnt = 1;
    lv_obj_t * btn = lv_event_get_target(e);
    lv_obj_t * label = lv_obj_get_child(btn, 0);
    lv_label_set_text_fmt(label, "%"LV_PRIu32, cnt);
    cnt++;
}

/**
 * Add click event to a button
 */
static void lv_example_event_1(void)
{
    lv_obj_t *btn = lv_button_create(lv_screen_active());

    printf("=====================> %s: lv_screen_active() = %p\n", __func__, lv_screen_active());
    printf("=====================> %s: btn = %p\n", __func__, btn);
    lv_obj_set_size(btn, 400, 200);

    lv_obj_center(btn);
    lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, "Click me!");
    lv_obj_center(label);
}
#endif

#undef LV_LOG_USER
#define LV_LOG_USER printf

static void event_handler(lv_event_t * e)
{
    lv_obj_t *win = lv_event_get_user_data(e);
    lv_obj_t * obj = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);                    /* 获取事件类型 */
    LV_UNUSED(obj);

    if(code == LV_EVENT_CLICKED)                                    /* 按钮按下 */
    {
        lv_obj_add_flag(win, LV_OBJ_FLAG_HIDDEN);                   /* 隐藏窗口 */
    }

    printf("Button %d clicked\n", (int)lv_obj_get_index(obj));
}

void lv_example_win_1(void)
{
    lv_obj_t * win = lv_win_create(lv_screen_active());
    lv_obj_t * btn;
    btn = lv_win_add_button(win, LV_SYMBOL_LEFT, 40);
    lv_obj_add_event_cb(btn, event_handler, LV_EVENT_CLICKED, win);

    lv_obj_set_size(win, 400, 200);
    lv_obj_center(win);

    // 
    //lv_obj_redraw(win, true);
    // lv_win_drag_enable(win, true);
    // lv_obj_set_drag(win, true);
    // lv_obj_add_flag(win, LV_OBJ_FLAG_DRAG);

    lv_win_add_title(win, "A title");

    btn = lv_win_add_button(win, LV_SYMBOL_RIGHT, 40);
    lv_obj_add_event_cb(btn, event_handler, LV_EVENT_CLICKED, win);

    btn = lv_win_add_button(win, LV_SYMBOL_CLOSE, 60);
    lv_obj_add_event_cb(btn, event_handler, LV_EVENT_CLICKED, win);

    lv_obj_t * cont = lv_win_get_content(win);  /*Content can be added here*/
    lv_obj_t * label = lv_label_create(cont);
    lv_label_set_text(label, "This is\n"
                      "a pretty\n"
                      "long text\n"
                      "to see how\n"
                      "the window\n"
                      "becomes\n"
                      "scrollable.\n"
                      "\n"
                      "\n"
                      "Some more\n"
                      "text to be\n"
                      "sure it\n"
                      "overflows. :)");

}

/**
 * @brief  按钮事件回调
 * @param  *e ：事件相关参数的集合，它包含了该事件的所有数据
 * @return 无
 */
static void btn_event_cb(lv_event_t *e)
{
    lv_obj_t * obj = lv_event_get_target(e);
    lv_obj_t *win = lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);                    /* 获取事件类型 */

    if(code == LV_EVENT_CLICKED)                                    /* 按钮按下 */
    {
        lv_obj_add_flag(win, LV_OBJ_FLAG_HIDDEN);                   /* 隐藏窗口 */
    }
    // printf("=======================\n");
    printf("Button %d clicked", (int)lv_obj_get_index(obj));
}

#define scr_act_width()     640
#define scr_act_height()    480

/**
 * @brief  窗口实例
 * @param  无
 * @return 无
 */
static void lv_example_win(void)
{
    const lv_font_t *font;

    /* 根据屏幕宽度选择字体 */
    if (scr_act_width() <= 480)
    {
        font = &lv_font_montserrat_12;
    }
    else
    {
        font = &lv_font_montserrat_20;
    }

    /* 窗口整体 */
    lv_obj_t * win = lv_win_create(lv_screen_active());
    //win = lv_win_create(lv_scr_act(), scr_act_height()/12);                                     /* 创建窗口 */
    lv_obj_set_size(win, scr_act_width()* 5/8, scr_act_height()* 4/7);                          /* 设置大小 */
    lv_obj_center(win);                                                                         /* 设置位置 */
    lv_obj_set_style_border_width(win, 1, LV_STATE_DEFAULT);                                    /* 设置边框宽度 */
    lv_obj_set_style_border_color(win, lv_color_hex(0x8a8a8a), LV_STATE_DEFAULT);               /* 设置边框颜色 */
    lv_obj_set_style_border_opa(win, 100, LV_STATE_DEFAULT);                                    /* 设置边框透明度 */
    lv_obj_set_style_radius(win, 10, LV_STATE_DEFAULT);                                         /* 设置圆角 */

    /*************************   第一部分：头部   ****************************/

    /* 左侧按钮 */
    lv_obj_t *btn_setting = lv_win_add_button(win, LV_SYMBOL_SETTINGS,30);                         /* 添加按钮 */
    lv_obj_set_style_bg_opa(btn_setting, 0, LV_STATE_DEFAULT);                                  /* 设置背景透明度 */
    lv_obj_set_style_shadow_width(btn_setting, 0, LV_STATE_DEFAULT);                            /* 设置阴影宽度 */
    lv_obj_set_style_text_color(btn_setting, lv_color_hex(0x000000), LV_STATE_DEFAULT);         /* 设置文本颜色 */

    /* 标题 */
    lv_obj_t *title = lv_win_add_title(win, "Setting");                                         /* 添加标题 */
    lv_obj_set_style_text_font(title, font, LV_STATE_DEFAULT);                                  /* 设置字体 */

    /* 右侧按钮 */
    lv_obj_t *btn_close = lv_win_add_button(win, LV_SYMBOL_CLOSE,30);                              /* 添加按钮 */
    lv_obj_set_style_bg_opa(btn_close, 0, LV_STATE_DEFAULT);                                    /* 设置背景透明度 */
    lv_obj_set_style_shadow_width(btn_close, 0, LV_STATE_DEFAULT);                              /* 设置阴影宽度 */
    lv_obj_set_style_text_color(btn_close, lv_color_hex(0x000000), LV_STATE_DEFAULT);           /* 设置文本颜色（未按下） */
    lv_obj_set_style_text_color(btn_close, lv_color_hex(0xff0000), LV_STATE_PRESSED);           /* 设置文本颜色（已按下） */
    lv_obj_add_event_cb(btn_close, btn_event_cb, LV_EVENT_CLICKED, win);                       /* 添加事件 */

    /*************************   第二部分：主体   ****************************/

    /* 主体背景 */
    lv_obj_t *content = lv_win_get_content(win);                                                /* 获取主体 */
    lv_obj_set_style_bg_color(content, lv_color_hex(0xffffff),LV_STATE_DEFAULT);                /* 设置背景颜色 */

    /* 音乐音量滑块 */
    lv_obj_t *slider_audio = lv_slider_create(content);                                         /* 创建滑块 */
    lv_obj_set_size(slider_audio, scr_act_width()/3, scr_act_height()/30);                      /* 设置大小 */
    lv_obj_align(slider_audio, LV_ALIGN_CENTER, 15, -scr_act_height()/14);                      /* 设置位置 */
    lv_slider_set_value(slider_audio, 50, LV_ANIM_OFF);                                         /* 设置当前值 */
    lv_obj_set_style_bg_color(slider_audio, lv_color_hex(0x787c78), LV_PART_MAIN);              /* 设置主体颜色 */
    lv_obj_set_style_bg_color(slider_audio, lv_color_hex(0xc3c3c3), LV_PART_INDICATOR);         /* 设置指示器颜色 */
    lv_obj_remove_style(slider_audio, NULL, LV_PART_KNOB);                                      /* 移除旋钮 */

    /* 音乐音量图标 */
    lv_obj_t *label_audio = lv_label_create(content);                                           /* 创建音量标签 */
    lv_label_set_text(label_audio, LV_SYMBOL_AUDIO);                                            /* 设置文本内容：音乐图标 */
    lv_obj_set_style_text_font(label_audio, font, LV_STATE_DEFAULT);                            /* 设置字体 */
    lv_obj_align_to(label_audio, slider_audio, LV_ALIGN_OUT_LEFT_MID, -scr_act_width()/40, 0);  /* 设置位置 */

    /* 闹钟音量滑块 */
    lv_obj_t *slider_bell = lv_slider_create(content);                                          /* 创建滑块 */
    lv_obj_set_size(slider_bell, scr_act_width()/3, scr_act_height()/30);                       /* 设置大小 */
    lv_obj_align(slider_bell, LV_ALIGN_CENTER, 15, scr_act_height()/14);                        /* 设置位置 */
    lv_slider_set_value(slider_bell, 50, LV_ANIM_OFF);                                          /* 设置当前值 */
    lv_obj_set_style_bg_color(slider_bell, lv_color_hex(0x787c78), LV_PART_MAIN);               /* 设置主体颜色 */
    lv_obj_set_style_bg_color(slider_bell, lv_color_hex(0xc3c3c3), LV_PART_INDICATOR);          /* 设置指示器颜色 */
    lv_obj_remove_style(slider_bell, NULL, LV_PART_KNOB);                                       /* 移除旋钮 */

    /* 闹钟音量图标 */
    lv_obj_t *label_bell = lv_label_create(content);                                            /* 创建音量标签 */
    lv_label_set_text(label_bell, LV_SYMBOL_BELL);                                              /* 设置文本内容：闹钟图标 */
    lv_obj_set_style_text_font(label_bell, font, LV_STATE_DEFAULT);                             /* 设置字体 */
    lv_obj_align_to(label_bell, slider_bell, LV_ALIGN_OUT_LEFT_MID, -scr_act_width()/40, 0);    /* 设置位置 */
}


// find lib/lvgl/lib -name "lib*.so*" -exec cp -a {} /usr/lib/ \;
// find lib/libjpeg-turbo/lib -name "lib*.so*" -exec cp -a {} /usr/lib/ \;
// find lib/rlottie/lib -name "lib*.so*" -exec cp -a {} /usr/lib/ \;
// find lib/FFmpeg/lib -name "lib*.so*" -exec cp -a {} /usr/lib/ \;

#if 0
#include <limits.h>
#include <errno.h>
#include <poll.h>

#include "lvgl/demos/lv_demos.h"

int main(int argc, char ** argv)
{
    struct pollfd pfd;
    uint32_t time_till_next;
    int sleep;

    lv_init();

    /*Linux display device init*/
    lv_linux_disp_init();

    /*Create a Demo*/
    //lv_demo_widgets();
    //lv_demo_widgets_start_slideshow();

    lv_example_win();
    lv_example_win_1();
    // lv_example_event_1();


    pfd.fd = lv_wayland_get_fd();
    pfd.events = POLLIN;

    while(1) {
        /* Handle any Wayland/LVGL timers/events */
        time_till_next = lv_wayland_timer_handler();

        /* Run until the last window closes */
        if (!lv_wayland_window_is_open(NULL)) {
            break;
        }

        /* Wait for something interesting to happen */
        if (time_till_next == LV_NO_TIMER_READY) {
            sleep = -1;
        } else if (time_till_next > INT_MAX) {
            sleep = INT_MAX;
        } else {
           sleep = time_till_next;
        }

        while ((poll(&pfd, 1, sleep) < 0) && (errno == EINTR));
    }
    return 0;
}
#endif

int main111(int argc, char ** argv)
{
    lv_init();

    /*Linux display device init*/
    lv_linux_disp_init();

    /*Create a Demo*/
    //lv_demo_widgets();
    //lv_demo_widgets_start_slideshow();
    // lv_example_event_1();

    lv_example_win();
    // lv_example_win_1();

    // while (1) usleep(5000);

    while (1) {
        // usleep(5000);

        /* Run until the last window closes */
        if (!lv_wayland_window_is_open(NULL)) {
            break;
        }

        bool ret = lv_wayland_timer_handler();
        // printf("%s: ret = %d\n", __func__, ret);
    }

    return 0;
}
#endif

