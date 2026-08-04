#ifndef UI_STATUS_BAR_H
#define UI_STATUS_BAR_H

#include "lvgl/lvgl.h"

void ui_status_bar_init(lv_obj_t * parent);
void ui_status_bar_set_headphone(bool on);

#endif // UI_STATUS_BAR_H
