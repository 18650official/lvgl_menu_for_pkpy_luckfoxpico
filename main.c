/**
 * @file main.c
 * @author Gemini
 * @brief A complete LVGL menu application for Luckfox Pico.
 *
 * Features:
 * - Dark theme with functional menu and dialogs.
 * - Real-time clock.
 * - Navigable menu with WASD/Space key support.
 * - "About" screen with system information.
 * - "Reboot" confirmation dialog with proper focus trapping.
 */

#define _DEFAULT_SOURCE // For usleep declaration
#include "lvgl/lvgl.h" // CRITICAL FIX: Changed .hh to .h
#include "lv_drivers/display/fbdev.h"
#include "lv_drivers/indev/evdev.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/input-event-codes.h> // For KEY_W, KEY_A, etc.

#define DISP_BUF_SIZE (480 * 10)

// --- Configuration ---
#define EVDEV_PATH "/dev/input/event1"

// --- Global UI Objects ---
lv_obj_t * time_label;
lv_obj_t * menu_list;
lv_obj_t * about_screen;

// --- Forward Declarations for UI Creation ---
void create_main_menu(lv_obj_t * parent, lv_group_t * g);
void create_about_screen(lv_obj_t * parent);
void create_reboot_msgbox();

// --- Helper function to read a file into a buffer ---
static char* read_file_to_string(const char* filepath, char* buffer, size_t buffer_size) {
    FILE* fp = fopen(filepath, "r");
    if (!fp) {
        snprintf(buffer, buffer_size, "Error: Cannot open %s", filepath);
        return buffer;
    }
    size_t read_len = fread(buffer, 1, buffer_size - 1, fp);
    buffer[read_len] = '\0';
    fclose(fp);
    return buffer;
}

// --- Event Callback Functions ---

/**
 * @brief A generic event handler that restores the main menu when a modal dialog is closed.
 * This is attached to the LV_EVENT_DELETE event of the modal.
 */
static void modal_close_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_DELETE) {
        LV_LOG_USER("Modal is closing, restoring main menu...");
        lv_obj_clear_flag(menu_list, LV_OBJ_FLAG_HIDDEN);
        lv_group_focus_obj(lv_obj_get_child(menu_list, 0)); // Return focus to the menu
    }
}

static void reboot_msgbox_event_handler(lv_event_t * e) {
    lv_obj_t * mbox = lv_event_get_current_target(e);
    const char * btn_text = lv_msgbox_get_active_btn_text(mbox);

    if (btn_text) {
        if (strcmp(btn_text, "Confirm") == 0) {
            LV_LOG_USER("Reboot confirmed. Rebooting now...");
            system("reboot");
            // If reboot command is sent, we don't need to close the msgbox manually
        } else {
            // For "Cancel" or any other button, just close the msgbox.
            // The modal_close_event_cb will handle the UI restoration.
            lv_msgbox_close(mbox);
        }
    }
}

static void about_screen_back_btn_event_handler(lv_event_t * e) {
    // We can use the generic close handler for the about screen as well
    lv_obj_del(about_screen);
    about_screen = NULL; // Set to NULL so it can be recreated
}

static void main_menu_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if(code == LV_EVENT_CLICKED) {
        lv_obj_t * label = lv_obj_get_child(obj, 0);
        const char * text = lv_label_get_text(label);
        LV_LOG_USER("Clicked: %s", text);

        if (strcmp(text, "About") == 0) {
            lv_obj_add_flag(menu_list, LV_OBJ_FLAG_HIDDEN);
            create_about_screen(lv_scr_act());
        }
        else if (strcmp(text, "Reboot") == 0) {
            create_reboot_msgbox();
        }
    }
}

static void time_update_task(lv_timer_t * timer)
{
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char time_str[9];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", tm_info);
    lv_label_set_text(time_label, time_str);
}


// --- UI Creation Functions ---

void create_reboot_msgbox() {
    lv_obj_add_flag(menu_list, LV_OBJ_FLAG_HIDDEN);

    static const char * btns[] = {"Confirm", "Cancel", ""};
    
    lv_obj_t * mbox = lv_msgbox_create(lv_scr_act(), "Reboot", "Are you sure you want to reboot?", btns, true);
    lv_obj_add_event_cb(mbox, reboot_msgbox_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    // CRITICAL FIX: Add a separate handler for the DELETE event.
    lv_obj_add_event_cb(mbox, modal_close_event_cb, LV_EVENT_DELETE, NULL);
    lv_obj_center(mbox);

    // --- Dark Theme Styling ---
    lv_obj_set_style_bg_color(mbox, lv_color_hex(0x2d2d2d), 0);
    lv_obj_set_style_text_color(lv_msgbox_get_title(mbox), lv_color_hex(0xe0e0e0), 0);
    lv_obj_set_style_text_color(lv_msgbox_get_text(mbox), lv_color_hex(0xc0c0c0), 0);
    
    lv_obj_t * mbox_btns = lv_msgbox_get_btns(mbox);
    lv_obj_set_style_bg_color(mbox_btns, lv_color_hex(0x404040), LV_PART_ITEMS);
    lv_obj_set_style_bg_color(mbox_btns, lv_color_hex(0x5070a0), LV_PART_ITEMS | LV_STATE_FOCUSED);
    lv_obj_set_style_text_color(mbox_btns, lv_color_hex(0xffffff), LV_PART_ITEMS);

    lv_group_add_obj(lv_group_get_default(), mbox);
    lv_group_focus_obj(mbox);
}

void create_about_screen(lv_obj_t * parent) {
    about_screen = lv_obj_create(parent);
    // CRITICAL FIX: Add the generic close handler to this modal too.
    lv_obj_add_event_cb(about_screen, modal_close_event_cb, LV_EVENT_DELETE, NULL);
    lv_obj_set_size(about_screen, lv_obj_get_width(parent), lv_obj_get_height(parent));
    lv_obj_set_style_bg_color(about_screen, lv_color_hex(0x1e1e1e), 0);
    lv_obj_set_style_border_width(about_screen, 0, 0);
    // lv_obj_add_flag(about_screen, LV_OBJ_FLAG_HIDDEN); // No longer needed, created on demand

    char buffer[512];
    
    long mem_total = 0, mem_available = 0;
    FILE* fp = fopen("/proc/meminfo", "r");
    if (fp) {
        char line[128];
        while(fgets(line, sizeof(line), fp)) {
            if(sscanf(line, "MemTotal: %ld kB", &mem_total) == 1) {}
            if(sscanf(line, "MemAvailable: %ld kB", &mem_available) == 1) {}
        }
        fclose(fp);
    }
    snprintf(buffer, sizeof(buffer),
        "Device: Luckfox Pico\n"
        "Memory: %ld MB / %ld MB Available\n\n"
        "Package Version:\n%s\n"
        "Developer: Snowmiku\ngithub.com/18650official",
        mem_total / 1024, mem_available / 1024,
        read_file_to_string("/oem/pkpy/info", (char[256]){"Reading..."}, 256));

    lv_obj_t * about_label = lv_label_create(about_screen);
    lv_label_set_text(about_label, buffer);
    lv_obj_set_style_text_color(about_label, lv_color_hex(0xe0e0e0), 0);
    lv_obj_set_style_text_font(about_label, &lv_font_montserrat_16, 0);
    lv_obj_align(about_label, LV_ALIGN_TOP_LEFT, 20, 20);

    lv_obj_t * back_btn = lv_btn_create(about_screen);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_add_event_cb(back_btn, about_screen_back_btn_event_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t * back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back");
    lv_obj_center(back_label);

    lv_group_add_obj(lv_group_get_default(), back_btn);
    lv_group_focus_obj(back_btn);
}

void create_main_menu(lv_obj_t * parent, lv_group_t * g) {
    menu_list = lv_list_create(parent);
    lv_obj_set_size(menu_list, 300, 240); // Adjusted height
    lv_obj_center(menu_list);
    lv_obj_set_style_bg_color(menu_list, lv_color_hex(0x2d2d2d), 0);
    lv_obj_set_style_border_width(menu_list, 0, 0);
    lv_obj_set_style_pad_all(menu_list, 10, 0);

    const char * menu_items[] = {"Start Game", "Console", "Settings", "About", "Reboot"};
    for(int i = 0; i < sizeof(menu_items)/sizeof(menu_items[0]); i++) {
        lv_obj_t * btn = lv_list_add_btn(menu_list, NULL, menu_items[i]);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x404040), LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x5070a0), LV_STATE_FOCUSED);
        lv_obj_set_style_text_color(btn, lv_color_hex(0xffffff), 0);
        lv_obj_add_event_cb(btn, main_menu_event_handler, LV_EVENT_CLICKED, NULL);
        lv_group_add_obj(g, btn);
    }
}


int main(void)
{
    lv_init();
    fbdev_init();

    static lv_color_t buf[DISP_BUF_SIZE];
    static lv_disp_draw_buf_t disp_buf;
    lv_disp_draw_buf_init(&disp_buf, buf, NULL, DISP_BUF_SIZE);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf  = &disp_buf;
    disp_drv.flush_cb  = fbdev_flush;
    disp_drv.hor_res   = 480;
    disp_drv.ver_res   = 320;
    lv_disp_drv_register(&disp_drv);
    
    evdev_init();
    evdev_set_file(EVDEV_PATH);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_KEYPAD;
    indev_drv.read_cb = evdev_read;
    lv_indev_t * keypad_indev = lv_indev_drv_register(&indev_drv);

    lv_group_t * g = lv_group_create();
    lv_group_set_default(g);
    lv_indev_set_group(keypad_indev, g);

    // --- UI Creation ---
    lv_obj_t * screen = lv_scr_act();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x1e1e1e), LV_PART_MAIN);

    time_label = lv_label_create(screen);
    lv_obj_set_style_text_color(time_label, lv_color_hex(0xe0e0e0), 0);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_20, 0);
    lv_obj_align(time_label, LV_ALIGN_TOP_RIGHT, -10, 10);
    
    lv_timer_create(time_update_task, 1000, NULL);
    time_update_task(NULL);

    create_main_menu(screen, g);
    
    while(1) {
        lv_timer_handler();
        usleep(5000);
    }

    return 0;
}

uint32_t custom_tick_get(void)
{
    static uint64_t start_ms = 0;
    if(start_ms == 0) {
        struct timeval tv_start;
        gettimeofday(&tv_start, NULL);
        start_ms = (tv_start.tv_sec * 1000000 + tv_start.tv_usec) / 1000;
    }

    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    uint64_t now_ms;
    now_ms = (tv_now.tv_sec * 1000000 + tv_now.tv_usec) / 1000;

    uint32_t time_ms = now_ms - start_ms;
    return time_ms;
}

