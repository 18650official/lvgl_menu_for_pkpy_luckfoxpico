#include "lvgl/lvgl.h"
#include "ui/ui.h"
#include "ui/components/ui_comp_common.h"
#include "ui/screens/ui_scr_desktop.h"
#include "utils/prefs.h"

#include "lv_drivers/display/fbdev.h"
#include "lv_drivers/indev/evdev.h"
#include <unistd.h>
#include <sys/time.h>

#define DISP_BUF_SIZE (320 * 20)
#define EVDEV_PATH "/dev/input/event0"

lv_obj_t * ui_time_label = NULL;
lv_obj_t * ui_menu_list = NULL;

static void init_display(void)
{
    fbdev_init();

    static lv_color_t buf[DISP_BUF_SIZE];
    static lv_disp_draw_buf_t disp_buf;
    lv_disp_draw_buf_init(&disp_buf, buf, NULL, DISP_BUF_SIZE);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf = &disp_buf;
    disp_drv.flush_cb = fbdev_flush;
    disp_drv.hor_res = 320;
    disp_drv.ver_res = 240;
    lv_disp_drv_register(&disp_drv);
}

static void init_input(void)
{
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
}

uint32_t custom_tick_get(void);

static uint32_t custom_tick_get_internal(void)
{
    static uint64_t start_ms = 0;
    if (start_ms == 0) {
        struct timeval tv_start;
        gettimeofday(&tv_start, NULL);
        start_ms = (tv_start.tv_sec * 1000000 + tv_start.tv_usec) / 1000;
    }

    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    uint64_t now_ms = (tv_now.tv_sec * 1000000 + tv_now.tv_usec) / 1000;
    return (uint32_t)(now_ms - start_ms);
}

void ui_backend_init(void)
{
    lv_init();
    ui_comp_init_styles();
    prefs_load();
    init_display();
    init_input();

    lv_theme_t * th = lv_theme_default_init(lv_disp_get_default(),
                                            lv_palette_main(LV_PALETTE_BLUE),
                                            lv_palette_main(LV_PALETTE_RED),
                                            false,
                                            &lv_font_montserrat_14);
    lv_disp_set_theme(lv_disp_get_default(), th);
}

void ui_init(void)
{
    ui_create_desktop();
}

uint32_t custom_tick_get(void)
{
    return custom_tick_get_internal();
}
