#ifndef UI_COMP_COMMON_H
#define UI_COMP_COMMON_H

#include "lvgl/lvgl.h"

extern lv_style_t ui_comp_style_nes_cjk;

LV_FONT_DECLARE(nes_font_16);


void ui_comp_init_styles(void);
lv_obj_t * ui_comp_create_styled_list_btn(lv_obj_t * parent, const char * text);
lv_obj_t * ui_comp_create_nes_list_btn(lv_obj_t * parent, const char * text);
void ui_comp_create_generic_option_page(const char* title, const char** options, lv_event_cb_t event_cb, lv_obj_t* parent_to_hide, lv_event_cb_t close_cb);
void ui_comp_generic_delete_event_cb(lv_event_t * e);
bool detect_usb_headset(void);

#endif // UI_COMP_COMMON_H
