#ifndef UI_STATUS_BAR_H
#define UI_STATUS_BAR_H

#include "lvgl/lvgl.h"

void ui_status_bar_init(lv_obj_t * parent);

/* 如果外部需要手动控制状态图标，可以调用这两个接口 */
void ui_status_bar_set_headphone(bool on);
void ui_status_bar_set_sdcard(bool on);

#endif // UI_STATUS_BAR_H