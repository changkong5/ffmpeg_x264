#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <lvgl/lvgl.h>

#include "initcall.h"

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

static int lv_example_win_1(void)
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

    return 0;
}
// fn_initcall(lv_example_win_1);




















