#include "ui/components/ui_status_bar.h"
#include "ui/components/ui_comp_common.h"
#include "lvgl/lvgl.h"
#include <unistd.h>
#include <stdbool.h>
#include <sys/stat.h>

static lv_obj_t * status_bar = NULL;
static lv_obj_t * headphone_label = NULL;
static lv_timer_t * status_timer = NULL;
static bool last_headset_present = false;

static void update_headphone_visibility(bool on)
{
    if (!headphone_label) return;
    if (on) {
        lv_obj_clear_flag(headphone_label, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(headphone_label, LV_OBJ_FLAG_HIDDEN);
    }
}

static bool detect_usb_headset(void)
{
    /* Check for presence of the USB pcm playback device created for card 1.
     * Use /dev/snd/pcmC1D0p as the primary indicator per system behavior.
     */
    const char *dev = "/dev/snd/pcmC1D0p";
    struct stat st;
    if (stat(dev, &st) == 0) {
        /* If it exists and is a character device, treat as present */
        if (S_ISCHR(st.st_mode)) return true;
        return true;
    }

    /* Fallback: check /proc/asound/card1 */
    if (access("/proc/asound/card1", F_OK) == 0) return true;

    return false;
}

static void status_check_cb(lv_timer_t * t)
{
    (void)t;
    bool present = detect_usb_headset();
    if (present != last_headset_present) {
        LV_LOG_USER("Headphone state changed: %s", present ? "connected" : "disconnected");
        last_headset_present = present;
    }
    update_headphone_visibility(present);
}

void ui_status_bar_init(lv_obj_t * parent)
{
    if (status_bar) return;
    status_bar = lv_obj_create(parent);
    lv_obj_clear_flag(status_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(status_bar, 64, 20);
    lv_obj_align(status_bar, LV_ALIGN_TOP_LEFT, 4, 4);
    lv_obj_set_style_bg_opa(status_bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(status_bar, 0, 0);

    headphone_label = lv_label_create(status_bar);
    lv_label_set_text(headphone_label, "qp");
    /* Use nes font and a visible color to avoid invisible icon issues */
    lv_obj_set_style_text_font(headphone_label, &nes_font_16, 0);
    lv_obj_set_style_text_color(headphone_label, lv_color_hex(0x00FF00), 0);
    lv_obj_align(headphone_label, LV_ALIGN_LEFT_MID, 0, 0);

    /* Start hidden until detected */
    lv_obj_add_flag(headphone_label, LV_OBJ_FLAG_HIDDEN);

    /* Poll every 1000 ms */
    status_timer = lv_timer_create(status_check_cb, 1000, NULL);
    lv_timer_ready(status_timer);
}

void ui_status_bar_set_headphone(bool on)
{
    update_headphone_visibility(on);
}
