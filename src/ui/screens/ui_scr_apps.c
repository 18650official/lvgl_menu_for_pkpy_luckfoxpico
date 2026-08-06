#define _DEFAULT_SOURCE
#include "ui/ui.h"
#include "ui/components/ui_comp_common.h"
#include "utils/file_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>

static lv_obj_t * about_screen = NULL;
static lv_obj_t * console_screen = NULL;
static lv_obj_t * nes_browser_screen = NULL;
static lv_obj_t * stella_browser_screen = NULL;

static void modal_close_event_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_DELETE) return;
    if (ui_menu_list) {
        lv_obj_clear_flag(ui_menu_list, LV_OBJ_FLAG_HIDDEN);
        lv_group_focus_obj(lv_obj_get_child(ui_menu_list, 0));
    }
}

static void console_exit_event_handler(lv_event_t * e)
{
    system("/oem/usr/etc/init.d/S98fbterm stop");
    if (console_screen) {
        lv_obj_del(console_screen);
        console_screen = NULL;
    }
    if (ui_menu_list) {
        lv_obj_clear_flag(ui_menu_list, LV_OBJ_FLAG_HIDDEN);
        lv_obj_t * console_btn = lv_obj_get_child(ui_menu_list, 3);
        if (console_btn) lv_group_focus_obj(console_btn);
    }
    if (ui_time_label) lv_obj_clear_flag(ui_time_label, LV_OBJ_FLAG_HIDDEN);
    lv_refr_now(lv_disp_get_default());
}

static void nes_browser_screen_close_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_DELETE && ui_menu_list) {
        lv_obj_clear_flag(ui_menu_list, LV_OBJ_FLAG_HIDDEN);
        lv_group_focus_obj(lv_obj_get_child(ui_menu_list, 1));
    }
}

static void stella_browser_screen_close_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_DELETE && ui_menu_list) {
        lv_obj_clear_flag(ui_menu_list, LV_OBJ_FLAG_HIDDEN);
        lv_group_focus_obj(lv_obj_get_child(ui_menu_list, 2));
    }
}

static void nes_game_launch_event_handler(lv_event_t * e)
{
    const char * filename = lv_event_get_user_data(e);
    if (!filename) return;

    char command[512];
    snprintf(command, sizeof(command), "/oem/lv_execute/nes_start.sh \"/oem/nes_games/%s\" &", filename);

    if (ui_menu_list) lv_obj_add_flag(ui_menu_list, LV_OBJ_FLAG_HIDDEN);
    if (ui_time_label) lv_obj_add_flag(ui_time_label, LV_OBJ_FLAG_HIDDEN);
    if (nes_browser_screen) lv_obj_add_flag(nes_browser_screen, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t * transition_screen = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(transition_screen);
    lv_obj_set_size(transition_screen, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(transition_screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(transition_screen, LV_OPA_COVER, 0);

    lv_timer_handler();
    usleep(16000);
    system(command);
}

static void stella_game_launch_event_handler(lv_event_t * e)
{
    const char * filename = lv_event_get_user_data(e);
    if (!filename) return;

    char command[512];
    snprintf(command, sizeof(command), "/oem/lv_execute/stella_start.sh \"/oem/atari_games/%s\" &", filename);

    if (ui_menu_list) lv_obj_add_flag(ui_menu_list, LV_OBJ_FLAG_HIDDEN);
    if (ui_time_label) lv_obj_add_flag(ui_time_label, LV_OBJ_FLAG_HIDDEN);
    if (stella_browser_screen) lv_obj_add_flag(stella_browser_screen, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t * transition_screen = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(transition_screen);
    lv_obj_set_size(transition_screen, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(transition_screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(transition_screen, LV_OPA_COVER, 0);

    lv_timer_handler();
    usleep(16000);
    system(command);
}

static void create_nes_browser_screen(lv_obj_t * parent)
{
    nes_browser_screen = lv_list_create(parent);
    lv_obj_add_style(nes_browser_screen, &ui_comp_style_nes_cjk, 0);
    lv_obj_set_size(nes_browser_screen, 280, 190);
    lv_obj_align(nes_browser_screen, LV_ALIGN_CENTER, 0, 10);
    lv_obj_add_event_cb(nes_browser_screen, nes_browser_screen_close_cb, LV_EVENT_DELETE, NULL);

    lv_group_t * g = lv_group_get_default();
    lv_obj_t * btn_back = ui_comp_create_nes_list_btn(nes_browser_screen, "Back");
    lv_obj_add_event_cb(btn_back, ui_comp_generic_delete_event_cb, LV_EVENT_CLICKED, nes_browser_screen);
    lv_group_add_obj(g, btn_back);

    const char * dir_path = "/oem/nes_games";
    DIR *d = opendir(dir_path);
    if (d) {
        struct dirent *dir;
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type == DT_REG) {
                char * filename_copy = strdup(dir->d_name);
                if (filename_copy) {
                    lv_obj_t * btn_game = ui_comp_create_nes_list_btn(nes_browser_screen, filename_copy);
                    lv_obj_add_event_cb(btn_game, nes_game_launch_event_handler, LV_EVENT_CLICKED, filename_copy);
                    lv_group_add_obj(g, btn_game);
                }
            }
        }
        closedir(d);
    } else {
        lv_list_add_text(nes_browser_screen, "Error: Cannot open dir");
    }
    lv_group_focus_obj(btn_back);
}

static void create_stella_browser_screen(lv_obj_t * parent)
{
    stella_browser_screen = lv_list_create(parent);
    lv_obj_add_style(stella_browser_screen, &ui_comp_style_nes_cjk, 0);
    lv_obj_set_size(stella_browser_screen, 280, 190);
    lv_obj_align(stella_browser_screen, LV_ALIGN_CENTER, 0, 10);
    lv_obj_add_event_cb(stella_browser_screen, stella_browser_screen_close_cb, LV_EVENT_DELETE, NULL);

    lv_group_t * g = lv_group_get_default();
    lv_obj_t * btn_back = ui_comp_create_nes_list_btn(stella_browser_screen, "Back");
    lv_obj_add_event_cb(btn_back, ui_comp_generic_delete_event_cb, LV_EVENT_CLICKED, stella_browser_screen);
    lv_group_add_obj(g, btn_back);

    const char * dir_path = "/oem/atari_games";
    DIR *d = opendir(dir_path);
    if (d) {
        struct dirent *dir;
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type == DT_REG) {
                char * filename_copy = strdup(dir->d_name);
                if (filename_copy) {
                    lv_obj_t * btn_game = ui_comp_create_nes_list_btn(stella_browser_screen, filename_copy);
                    lv_obj_add_event_cb(btn_game, stella_game_launch_event_handler, LV_EVENT_CLICKED, filename_copy);
                    lv_group_add_obj(g, btn_game);
                }
            }
        }
        closedir(d);
    } else {
        lv_list_add_text(stella_browser_screen, "Error: Cannot open dir");
    }
    lv_group_focus_obj(btn_back);
}

void ui_open_about_screen(void)
{
    about_screen = lv_obj_create(lv_scr_act());
    lv_obj_set_size(about_screen, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_border_width(about_screen, 0, 0);
    lv_obj_set_style_pad_all(about_screen, 5, 0);
    lv_obj_add_event_cb(about_screen, modal_close_event_cb, LV_EVENT_DELETE, NULL);

    char buffer[512];
    long mem_total = 0, mem_available = 0;
    FILE * fp = fopen("/proc/meminfo", "r");
    if (fp) {
        char line[128];
        while (fgets(line, sizeof(line), fp)) {
            if (sscanf(line, "MemTotal: %ld kB", &mem_total) == 1) {}
            if (sscanf(line, "MemAvailable: %ld kB", &mem_available) == 1) {}
        }
        fclose(fp);
    }

    char file_buffer[256] = "Reading...";
    read_file_to_string("/oem/.mkconsole_info", file_buffer, sizeof(file_buffer));
    snprintf(buffer, sizeof(buffer),
             "Device: Miku Console 2026\n"
             "RAM: %ld / %ld MB\n\n"
             "Ver:\n%s\n"
             "Dev: Snowmiku",
             mem_total / 1024, mem_available / 1024,
             file_buffer);

    lv_obj_t * about_label = lv_label_create(about_screen);
    lv_label_set_text(about_label, buffer);
    lv_obj_set_style_text_font(about_label, &lv_font_montserrat_12, 0);
    lv_obj_set_width(about_label, 280);
    lv_obj_align(about_label, LV_ALIGN_TOP_LEFT, 10, 20);

    lv_obj_t * back_btn = lv_btn_create(about_screen);
    lv_obj_set_height(back_btn, 24);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_add_event_cb(back_btn, ui_comp_generic_delete_event_cb, LV_EVENT_CLICKED, about_screen);

    lv_obj_t * back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back");
    lv_obj_set_style_text_font(back_label, &lv_font_montserrat_12, 0);
    lv_obj_center(back_label);

    lv_group_add_obj(lv_group_get_default(), back_btn);
    lv_group_focus_obj(back_btn);
}

void ui_open_console_screen(void)
{
    lv_obj_add_flag(ui_menu_list, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_time_label, LV_OBJ_FLAG_HIDDEN);

    console_screen = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(console_screen);
    lv_obj_set_size(console_screen, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(console_screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(console_screen, LV_OPA_COVER, 0);

    lv_obj_t * exit_btn = lv_btn_create(console_screen);
    lv_obj_align(exit_btn, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_add_event_cb(exit_btn, console_exit_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(exit_btn, lv_color_hex(0x404040), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(exit_btn, lv_color_hex(0x5070a0), LV_STATE_FOCUSED);
    lv_obj_set_height(exit_btn, 24);

    lv_obj_t * exit_label = lv_label_create(exit_btn);
    lv_label_set_text(exit_label, "Exit");
    lv_obj_set_style_text_font(exit_label, &lv_font_montserrat_12, 0);
    lv_obj_center(exit_label);
    lv_obj_set_style_text_color(exit_label, lv_color_hex(0xffffff), 0);

    lv_group_add_obj(lv_group_get_default(), exit_btn);
    lv_group_focus_obj(exit_btn);

    lv_timer_handler();
    usleep(16000);
    system("/oem/usr/etc/init.d/S98fbterm start_with_input &");
}

void ui_open_game_screen(void)
{
    lv_obj_add_flag(ui_menu_list, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_time_label, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t * game_screen = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(game_screen);
    lv_obj_set_size(game_screen, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(game_screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(game_screen, LV_OPA_COVER, 0);

    lv_timer_handler();
    usleep(16000);
    system("/oem/lv_execute/term_start_all.sh < /dev/null &");
}

void ui_open_joystick_screen(void)
{
    lv_obj_add_flag(ui_menu_list, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_time_label, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t * joystick_screen = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(joystick_screen);
    lv_obj_set_size(joystick_screen, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(joystick_screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(joystick_screen, LV_OPA_COVER, 0);

    lv_timer_handler();
    usleep(16000);
    system("/oem/lv_execute/joystick_start_all.sh < /dev/null &");
}

void ui_open_nes_browser(void)
{
    create_nes_browser_screen(lv_scr_act());
}

void ui_open_stella_browser(void)
{
    create_stella_browser_screen(lv_scr_act());
}
