#ifndef UI_UI_H
#define UI_UI_H

#include "lvgl/lvgl.h"

extern lv_obj_t * ui_time_label;
extern lv_obj_t * ui_menu_list;

void ui_backend_init(void);
void ui_init(void);
void ui_refresh_time(void);
void ui_open_about_screen(void);
void ui_open_settings_screen(void);
void ui_open_console_screen(void);
void ui_open_game_screen(void);
void ui_open_joystick_screen(void);
void ui_open_nes_browser(void);
void ui_open_stella_browser(void);

#endif // UI_UI_H
