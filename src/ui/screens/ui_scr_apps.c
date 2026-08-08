#define _DEFAULT_SOURCE
#define _GNU_SOURCE

/* 先包含 ALSA，避免标准库把 timeval 提前定义导致冲突 */
#include <alsa/asoundlib.h>
#include <mpg123.h>
#include <pthread.h>

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
// static bool is_music_paused = false;

static pthread_t audio_thread;
static volatile bool audio_thread_exit = false;
static volatile bool audio_is_paused = false;
static volatile int audio_seek_request = -1; // 0-100 进度百分比，-1 表示无请求
static double audio_current_time = 0.0;
static double audio_total_time = 0.0;
static char current_audio_path[256];

// ALSA 播放句柄
static snd_pcm_t *pcm_handle = NULL;

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

// 初始化 ALSA 声卡 (注意这里使用了你的 plughw:1,0)
static int alsa_init(uint32_t rate, uint32_t channels) {
    int err;
    if ((err = snd_pcm_open(&pcm_handle, "plughw:1,0", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        printf("Cannot open ALSA device: %s\n", snd_strerror(err));
        return -1;
    }
    snd_pcm_set_params(pcm_handle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, channels, rate, 1, 100000);
    return 0;
}

static void alsa_close() {
    if (pcm_handle) {
        snd_pcm_drop(pcm_handle);
        snd_pcm_close(pcm_handle);
        pcm_handle = NULL;
    }
}

// 独立的音频播放线程
static void * audio_playback_thread(void * arg) {
    bool is_mp3 = strstr(current_audio_path, ".mp3") != NULL;
    bool is_wav = strstr(current_audio_path, ".wav") != NULL;
    
    // ================= MP3 初始化 =================
    mpg123_handle *m = NULL;
    if (is_mp3) {
        mpg123_init();
        m = mpg123_new(NULL, NULL);
        if (mpg123_open(m, current_audio_path) == MPG123_OK) {
            long rate; int channels, encoding;
            mpg123_getformat(m, &rate, &channels, &encoding);
            alsa_init(rate, channels);
            
            off_t total_samples = mpg123_length(m);
            audio_total_time = (double)total_samples / rate;
        } else {
            goto thread_exit;
        }
    } 
    // ================= WAV 初始化 =================
    else if (is_wav) {
        FILE *f = fopen(current_audio_path, "rb");
        if (!f) goto thread_exit;
        
        // 简易 WAV 头解析 (跳过寻找 fmt 和 data 块)
        uint32_t sample_rate = 44100;
        uint16_t channels = 2;
        uint32_t data_size = 0;
        
        fseek(f, 22, SEEK_SET); fread(&channels, 2, 1, f);
        fseek(f, 24, SEEK_SET); fread(&sample_rate, 4, 1, f);
        
        // 寻找 data 块获取大小
        fseek(f, 12, SEEK_SET);
        char chunk[4]; uint32_t chunk_size;
        while (fread(chunk, 1, 4, f) == 4 && fread(&chunk_size, 4, 1, f) == 1) {
            if (strncmp(chunk, "data", 4) == 0) { data_size = chunk_size; break; }
            fseek(f, chunk_size, SEEK_CUR);
        }
        
        alsa_init(sample_rate, channels);
        audio_total_time = (double)data_size / (sample_rate * channels * 2);
        
        // 播放循环
        unsigned char buffer[4096];
        size_t bytes_read;
        long total_bytes_played = 0;
        
        while (!audio_thread_exit && (bytes_read = fread(buffer, 1, sizeof(buffer), f)) > 0) {
            while (audio_is_paused && !audio_thread_exit) usleep(50000); // 暂停逻辑
            
            // 处理 UI 传来的快进快退请求
            if (audio_seek_request >= 0) {
                long target_offset = (data_size * audio_seek_request) / 100;
                target_offset -= (target_offset % (channels * 2)); // 对齐
                fseek(f, ftell(f) - total_bytes_played + target_offset, SEEK_SET);
                total_bytes_played = target_offset;
                audio_seek_request = -1; // 清除请求
            }
            
            int frames = bytes_read / (channels * 2);
            snd_pcm_writei(pcm_handle, buffer, frames);
            total_bytes_played += bytes_read;
            audio_current_time = (double)total_bytes_played / (sample_rate * channels * 2);
        }
        fclose(f);
        goto thread_exit;
    }
    
    // MP3 播放循环
    if (is_mp3) {
        size_t done;
        unsigned char buffer[4096];
        long rate; int channels, encoding;
        mpg123_getformat(m, &rate, &channels, &encoding);
        
        while (!audio_thread_exit) {
            while (audio_is_paused && !audio_thread_exit) usleep(50000);
            
            // 处理 UI 传来的快进快退请求
            if (audio_seek_request >= 0) {
                off_t target_sample = (mpg123_length(m) * audio_seek_request) / 100;
                mpg123_seek(m, target_sample, SEEK_SET);
                audio_seek_request = -1; // 清除请求
            }
            
            int err = mpg123_read(m, buffer, sizeof(buffer), &done);
            if (err == MPG123_DONE) break; // 播放结束
            if (err != MPG123_OK) continue;
            
            int frames = done / (channels * 2);
            // 写入 ALSA，如果发生 underflow 则恢复
            if (snd_pcm_writei(pcm_handle, buffer, frames) == -EPIPE) {
                snd_pcm_prepare(pcm_handle);
            }
            audio_current_time = (double)mpg123_tell(m) / rate;
        }
        mpg123_close(m);
        mpg123_delete(m);
        mpg123_exit();
    }

thread_exit:
    alsa_close();
    return NULL;
}

// ==========================================
// LVGL UI Logic (Music Player)
// ==========================================
static void music_pause_resume_cb(lv_event_t * e)
{
    lv_obj_t * btn = lv_event_get_target(e);
    lv_obj_t * label = lv_obj_get_child(btn, 0);

    if (!audio_is_paused) {
        audio_is_paused = true;
        lv_label_set_text(label, "Play");
    } else {
        audio_is_paused = false;
        lv_label_set_text(label, "Pause");
    }
}

static void music_close_cb(lv_event_t * e)
{
    // 优雅退出后台音频线程
    audio_thread_exit = true;
    audio_is_paused = false; // 防止线程卡在暂停循环里
    pthread_join(audio_thread, NULL); 
    
    if (music_fake_timer) {
        lv_timer_del(music_fake_timer);
        music_fake_timer = NULL;
    }
    if (music_player_screen) {
        lv_obj_del(music_player_screen);
        music_player_screen = NULL;
    }
    if (music_browser_screen) {
        lv_obj_clear_flag(music_browser_screen, LV_OBJ_FLAG_HIDDEN);
        lv_group_focus_obj(lv_obj_get_child(music_browser_screen, 0)); 
    }
}

// 真实进度条 UI 定时器
static void music_progress_timer_cb(lv_timer_t * t)
{
    if (audio_total_time <= 0 || !music_progress_bar) return;
    
    // 如果用户正在拖动/操作滑动条，先不要自动更新它，以免产生 UI 冲突
    if (lv_obj_has_state(music_progress_bar, LV_STATE_FOCUSED) && lv_indev_get_act()) return;

    int percent = (int)((audio_current_time / audio_total_time) * 100.0);
    if (percent > 100) percent = 100;
    lv_slider_set_value(music_progress_bar, percent, LV_ANIM_ON);
}

// 用户滑动进度条触发的回调
static void music_slider_changed_cb(lv_event_t * e)
{
    lv_obj_t * slider = lv_event_get_target(e);
    audio_seek_request = lv_slider_get_value(slider);
}

static void music_play_event_handler(lv_event_t * e)
{
    const char * filename = lv_event_get_user_data(e);
    if (!filename) return;

    snprintf(current_audio_path, sizeof(current_audio_path), "/oem/usr/share/audio/%s", filename);

    // 初始化线程控制变量
    audio_thread_exit = false;
    audio_is_paused = false;
    audio_seek_request = -1;
    audio_current_time = 0;
    audio_total_time = 0;

    // 启动后台播放线程
    pthread_create(&audio_thread, NULL, audio_playback_thread, NULL);

    if (music_browser_screen) lv_obj_add_flag(music_browser_screen, LV_OBJ_FLAG_HIDDEN);

    music_player_screen = lv_obj_create(lv_scr_act());
    lv_obj_set_size(music_player_screen, 240, 160);
    lv_obj_center(music_player_screen);
    lv_obj_set_style_border_width(music_player_screen, 2, 0);

    lv_obj_t * title = lv_label_create(music_player_screen);
    lv_label_set_text(title, filename);
    lv_obj_set_style_text_font(title, &nes_font_16, 0); 
    lv_obj_set_width(title, 200);
    lv_label_set_long_mode(title, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // ==================== 进度条升级为 Slider ====================
    music_progress_bar = lv_slider_create(music_player_screen);
    lv_obj_set_size(music_progress_bar, 200, 10);
    lv_obj_align(music_progress_bar, LV_ALIGN_CENTER, 0, -10);
    lv_slider_set_range(music_progress_bar, 0, 100);
    lv_slider_set_value(music_progress_bar, 0, LV_ANIM_OFF);
    // 监听拖动或按键改变值的事件，从而实现快进快退
    lv_obj_add_event_cb(music_progress_bar, music_slider_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t * btn_pause = lv_btn_create(music_player_screen);
    lv_obj_set_size(btn_pause, 80, 30);
    lv_obj_align(btn_pause, LV_ALIGN_BOTTOM_LEFT, 15, -10);
    lv_obj_add_event_cb(btn_pause, music_pause_resume_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * lbl_pause = lv_label_create(btn_pause);
    lv_label_set_text(lbl_pause, "Pause");
    lv_obj_center(lbl_pause);

    lv_obj_t * btn_close = lv_btn_create(music_player_screen);
    lv_obj_set_size(btn_close, 80, 30);
    lv_obj_align(btn_close, LV_ALIGN_BOTTOM_RIGHT, -15, -10);
    lv_obj_add_event_cb(btn_close, music_close_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * lbl_close = lv_label_create(btn_close);
    lv_label_set_text(lbl_close, "Close");
    lv_obj_center(lbl_close);

    // 将 Slider 也加入按键组！这样你的手柄方向键(上下/左右)就能聚焦并拖动进度条了
    lv_group_t * g = lv_group_get_default();
    lv_group_add_obj(g, music_progress_bar); 
    lv_group_add_obj(g, btn_pause);
    lv_group_add_obj(g, btn_close);
    lv_group_focus_obj(btn_pause);

    // 定时器现在更新真实的进度
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
