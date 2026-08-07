#define _DEFAULT_SOURCE
#include "ui/ui.h"
#include "ui/components/ui_comp_common.h"
#include "ui/components/ui_status_bar.h"
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

// Music
static lv_obj_t * music_browser_screen = NULL;
static lv_obj_t * music_player_screen = NULL;
static lv_obj_t * music_progress_bar = NULL;
static lv_timer_t * music_fake_timer = NULL;
static bool is_music_paused = false;

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

// Music player logic
static void music_pause_resume_cb(lv_event_t * e)
{
    lv_obj_t * btn = lv_event_get_target(e);
    lv_obj_t * label = lv_obj_get_child(btn, 0);

    if (!is_music_paused) {
        // 暂停进程
        system("killall -STOP mpg123 aplay 2>/dev/null");
        lv_label_set_text(label, "Play");
        is_music_paused = true;
    } else {
        // 恢复进程
        system("killall -CONT mpg123 aplay 2>/dev/null");
        lv_label_set_text(label, "Pause");
        is_music_paused = false;
    }
}

static void music_close_cb(lv_event_t * e)
{
    // 彻底杀掉播放进程
    system("killall -9 mpg123 aplay 2>/dev/null"); 
    
    if (music_fake_timer) {
        lv_timer_del(music_fake_timer);
        music_fake_timer = NULL;
    }
    if (music_player_screen) {
        lv_obj_del(music_player_screen);
        music_player_screen = NULL;
    }
    // 恢复显示音乐列表
    if (music_browser_screen) {
        lv_obj_clear_flag(music_browser_screen, LV_OBJ_FLAG_HIDDEN);
        lv_group_focus_obj(lv_obj_get_child(music_browser_screen, 0)); // 聚焦返回按钮
    }
}

// 因为不借助外部库很难获取准确的音频总时长，这里做一个循环跑动的假进度条，表示正在播放
static void music_progress_timer_cb(lv_timer_t * t)
{
    if (is_music_paused || !music_progress_bar) return;
    int32_t val = lv_bar_get_value(music_progress_bar);
    val += 2; 
    if (val > 100) val = 0;
    lv_bar_set_value(music_progress_bar, val, LV_ANIM_ON);
}

static void music_play_event_handler(lv_event_t * e)
{
    const char * filename = lv_event_get_user_data(e);
    if (!filename) return;

    // 播放前先清理残留进程
    system("killall -9 mpg123 aplay 2>/dev/null");

    char command[512];
    if (strstr(filename, ".mp3")) {
        snprintf(command, sizeof(command), 
                 "/usr/bin/mpg123 -q -o alsa -a plughw:1,0 \"/oem/usr/share/audio/%s\" < /dev/null > /dev/null 2>&1 &", 
                 filename);
    } else if (strstr(filename, ".wav")) {
        snprintf(command, sizeof(command), 
                 "/usr/bin/aplay -q -D plughw:1,0 \"/oem/usr/share/audio/%s\" < /dev/null > /dev/null 2>&1 &", 
                 filename);
    } else {
        return;
    }

    system(command);
    is_music_paused = false;

    // 隐藏列表，准备显示播放器 UI
    if (music_browser_screen) lv_obj_add_flag(music_browser_screen, LV_OBJ_FLAG_HIDDEN);

    music_player_screen = lv_obj_create(lv_scr_act());
    lv_obj_set_size(music_player_screen, 240, 160); // 简约的居中小窗
    lv_obj_center(music_player_screen);
    lv_obj_set_style_border_width(music_player_screen, 2, 0);

    // 滚动显示的文件名
    lv_obj_t * title = lv_label_create(music_player_screen);
    lv_label_set_text(title, filename);
    lv_obj_set_style_text_font(title, &nes_font_16, 0); // 使用你的中文字体
    lv_obj_set_width(title, 200);
    lv_label_set_long_mode(title, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // 假进度条（仅起视觉动效作用）
    music_progress_bar = lv_bar_create(music_player_screen);
    lv_obj_set_size(music_progress_bar, 200, 10);
    lv_obj_align(music_progress_bar, LV_ALIGN_CENTER, 0, -10);
    lv_bar_set_range(music_progress_bar, 0, 100);
    lv_bar_set_value(music_progress_bar, 0, LV_ANIM_OFF);

    // 暂停按钮
    lv_obj_t * btn_pause = lv_btn_create(music_player_screen);
    lv_obj_set_size(btn_pause, 80, 30);
    lv_obj_align(btn_pause, LV_ALIGN_BOTTOM_LEFT, 15, -10);
    lv_obj_add_event_cb(btn_pause, music_pause_resume_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * lbl_pause = lv_label_create(btn_pause);
    lv_label_set_text(lbl_pause, "Pause");
    lv_obj_center(lbl_pause);

    // 关闭按钮
    lv_obj_t * btn_close = lv_btn_create(music_player_screen);
    lv_obj_set_size(btn_close, 80, 30);
    lv_obj_align(btn_close, LV_ALIGN_BOTTOM_RIGHT, -15, -10);
    lv_obj_add_event_cb(btn_close, music_close_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * lbl_close = lv_label_create(btn_close);
    lv_label_set_text(lbl_close, "Close");
    lv_obj_center(lbl_close);

    // 设置焦点，支持手柄控制
    lv_group_t * g = lv_group_get_default();
    lv_group_add_obj(g, btn_pause);
    lv_group_add_obj(g, btn_close);
    lv_group_focus_obj(btn_pause);

    music_fake_timer = lv_timer_create(music_progress_timer_cb, 500, NULL);
}

static void music_browser_close_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_DELETE && ui_menu_list) {
        lv_obj_clear_flag(ui_menu_list, LV_OBJ_FLAG_HIDDEN);
        lv_group_focus_obj(lv_obj_get_child(ui_menu_list, 0)); // 聚焦到桌面的首个选项
    }
}

// 弹出提示框后的关闭回调
static void headset_msgbox_close_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * mbox = lv_event_get_current_target(e);
    
    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_msgbox_close(mbox);
    } else if (code == LV_EVENT_DELETE) {
        if (ui_menu_list) {
            lv_obj_clear_flag(ui_menu_list, LV_OBJ_FLAG_HIDDEN);
            lv_group_focus_obj(lv_obj_get_child(ui_menu_list, 0));
        }
    }
}

void ui_open_music_browser(void)
{
    // 如果没有插入耳机，拒绝打开并弹窗
    if (!detect_usb_headset()) {
        static const char * btns[] = {"OK", ""};
        lv_obj_t * mbox = lv_msgbox_create(lv_scr_act(), "Warning", "Please plug in the headset", btns, true);
        lv_obj_center(mbox);
        lv_obj_add_event_cb(mbox, headset_msgbox_close_cb, LV_EVENT_VALUE_CHANGED, NULL);
        lv_obj_add_event_cb(mbox, headset_msgbox_close_cb, LV_EVENT_DELETE, NULL);
        
        lv_obj_t * btns_obj = lv_msgbox_get_btns(mbox);
        if (btns_obj) lv_group_focus_obj(btns_obj);
        return;
    }

    music_browser_screen = lv_list_create(lv_scr_act());
    lv_obj_add_style(music_browser_screen, &ui_comp_style_nes_cjk, 0); // 复用你的样式
    lv_obj_set_size(music_browser_screen, 280, 190);
    lv_obj_align(music_browser_screen, LV_ALIGN_CENTER, 0, 10);
    lv_obj_add_event_cb(music_browser_screen, music_browser_close_cb, LV_EVENT_DELETE, NULL);

    lv_group_t * g = lv_group_get_default();
    lv_obj_t * btn_back = ui_comp_create_nes_list_btn(music_browser_screen, "Back");
    lv_obj_add_event_cb(btn_back, ui_comp_generic_delete_event_cb, LV_EVENT_CLICKED, music_browser_screen);
    lv_group_add_obj(g, btn_back);

    const char * dir_path = "/oem/usr/share/audio";
    DIR *d = opendir(dir_path);
    if (d) {
        struct dirent *dir;
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type == DT_REG) {
                // 仅筛选 .mp3 和 .wav 文件
                if (strstr(dir->d_name, ".mp3") || strstr(dir->d_name, ".wav")) {
                    char * filename_copy = strdup(dir->d_name);
                    if (filename_copy) {
                        lv_obj_t * btn_music = lv_list_add_btn(music_browser_screen, LV_SYMBOL_AUDIO, filename_copy);
                        // 强制应用中文字体
                        lv_obj_set_style_text_font(btn_music, &nes_font_16, 0); 
                        lv_obj_add_event_cb(btn_music, music_play_event_handler, LV_EVENT_CLICKED, filename_copy);
                        lv_group_add_obj(g, btn_music);
                    }
                }
            }
        }
        closedir(d);
    } else {
        lv_list_add_text(music_browser_screen, "Error: Cannot open dir");
    }
    lv_group_focus_obj(btn_back);
}