#include "ui/ui.h"
#include "ui/screens/ui_scr_desktop.h"
#include "ui/components/ui_comp_common.h"
#include "utils/prefs.h"
#include "utils/time_utils.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

static lv_timer_t * time_timer = NULL;

static void modal_close_event_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_DELETE) return;
    if (ui_menu_list) {
        lv_obj_clear_flag(ui_menu_list, LV_OBJ_FLAG_HIDDEN);
        lv_group_focus_obj(lv_obj_get_child(ui_menu_list, 0));
    }
}

static void reboot_msgbox_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * mbox = lv_event_get_current_target(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        const char * btn_text = lv_msgbox_get_active_btn_text(mbox);
        if (btn_text && strcmp(btn_text, "Confirm") == 0) {
            system("reboot");
        } else {
            lv_msgbox_close(mbox);
        }
    }
}

static void main_menu_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if (code == LV_EVENT_CLICKED) {
        const char * text = lv_list_get_btn_text(ui_menu_list, obj);
        if (strcmp(text, "About") == 0) {
            lv_obj_add_flag(ui_menu_list, LV_OBJ_FLAG_HIDDEN);
            ui_open_about_screen();
        } else if (strcmp(text, "Reboot") == 0) {
            lv_obj_add_flag(ui_menu_list, LV_OBJ_FLAG_HIDDEN);
            static const char * btns[] = {"Confirm", ""};
            lv_obj_t * mbox = lv_msgbox_create(lv_scr_act(), "Reboot", "Reboot system?", btns, true);
            lv_obj_set_width(mbox, 260);
            lv_obj_add_event_cb(mbox, reboot_msgbox_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
            lv_obj_add_event_cb(mbox, modal_close_event_cb, LV_EVENT_DELETE, NULL);
            lv_obj_center(mbox);
            lv_obj_t * btns_obj = lv_msgbox_get_btns(mbox);
            if (btns_obj) {
                lv_group_focus_obj(btns_obj);
            }
        } else if (strcmp(text, "Console") == 0) {
            ui_open_console_screen();
        } else if (strcmp(text, "Settings") == 0) {
            lv_obj_add_flag(ui_menu_list, LV_OBJ_FLAG_HIDDEN);
            ui_open_settings_screen();
        } else if (strcmp(text, "Meow RPG") == 0) {
            ui_open_game_screen();
        } else if (strcmp(text, "NES Emulator") == 0) {
            lv_obj_add_flag(ui_menu_list, LV_OBJ_FLAG_HIDDEN);
            ui_open_nes_browser();
        } else if (strcmp(text, "Stella") == 0) {
            lv_obj_add_flag(ui_menu_list, LV_OBJ_FLAG_HIDDEN);
            ui_open_stella_browser();
        }
    }
}

void ui_refresh_time(void)
{
    if (!ui_time_label) return;
    char time_str[16];
    util_format_current_time(time_str, sizeof(time_str), util_show_seconds, util_is_24_hour_format);
    lv_label_set_text(ui_time_label, time_str);
}

static void time_update_task(lv_timer_t * timer)
{
    ui_refresh_time();
}

static void create_main_menu(lv_obj_t * parent, lv_group_t * g)
{
    ui_menu_list = lv_list_create(parent);
    lv_obj_set_size(ui_menu_list, 280, 200);
    lv_obj_align(ui_menu_list, LV_ALIGN_BOTTOM_MID, 0, -5);

    const char * menu_items[] = {"Meow RPG", "NES Emulator", "Stella", "Console", "Settings", "About", "Reboot"};
    for (int i = 0; i < (int)(sizeof(menu_items) / sizeof(menu_items[0])); i++) {
        lv_obj_t * btn = ui_comp_create_styled_list_btn(ui_menu_list, menu_items[i]);
        lv_obj_add_event_cb(btn, main_menu_event_handler, LV_EVENT_CLICKED, NULL);
        lv_group_add_obj(g, btn);
    }
}

void ui_create_desktop(void)
{
    lv_obj_t * screen = lv_scr_act();
    ui_time_label = lv_label_create(screen);
    lv_obj_set_style_text_font(ui_time_label, &nes_font_16, 0);
    lv_obj_align(ui_time_label, LV_ALIGN_TOP_RIGHT, -8, 8);

    ui_refresh_time();
    time_timer = lv_timer_create(time_update_task, 1000, NULL);

    create_main_menu(screen, lv_group_get_default());
}
