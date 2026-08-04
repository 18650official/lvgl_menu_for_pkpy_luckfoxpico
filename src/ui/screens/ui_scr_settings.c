#include "ui/ui.h"
#include "ui/components/ui_comp_common.h"
#include "utils/prefs.h"
#include "utils/time_utils.h"
#include <stdlib.h>
#include <string.h>

static lv_obj_t * settings_screen = NULL;
static lv_obj_t * time_settings_screen = NULL;

static void ui_open_time_settings_screen(void);
static void ui_open_time_setter_page(void);
static void sub_page_close_event_cb(lv_event_t * e);
static int edit_hour;
static int edit_minute;
static lv_obj_t * time_setter_hour_label;
static lv_obj_t * time_setter_minute_label;

static void settings_screen_close_cb(lv_event_t * e);
static void time_settings_screen_close_cb(lv_event_t * e);
static void sub_page_close_event_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_DELETE) return;
    if (time_settings_screen) {
        lv_obj_clear_flag(time_settings_screen, LV_OBJ_FLAG_HIDDEN);
        lv_obj_t * first_button = lv_obj_get_child(time_settings_screen, 1);
        if (!first_button) {
            first_button = lv_obj_get_child(time_settings_screen, 0);
        }
        if (first_button) {
            lv_group_focus_obj(first_button);
        }
    }
}

static void settings_screen_close_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_DELETE) return;
    if (ui_menu_list) {
        lv_obj_clear_flag(ui_menu_list, LV_OBJ_FLAG_HIDDEN);
        lv_group_focus_obj(lv_obj_get_child(ui_menu_list, 0));
    }
}

static void time_settings_screen_close_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_DELETE) return;
    if (settings_screen) {
        lv_obj_clear_flag(settings_screen, LV_OBJ_FLAG_HIDDEN);
        lv_group_focus_obj(lv_obj_get_child(settings_screen, 0));
    }
}

static void time_value_adjust_event_cb(lv_event_t * e)
{
    lv_obj_t * label = lv_event_get_user_data(e);
    uint32_t key = lv_indev_get_key(lv_indev_get_act());
    bool is_hour = (label == time_setter_hour_label);

    if (key == LV_KEY_UP) {
        if (is_hour) edit_hour = (edit_hour + 1) % 24;
        else edit_minute = (edit_minute + 1) % 60;
    } else if (key == LV_KEY_DOWN) {
        if (is_hour) edit_hour = (edit_hour - 1 + 24) % 24;
        else edit_minute = (edit_minute - 1 + 60) % 60;
    }

    if (is_hour) {
        lv_label_set_text_fmt(label, "%02d", edit_hour);
    } else {
        lv_label_set_text_fmt(label, "%02d", edit_minute);
    }
}

static void time_save_event_cb(lv_event_t * e)
{
    lv_obj_t * page = lv_event_get_user_data(e);
    util_set_system_time(edit_hour, edit_minute);
    ui_refresh_time();
    if (page) {
        lv_obj_del(page);
    }
}

static void show_seconds_event_cb(lv_event_t * e)
{
    lv_obj_t * btn = lv_event_get_target(e);
    lv_obj_t * page = lv_event_get_user_data(e);
    const char * text = lv_list_get_btn_text(lv_obj_get_parent(btn), btn);
    util_show_seconds = (strcmp(text, "On") == 0);
    prefs_save();
    ui_refresh_time();
    if (page) {
        lv_obj_del(page);
    }
}

static void hour_format_event_cb(lv_event_t * e)
{
    lv_obj_t * btn = lv_event_get_target(e);
    lv_obj_t * page = lv_event_get_user_data(e);
    const char * text = lv_list_get_btn_text(lv_obj_get_parent(btn), btn);
    util_is_24_hour_format = (strcmp(text, "24 Hour") == 0);
    prefs_save();
    ui_refresh_time();
    if (page) {
        lv_obj_del(page);
    }
}

static void settings_menu_event_handler(lv_event_t * e)
{
    lv_obj_t * obj = lv_event_get_target(e);
    const char * text = lv_list_get_btn_text(settings_screen, obj);
    if (strcmp(text, "Time Settings") == 0) {
        lv_obj_add_flag(settings_screen, LV_OBJ_FLAG_HIDDEN);
        ui_open_time_settings_screen();
    } else if (strcmp(text, "Back") == 0) {
        lv_obj_del(settings_screen);
    }
}

static void time_settings_menu_event_handler(lv_event_t * e)
{
    lv_obj_t * obj = lv_event_get_target(e);
    const char * text = lv_list_get_btn_text(time_settings_screen, obj);
    if (strcmp(text, "Set time") == 0) {
        ui_open_time_setter_page();
    } else if (strcmp(text, "Second display") == 0) {
        static const char* opts[] = {"On", "Off", NULL};
        ui_comp_create_generic_option_page("Show Seconds", opts, show_seconds_event_cb, time_settings_screen, sub_page_close_event_cb);
    } else if (strcmp(text, "12/24 Hour format") == 0) {
        static const char* opts[] = {"24 Hour", "12 Hour", NULL};
        ui_comp_create_generic_option_page("Time Format", opts, hour_format_event_cb, time_settings_screen, sub_page_close_event_cb);
    } else if (strcmp(text, "Back") == 0) {
        lv_obj_del(time_settings_screen);
    }
}

void ui_open_time_setter_page(void)
{
    lv_obj_add_flag(time_settings_screen, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t * page = lv_obj_create(lv_scr_act());
    lv_obj_set_size(page, LV_HOR_RES, LV_VER_RES);
    lv_obj_add_event_cb(page, sub_page_close_event_cb, LV_EVENT_DELETE, NULL);
    lv_obj_set_style_pad_all(page, 0, 0);

    util_get_local_time(&edit_hour, &edit_minute);

    lv_obj_t * container = lv_obj_create(page);
    lv_obj_center(container);
    lv_obj_set_size(container, 280, 140);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(container, 10, 0);

    lv_obj_t * hour_obj = lv_obj_create(container);
    lv_obj_set_size(hour_obj, 80, 70);
    lv_obj_set_style_bg_color(hour_obj, lv_color_hex(0x404040), 0);
    lv_obj_set_style_bg_color(hour_obj, lv_color_hex(0x5070a0), LV_STATE_FOCUSED);
    lv_obj_set_scrollbar_mode(hour_obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_border_width(hour_obj, 0, 0);

    time_setter_hour_label = lv_label_create(hour_obj);
    lv_label_set_text_fmt(time_setter_hour_label, "%02d", edit_hour);
    lv_obj_set_style_text_font(time_setter_hour_label, &lv_font_montserrat_36, 0);
    lv_obj_set_style_text_color(time_setter_hour_label, lv_color_white(), 0);
    lv_obj_center(time_setter_hour_label);
    lv_obj_add_event_cb(hour_obj, time_value_adjust_event_cb, LV_EVENT_KEY, time_setter_hour_label);

    lv_obj_t * sep_label = lv_label_create(container);
    lv_label_set_text(sep_label, ":");
    lv_obj_set_style_text_font(sep_label, &lv_font_montserrat_36, 0);
    lv_obj_set_style_text_color(sep_label, lv_color_white(), 0);

    lv_obj_t * minute_obj = lv_obj_create(container);
    lv_obj_set_size(minute_obj, 80, 70);
    lv_obj_set_style_bg_color(minute_obj, lv_color_hex(0x404040), 0);
    lv_obj_set_style_bg_color(minute_obj, lv_color_hex(0x5070a0), LV_STATE_FOCUSED);
    lv_obj_set_scrollbar_mode(minute_obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_border_width(minute_obj, 0, 0);

    time_setter_minute_label = lv_label_create(minute_obj);
    lv_label_set_text_fmt(time_setter_minute_label, "%02d", edit_minute);
    lv_obj_set_style_text_font(time_setter_minute_label, &lv_font_montserrat_36, 0);
    lv_obj_set_style_text_color(time_setter_minute_label, lv_color_white(), 0);
    lv_obj_center(time_setter_minute_label);
    lv_obj_add_event_cb(minute_obj, time_value_adjust_event_cb, LV_EVENT_KEY, time_setter_minute_label);

    lv_obj_t * save_btn = lv_btn_create(page);
    lv_obj_set_height(save_btn, 30);
    lv_obj_align(save_btn, LV_ALIGN_BOTTOM_LEFT, 20, -10);
    lv_obj_t * save_label = lv_label_create(save_btn);
    lv_label_set_text(save_label, "Save");
    lv_obj_set_style_text_font(save_label, &lv_font_montserrat_12, 0);
    lv_obj_add_event_cb(save_btn, time_save_event_cb, LV_EVENT_CLICKED, page);

    lv_obj_t * back_btn = lv_btn_create(page);
    lv_obj_set_height(back_btn, 30);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_RIGHT, -20, -10);
    lv_obj_t * back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back");
    lv_obj_set_style_text_font(back_label, &lv_font_montserrat_12, 0);
    lv_obj_add_event_cb(back_btn, ui_comp_generic_delete_event_cb, LV_EVENT_CLICKED, page);

    lv_group_t* g = lv_group_get_default();
    lv_group_add_obj(g, hour_obj);
    lv_group_add_obj(g, minute_obj);
    lv_group_add_obj(g, save_btn);
    lv_group_add_obj(g, back_btn);
    lv_group_focus_obj(hour_obj);
}

void ui_open_time_settings_screen(void)
{
    time_settings_screen = lv_list_create(lv_scr_act());
    lv_obj_set_size(time_settings_screen, 280, 190);
    lv_obj_align(time_settings_screen, LV_ALIGN_CENTER, 0, 10);
    lv_obj_add_event_cb(time_settings_screen, time_settings_screen_close_cb, LV_EVENT_DELETE, NULL);
    const char * items[] = {"Set time", "Second display", "12/24 Hour format", "Back"};
    for (int i = 0; i < 4; i++) {
        lv_obj_t * btn = ui_comp_create_styled_list_btn(time_settings_screen, items[i]);
        lv_obj_add_event_cb(btn, time_settings_menu_event_handler, LV_EVENT_CLICKED, NULL);
        lv_group_add_obj(lv_group_get_default(), btn);
    }
    lv_group_focus_obj(lv_obj_get_child(time_settings_screen, 0));
}

void ui_open_settings_screen(void)
{
    settings_screen = lv_list_create(lv_scr_act());
    lv_obj_set_size(settings_screen, 280, 190);
    lv_obj_align(settings_screen, LV_ALIGN_CENTER, 0, 10);
    lv_obj_add_event_cb(settings_screen, settings_screen_close_cb, LV_EVENT_DELETE, NULL);

    const char * items[] = {"Time Settings", "Back"};
    for (int i = 0; i < 2; i++) {
        lv_obj_t * btn = ui_comp_create_styled_list_btn(settings_screen, items[i]);
        lv_obj_add_event_cb(btn, settings_menu_event_handler, LV_EVENT_CLICKED, NULL);
        lv_group_add_obj(lv_group_get_default(), btn);
    }
    lv_group_focus_obj(lv_obj_get_child(settings_screen, 0));
}
