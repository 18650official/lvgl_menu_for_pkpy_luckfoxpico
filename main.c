/**
 * @file main.c
 * @author Gemini
 * @brief A complete LVGL menu application for Luckfox Pico with fbterm console integration.
 *
 * Features:
 * - Dark theme with functional menu and dialogs.
 * - Real-time clock with persistence.
 * - Navigable menu with WASD/Space key support.
 * - "About" screen with system information.
 * - "Reboot" confirmation dialog with proper focus trapping.
 * - "Console" mode that hides UI, launches fbterm, and provides a clean exit.
 */

#define _DEFAULT_SOURCE // For usleep declaration
#include "lvgl/lvgl.h"
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
#define PREFS_FILE "/etc/menu_prefs.conf"

// --- Global UI Objects ---
lv_obj_t * time_label;
lv_obj_t * menu_list;
lv_obj_t * about_screen;
lv_obj_t * console_screen; // Global handle for the console screen
lv_obj_t * settings_screen;
lv_obj_t * time_settings_screen;

// --- Global Preferences ---
bool show_seconds = true;
bool is_24_hour_format = true;

// --- Forward Declarations for UI and Helper Functions ---
void create_main_menu(lv_obj_t * parent, lv_group_t * g);
void create_about_screen(lv_obj_t * parent);
void create_reboot_msgbox();
void create_console_screen(lv_obj_t * parent);
void create_settings_screen(lv_obj_t * parent);
void create_time_settings_screen();
void create_time_setter_page();
void create_timezone_page();
void create_show_seconds_page();
void create_hour_format_page();
void create_generic_option_page(const char* title, const char** options, lv_event_cb_t event_cb, lv_obj_t** screen_handle, lv_event_cb_t close_cb);
void load_preferences();
void save_preferences();
static void time_update_task(lv_timer_t * timer);
static void generic_delete_obj_event_cb(lv_event_t * e);
static lv_obj_t* create_styled_list_btn(lv_obj_t * parent, const char * text);


// --- Style for focused items in custom pages ---
static lv_style_t style_focused;

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

// --- NEWLY ADDED HELPER FUNCTIONS ---

/**
 * @brief Helper function to create a styled list button, reducing code duplication.
 */
static lv_obj_t* create_styled_list_btn(lv_obj_t * parent, const char * text) {
    lv_obj_t * btn = lv_list_add_btn(parent, NULL, text);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x404040), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x5070a0), LV_STATE_FOCUSED);
    lv_obj_set_style_text_color(btn, lv_color_hex(0xffffff), 0);
    return btn;
}

/**
 * @brief Helper function to create a generic options page (e.g., On/Off selectors).
 */
void create_generic_option_page(const char* title, const char** options, lv_event_cb_t event_cb, lv_obj_t** screen_handle, lv_event_cb_t close_cb) {
    lv_obj_t * parent_screen = *screen_handle; // Get the parent screen
    if(parent_screen) lv_obj_add_flag(parent_screen, LV_OBJ_FLAG_HIDDEN); // Hide it

    lv_obj_t * page = lv_obj_create(lv_scr_act());
    *screen_handle = page; // The new page is now the handle
    lv_obj_set_size(page, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(page, lv_color_hex(0x1e1e1e), 0);
    lv_obj_add_event_cb(page, close_cb, LV_EVENT_DELETE, NULL);

    lv_obj_t * list = lv_list_create(page);
    lv_obj_center(list);
    lv_obj_set_size(list, 300, 280);
    lv_obj_set_style_bg_color(list, lv_color_hex(0x2d2d2d), 0);

    lv_list_add_text(list, title);

    lv_group_t * g = lv_group_get_default();
    for (int i = 0; options[i] != NULL; i++) {
        lv_obj_t* btn = create_styled_list_btn(list, options[i]);
        lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, page);
        lv_group_add_obj(g, btn);
    }
    // Focus the first actual option button, skipping the title
    if (lv_obj_get_child_cnt(list) > 1) {
        lv_group_focus_obj(lv_obj_get_child(list, 1));
    }
}


// --- Preference Management ---
void save_preferences() {
    FILE* fp = fopen(PREFS_FILE, "w");
    if (!fp) {
        LV_LOG_ERROR("Failed to open preferences file for writing.");
        return;
    }
    fprintf(fp, "SHOW_SECONDS=%d\n", show_seconds ? 1 : 0);
    fprintf(fp, "IS_24_HOUR=%d\n", is_24_hour_format ? 1 : 0);
    fclose(fp);
    LV_LOG_USER("Preferences saved.");
}

void load_preferences() {
    FILE* fp = fopen(PREFS_FILE, "r");
    if (!fp) {
        LV_LOG_WARN("Preferences file not found, creating with defaults.");
        save_preferences(); // Create with default values
        return;
    }
    char line[100];
    while (fgets(line, sizeof(line), fp)) {
        char key[50];
        int value;
        if (sscanf(line, "%[^=]=%d", key, &value) == 2) {
            if (strcmp(key, "SHOW_SECONDS") == 0) {
                show_seconds = (value == 1);
            } else if (strcmp(key, "IS_24_HOUR") == 0) {
                is_24_hour_format = (value == 1);
            }
        }
    }
    fclose(fp);
    LV_LOG_USER("Preferences loaded.");
}

// --- Event Callback Functions ---

// Generic handler to delete an object, used for "Back" buttons
static void generic_delete_obj_event_cb(lv_event_t * e) {
    lv_obj_t * obj_to_delete = lv_event_get_user_data(e);
    if(obj_to_delete) {
        lv_obj_del(obj_to_delete);
    }
}

static void modal_close_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_DELETE) {
        LV_LOG_USER("Modal is closing, restoring main menu...");
        if(menu_list) {
            lv_obj_clear_flag(menu_list, LV_OBJ_FLAG_HIDDEN);
            lv_group_focus_obj(lv_obj_get_child(menu_list, 0));
        }
    }
}

static void settings_screen_close_cb(lv_event_t * e) {
    if (lv_event_get_code(e) == LV_EVENT_DELETE && menu_list) {
        lv_obj_clear_flag(menu_list, LV_OBJ_FLAG_HIDDEN);
        lv_group_focus_obj(lv_obj_get_child(menu_list, 2)); // Focus "Settings"
    }
}

static void time_settings_screen_close_cb(lv_event_t * e) {
    if (lv_event_get_code(e) == LV_EVENT_DELETE && settings_screen) {
        lv_obj_clear_flag(settings_screen, LV_OBJ_FLAG_HIDDEN);
        lv_group_focus_obj(lv_obj_get_child(settings_screen, 0));
    }
}

static void reboot_msgbox_event_handler(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * mbox = lv_event_get_current_target(e);

    if (code == LV_EVENT_VALUE_CHANGED) {
        const char * btn_text = lv_msgbox_get_active_btn_text(mbox);
        if (btn_text && strcmp(btn_text, "Confirm") == 0) {
            LV_LOG_USER("Reboot confirmed. Rebooting now...");
            system("reboot");
        } else {
            lv_msgbox_close(mbox);
        }
    }
}

static void about_screen_back_btn_event_handler(lv_event_t * e) {
    lv_obj_del(about_screen);
    about_screen = NULL;
}

// --- Time Setter Page: Logic and Callbacks ---
static int edit_hour;
static int edit_minute;
static lv_obj_t * time_setter_hour_label;
static lv_obj_t * time_setter_minute_label;

static void time_value_adjust_event_cb(lv_event_t * e) {
    lv_obj_t* label = lv_event_get_user_data(e);
    uint32_t key = lv_indev_get_key(lv_indev_get_act());
    bool is_hour = (label == time_setter_hour_label);

    if (key == LV_KEY_UP) {
        if (is_hour) edit_hour = (edit_hour + 1) % 24;
        else edit_minute = (edit_minute + 1) % 60;
    } else if (key == LV_KEY_DOWN) {
        if (is_hour) edit_hour = (edit_hour - 1 + 24) % 24;
        else edit_minute = (edit_minute - 1 + 60) % 60;
    }

    if (is_hour) lv_label_set_text_fmt(label, "%02d", edit_hour);
    else lv_label_set_text_fmt(label, "%02d", edit_minute);
}

static void time_save_event_cb(lv_event_t * e) {
    char command[50];
    snprintf(command, sizeof(command), "date -s \"%02d:%02d:00\"", edit_hour, edit_minute);
    LV_LOG_USER("Setting time: %s", command);
    system(command);
    system("hwclock -w");
    time_update_task(NULL);
    generic_delete_obj_event_cb(e); // Delete the page
}

// --- Time Format/Seconds Pages: Logic and Callbacks ---
static void show_seconds_event_cb(lv_event_t * e) {
    lv_obj_t* btn = lv_event_get_target(e);
    lv_obj_t* page = lv_event_get_user_data(e);
    const char* text = lv_list_get_btn_text(lv_obj_get_parent(btn), btn);
    show_seconds = (strcmp(text, "On") == 0);
    save_preferences();
    time_update_task(NULL);
    if(page) lv_obj_del(page); // Go back
}

static void hour_format_event_cb(lv_event_t * e) {
    lv_obj_t* btn = lv_event_get_target(e);
    lv_obj_t* page = lv_event_get_user_data(e);
    const char* text = lv_list_get_btn_text(lv_obj_get_parent(btn), btn);
    is_24_hour_format = (strcmp(text, "24 Hour") == 0);
    save_preferences();
    time_update_task(NULL);
    if(page) lv_obj_del(page); // Go back
}

static void timezone_select_event_handler(lv_event_t * e) {
    const char * tz = lv_obj_get_user_data(lv_event_get_target(e));
    char command[100];
    snprintf(command, sizeof(command), "ln -sf /usr/share/zoneinfo/%s /etc/localtime", tz);
    LV_LOG_USER("Setting timezone: %s", command);
    system(command);
    generic_delete_obj_event_cb(e); // Go back
}

static void time_update_task(lv_timer_t * timer) {
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char time_str[12];
    const char * format_str = is_24_hour_format ? (show_seconds ? "%H:%M:%S" : "%H:%M")
                                                : (show_seconds ? "%I:%M:%S %p" : "%I:%M %p");
    strftime(time_str, sizeof(time_str), format_str, tm_info);
    lv_label_set_text(time_label, time_str);
}


// --- Menu Handlers ---
static void main_menu_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if(code == LV_EVENT_CLICKED) {
        const char * text = lv_list_get_btn_text(menu_list, obj);
        LV_LOG_USER("Clicked: %s", text);

        if (strcmp(text, "About") == 0) {
            lv_obj_add_flag(menu_list, LV_OBJ_FLAG_HIDDEN);
            create_about_screen(lv_scr_act());
        }
        else if (strcmp(text, "Reboot") == 0) {
            create_reboot_msgbox();
        }
        else if (strcmp(text, "Console") == 0) {
            create_console_screen(lv_scr_act());
        }
        else if (strcmp(text, "Settings") == 0) {
            lv_obj_add_flag(menu_list, LV_OBJ_FLAG_HIDDEN);
            create_settings_screen(lv_scr_act());
        }
    }
}

static void settings_menu_event_handler(lv_event_t * e) {
    lv_obj_t * obj = lv_event_get_target(e);
    const char * text = lv_list_get_btn_text(settings_screen, obj);
    if (strcmp(text, "Time Settings") == 0) {
        lv_obj_add_flag(settings_screen, LV_OBJ_FLAG_HIDDEN);
        create_time_settings_screen();
    } else if (strcmp(text, "Back") == 0) {
        lv_obj_del(settings_screen);
    }
}

static void time_settings_menu_event_handler(lv_event_t * e) {
    lv_obj_t * obj = lv_event_get_target(e);
    const char * text = lv_list_get_btn_text(time_settings_screen, obj);

    if (strcmp(text, "Set time") == 0) create_time_setter_page();
    else if (strcmp(text, "Timezone") == 0) create_timezone_page();
    else if (strcmp(text, "Display method") == 0) create_show_seconds_page();
    else if (strcmp(text, "12/24 Hour format") == 0) create_hour_format_page();
    else if (strcmp(text, "Back") == 0) lv_obj_del(time_settings_screen);
}

static void console_exit_event_handler(lv_event_t * e) {
    LV_LOG_USER("Exiting console mode.");
    system("/etc/init.d/S99fbterm stop");

    if(console_screen) {
        lv_obj_del(console_screen);
        console_screen = NULL;
    }

    if(menu_list) {
        lv_obj_clear_flag(menu_list, LV_OBJ_FLAG_HIDDEN);
        lv_obj_t* console_btn = lv_obj_get_child(menu_list, 1);
        if(console_btn) {
            lv_group_focus_obj(console_btn);
        }
    }
    if(time_label) {
        lv_obj_clear_flag(time_label, LV_OBJ_FLAG_HIDDEN);
    }
    
    lv_refr_now(lv_disp_get_default());
}

// --- UI Creation Functions ---
void create_console_screen(lv_obj_t * parent) {
    lv_obj_add_flag(menu_list, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(time_label, LV_OBJ_FLAG_HIDDEN);
    
    console_screen = lv_obj_create(parent);
    lv_obj_remove_style_all(console_screen);
    lv_obj_set_size(console_screen, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_pos(console_screen, 0, 0);
    lv_obj_set_style_bg_color(console_screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(console_screen, LV_OPA_COVER, 0);

    lv_obj_t * exit_btn = lv_btn_create(console_screen);
    lv_obj_align(exit_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_add_event_cb(exit_btn, console_exit_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(exit_btn, lv_color_hex(0x404040), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(exit_btn, lv_color_hex(0x5070a0), LV_STATE_FOCUSED);

    lv_obj_t * exit_label = lv_label_create(exit_btn);
    lv_label_set_text(exit_label, "Exit");
    lv_obj_center(exit_label);
    lv_obj_set_style_text_color(exit_label, lv_color_hex(0xffffff), 0);

    lv_group_t * g = lv_group_get_default();
    lv_group_add_obj(g, exit_btn);
    lv_group_focus_obj(exit_btn);
    
    lv_timer_handler();
    usleep(16000);

    LV_LOG_USER("Starting fbterm...");
    system("/etc/init.d/S99fbterm start_with_input &");
}

void create_reboot_msgbox() {
    lv_obj_add_flag(menu_list, LV_OBJ_FLAG_HIDDEN);

    static const char * btns[] = {"Confirm", "Cancel", ""};
    
    lv_obj_t * mbox = lv_msgbox_create(lv_scr_act(), "Reboot", "Are you sure you want to reboot?", btns, true);
    lv_obj_add_event_cb(mbox, reboot_msgbox_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(mbox, modal_close_event_cb, LV_EVENT_DELETE, NULL);
    lv_obj_center(mbox);

    lv_obj_set_style_bg_color(mbox, lv_color_hex(0x2d2d2d), 0);
    lv_obj_set_style_text_color(lv_msgbox_get_title(mbox), lv_color_hex(0xe0e0e0), 0);
    lv_obj_set_style_text_color(lv_msgbox_get_text(mbox), lv_color_hex(0xc0c0c0), 0);
    
    lv_obj_t * mbox_btns = lv_msgbox_get_btns(mbox);
    lv_obj_set_style_bg_color(mbox_btns, lv_color_hex(0x404040), LV_PART_ITEMS);
    lv_obj_set_style_bg_color(mbox_btns, lv_color_hex(0x5070a0), LV_PART_ITEMS | LV_STATE_FOCUSED);
    lv_obj_set_style_text_color(mbox_btns, lv_color_hex(0xffffff), LV_PART_ITEMS);

    lv_obj_t * close_btn = lv_msgbox_get_close_btn(mbox);
    if (close_btn) {
        lv_obj_set_style_bg_color(close_btn, lv_color_hex(0x404040), LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(close_btn, lv_color_hex(0x5070a0), LV_STATE_FOCUSED);
    }
    
    lv_group_add_obj(lv_group_get_default(), mbox);
    lv_group_focus_obj(mbox);
}

void create_about_screen(lv_obj_t * parent) {
    about_screen = lv_obj_create(parent);
    lv_obj_add_event_cb(about_screen, modal_close_event_cb, LV_EVENT_DELETE, NULL);
    lv_obj_set_size(about_screen, lv_obj_get_width(parent), lv_obj_get_height(parent));
    lv_obj_set_style_bg_color(about_screen, lv_color_hex(0x1e1e1e), 0);
    lv_obj_set_style_border_width(about_screen, 0, 0);

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
    lv_obj_set_size(menu_list, 300, 240);
    lv_obj_center(menu_list);
    lv_obj_set_style_bg_color(menu_list, lv_color_hex(0x2d2d2d), 0);
    lv_obj_set_style_border_width(menu_list, 0, 0);
    lv_obj_set_style_pad_all(menu_list, 10, 0);

    const char * menu_items[] = {"Start Game", "Console", "Settings", "About", "Reboot"};
    for(int i = 0; i < sizeof(menu_items)/sizeof(menu_items[0]); i++) {
        lv_obj_t * btn = create_styled_list_btn(menu_list, menu_items[i]);
        lv_obj_add_event_cb(btn, main_menu_event_handler, LV_EVENT_CLICKED, NULL);
        lv_group_add_obj(g, btn);
    }
}

// --- Time and Settings Screen Creation ---
void create_show_seconds_page() {
    static const char* opts[] = {"On", "Off", NULL};
    create_generic_option_page("Second Display", opts, show_seconds_event_cb, &time_settings_screen, time_settings_screen_close_cb);
}

void create_hour_format_page() {
    static const char* opts[] = {"24 Hour", "12 Hour", NULL};
    create_generic_option_page("Time Format", opts, hour_format_event_cb, &time_settings_screen, time_settings_screen_close_cb);
}

void create_timezone_page() {
    lv_obj_add_flag(time_settings_screen, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t * page = lv_obj_create(lv_scr_act());
    lv_obj_set_size(page, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(page, lv_color_hex(0x1e1e1e), 0);
    lv_obj_add_event_cb(page, time_settings_screen_close_cb, LV_EVENT_DELETE, page);

    lv_obj_t* list = lv_list_create(page);
    lv_obj_center(list);
    lv_obj_set_size(list, 300, 280);
    lv_obj_set_style_bg_color(list, lv_color_hex(0x2d2d2d), 0);

    const char * timezones[] = {"UTC", "Asia/Shanghai", "America/New_York", "Europe/London", NULL};
    static const char * tz_paths[] = {"UTC", "Asia/Shanghai", "America/New_York", "Europe/London"};

    lv_group_t * g = lv_group_get_default();
    for(int i = 0; timezones[i] != NULL; i++) {
        lv_obj_t* btn = create_styled_list_btn(list, timezones[i]);
        lv_obj_add_event_cb(btn, timezone_select_event_handler, LV_EVENT_CLICKED, page);
        lv_obj_set_user_data(btn, (void*)tz_paths[i]);
        lv_group_add_obj(g, btn);
    }

    lv_obj_t* back_btn = create_styled_list_btn(list, "Back");
    lv_obj_add_event_cb(back_btn, generic_delete_obj_event_cb, LV_EVENT_CLICKED, page);
    lv_group_add_obj(g, back_btn);

    lv_group_focus_obj(lv_obj_get_child(list, 0));
}

void create_time_setter_page() {
    lv_obj_add_flag(time_settings_screen, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t * page = lv_obj_create(lv_scr_act());
    lv_obj_set_size(page, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(page, lv_color_hex(0x1e1e1e), 0);
    lv_obj_add_event_cb(page, time_settings_screen_close_cb, LV_EVENT_DELETE, NULL);

    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    edit_hour = tm_info->tm_hour;
    edit_minute = tm_info->tm_min;

    lv_obj_t* container = lv_obj_create(page);
    lv_obj_center(container);
    lv_obj_set_size(container, 350, 150);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(container, 20, 0);

    lv_obj_t * hour_obj = lv_obj_create(container);
    lv_obj_set_size(hour_obj, 100, 80);
    lv_obj_set_style_bg_color(hour_obj, lv_color_hex(0x404040), 0);
    lv_obj_add_style(hour_obj, &style_focused, LV_STATE_FOCUSED);
    time_setter_hour_label = lv_label_create(hour_obj);
    lv_label_set_text_fmt(time_setter_hour_label, "%02d", edit_hour);
    lv_obj_set_style_text_font(time_setter_hour_label, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(time_setter_hour_label, lv_color_white(), 0);
    lv_obj_center(time_setter_hour_label);
    lv_obj_add_event_cb(hour_obj, time_value_adjust_event_cb, LV_EVENT_KEY, time_setter_hour_label);

    lv_obj_t* sep_label = lv_label_create(container);
    lv_label_set_text(sep_label, ":");
    lv_obj_set_style_text_font(sep_label, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(sep_label, lv_color_white(), 0);

    lv_obj_t * minute_obj = lv_obj_create(container);
    lv_obj_set_size(minute_obj, 100, 80);
    lv_obj_set_style_bg_color(minute_obj, lv_color_hex(0x404040), 0);
    lv_obj_add_style(minute_obj, &style_focused, LV_STATE_FOCUSED);
    time_setter_minute_label = lv_label_create(minute_obj);
    lv_label_set_text_fmt(time_setter_minute_label, "%02d", edit_minute);
    lv_obj_set_style_text_font(time_setter_minute_label, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(time_setter_minute_label, lv_color_white(), 0);
    lv_obj_center(time_setter_minute_label);
    lv_obj_add_event_cb(minute_obj, time_value_adjust_event_cb, LV_EVENT_KEY, time_setter_minute_label);

    lv_obj_t* save_btn = lv_btn_create(page);
    lv_obj_align(save_btn, LV_ALIGN_BOTTOM_LEFT, 40, -20);
    lv_obj_t* save_label = lv_label_create(save_btn);
    lv_label_set_text(save_label, "Save");
    lv_obj_add_event_cb(save_btn, time_save_event_cb, LV_EVENT_CLICKED, page);

    lv_obj_t* back_btn = lv_btn_create(page);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_RIGHT, -40, -20);
    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back");
    lv_obj_add_event_cb(back_btn, generic_delete_obj_event_cb, LV_EVENT_CLICKED, page);

    lv_group_t* g = lv_group_get_default();
    lv_group_add_obj(g, hour_obj);
    lv_group_add_obj(g, minute_obj);
    lv_group_add_obj(g, save_btn);
    lv_group_add_obj(g, back_btn);
    lv_group_focus_obj(hour_obj);
}

void create_time_settings_screen() {
    time_settings_screen = lv_list_create(lv_scr_act());
    lv_obj_set_size(time_settings_screen, 300, 280);
    lv_obj_center(time_settings_screen);
    lv_obj_add_event_cb(time_settings_screen, time_settings_screen_close_cb, LV_EVENT_DELETE, NULL);
    lv_obj_set_style_bg_color(time_settings_screen, lv_color_hex(0x2d2d2d), 0);

    const char * items[] = {"Set time", "Timezone", "Display method", "12/24 Hour format", "Back"};
    for (int i = 0; i < 5; i++) {
        lv_obj_t * btn = create_styled_list_btn(time_settings_screen, items[i]);
        lv_obj_add_event_cb(btn, time_settings_menu_event_handler, LV_EVENT_CLICKED, NULL);
        lv_group_add_obj(lv_group_get_default(), btn);
    }
    lv_group_focus_obj(lv_obj_get_child(time_settings_screen, 0));
}

void create_settings_screen(lv_obj_t * parent) {
    settings_screen = lv_list_create(parent);
    lv_obj_set_size(settings_screen, 300, 240);
    lv_obj_center(settings_screen);
    lv_obj_add_event_cb(settings_screen, settings_screen_close_cb, LV_EVENT_DELETE, NULL);
    lv_obj_set_style_bg_color(settings_screen, lv_color_hex(0x2d2d2d), 0);

    create_styled_list_btn(settings_screen, "Time Settings");
    create_styled_list_btn(settings_screen, "Back");

    lv_group_t * g = lv_group_get_default();
    for (uint32_t i = 0; i < lv_obj_get_child_cnt(settings_screen); i++) {
        lv_obj_t * btn = lv_obj_get_child(settings_screen, i);
        lv_obj_add_event_cb(btn, settings_menu_event_handler, LV_EVENT_CLICKED, NULL);
        lv_group_add_obj(g, btn);
    }
    lv_group_focus_obj(lv_obj_get_child(settings_screen, 0));
}

// --- Main Application Entry ---
int main(void)
{
    lv_init();
    load_preferences(); // Load saved settings at startup

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
    
    time_update_task(NULL); // Initial time update
    lv_timer_create(time_update_task, 1000, NULL);

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