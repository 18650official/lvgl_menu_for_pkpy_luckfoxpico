/**
 * @file main.c
 * @author Gemini
 * @brief A complete LVGL menu example for Luckfox Pico.
 *
 * Features:
 * - Dark theme.
 * - Real-time local clock.
 * - A navigable menu list using keypad input.
 * - Event handling for menu item selection.
 * - No network dependency.
 * - Key translation is now handled directly inside the modified evdev.c driver.
 */

#define _DEFAULT_SOURCE // For usleep declaration
#include "lvgl/lvgl.h"
#include "lv_drivers/display/fbdev.h"
#include "lv_drivers/indev/evdev.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <linux/input-event-codes.h> // For KEY_W, KEY_A, etc.

#define DISP_BUF_SIZE (480 * 10)

// --- Configuration ---
// Set the path to your virtual keypad's event device file.
#define EVDEV_PATH "/dev/input/event1"

// --- Global UI component pointer ---
lv_obj_t * time_label;

// --- Event Callback Functions ---

/**
 * @brief Event handler called when a menu item is clicked (or Enter is pressed).
 */
static void menu_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if(code == LV_EVENT_CLICKED) {
        // Get the label text from the button to identify which item was selected
        lv_obj_t * label = lv_obj_get_child(obj, 0);
        LV_LOG_USER("Clicked: %s", lv_label_get_text(label));

        // Here, you can add actions based on the clicked menu item.
    }
}

/**
 * @brief Timer callback function to update the time label every second.
 */
static void time_update_task(lv_timer_t * timer)
{
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    
    char time_str[9];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", tm_info);

    // Update the label's text
    lv_label_set_text(time_label, time_str);
}


int main(void)
{
    /* LVGL initialization */
    lv_init();

    /* Linux Framebuffer device initialization */
    fbdev_init();

    /* A buffer for LVGL to draw the screen's content */
    static lv_color_t buf[DISP_BUF_SIZE];
    static lv_disp_draw_buf_t disp_buf;
    lv_disp_draw_buf_init(&disp_buf, buf, NULL, DISP_BUF_SIZE);

    /* Create a display driver */
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf  = &disp_buf;
    disp_drv.flush_cb  = fbdev_flush;
    disp_drv.hor_res   = 480; // Set to your screen's width
    disp_drv.ver_res   = 320; // Set to your screen's height
    lv_disp_drv_register(&disp_drv);
    
    /* Linux evdev device initialization */
    evdev_init();
    evdev_set_file(EVDEV_PATH);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_KEYPAD;
    /* --- REVERTED: Use the original evdev_read callback --- */
    indev_drv.read_cb = evdev_read;
    lv_indev_t * keypad_indev = lv_indev_drv_register(&indev_drv);

    /* Create an input group for navigation */
    lv_group_t * g = lv_group_create();
    lv_group_set_default(g);
    lv_indev_set_group(keypad_indev, g);

    // ===================================
    // == UI Creation
    // ===================================

    lv_obj_t * screen = lv_scr_act();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x1e1e1e), LV_PART_MAIN);

    time_label = lv_label_create(screen);
    lv_obj_set_style_text_color(time_label, lv_color_hex(0xe0e0e0), 0);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_20, 0);
    lv_obj_align(time_label, LV_ALIGN_TOP_RIGHT, -10, 10);
    
    lv_timer_create(time_update_task, 1000, NULL);
    time_update_task(NULL);

    lv_obj_t * menu_list = lv_list_create(screen);
    lv_obj_set_size(menu_list, 300, 200);
    lv_obj_center(menu_list);
    lv_obj_set_style_bg_color(menu_list, lv_color_hex(0x2d2d2d), 0);
    lv_obj_set_style_border_width(menu_list, 0, 0);
    lv_obj_set_style_pad_all(menu_list, 10, 0);

    // --- Add menu items (buttons) ---
    const char * menu_items[] = {"Start Game", "Settings", "About", "Reboot"};
    for(int i = 0; i < sizeof(menu_items)/sizeof(menu_items[0]); i++) {
        lv_obj_t * btn = lv_list_add_btn(menu_list, NULL, menu_items[i]);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x404040), LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x5070a0), LV_STATE_FOCUSED);
        lv_obj_set_style_text_color(btn, lv_color_hex(0xffffff), 0);
        
        lv_obj_add_event_cb(btn, menu_event_handler, LV_EVENT_CLICKED, NULL);

        /* --- CRITICAL FIX: Add the button to the input group --- */
        lv_group_add_obj(g, btn);
    }
    
    // ===================================
    // == Main Loop
    // ===================================
    while(1) {
        lv_timer_handler();
        usleep(5000);
    }

    return 0;
}

/*Set in lv_conf.h as `LV_TICK_CUSTOM_SYS_TIME_EXPR`*/
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

