#include "ui/components/ui_status_bar.h"
#include "ui/components/ui_comp_common.h"
#include "lvgl/lvgl.h"
#include <unistd.h>
#include <stdbool.h>
#include <sys/stat.h>

/* ================== 优先级列表 ================== */
/* 如果未来要加新的标志，按照你希望从左到右的显示顺序加在这里 */
typedef enum {
    STATUS_ICON_HEADSET = 0, // 优先级 1: 耳机 (最左侧)
    STATUS_ICON_SDCARD,      // 优先级 2: SD卡
    // STATUS_ICON_WIFI,     // 优先级 3: (未来预留)
    // STATUS_ICON_BATTERY,  // 优先级 4: (未来预留)
    STATUS_ICON_MAX          // 占位符，勿删
} status_icon_id_t;
/* ============================================== */

static lv_obj_t * status_bar = NULL;
// 使用数组统一管理所有状态图标，方便按优先级控制
static lv_obj_t * status_icons[STATUS_ICON_MAX] = {NULL};

static lv_timer_t * status_timer = NULL;
static bool last_headset_present = false;
static bool last_sdcard_present = false;

bool detect_usb_headset(void)
{
    const char *dev = "/dev/snd/pcmC1D0p";
    struct stat st;
    if (stat(dev, &st) == 0) {
        if (S_ISCHR(st.st_mode)) return true;
        return true;
    }
    if (access("/proc/asound/card1", F_OK) == 0) return true;
    return false;
}

// 统一的可见性更新函数
static void update_icon_visibility(status_icon_id_t icon_id, bool on)
{
    if (!status_icons[icon_id]) return;
    if (on) {
        lv_obj_clear_flag(status_icons[icon_id], LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(status_icons[icon_id], LV_OBJ_FLAG_HIDDEN);
    }
}

// 提取出一个公共的图标创建函数，简化代码
// 增加一个 font 参数，以便我们可以给图标指定 LVGL 内置字体
static lv_obj_t * create_status_icon(lv_obj_t * parent, const char * text, uint32_t color_hex, const lv_font_t * font)
{
    lv_obj_t * label = lv_label_create(parent);
    lv_label_set_text(label, text);
    
    // 使用传入的字体
    lv_obj_set_style_text_font(label, font, 0); 
    
    lv_obj_set_style_text_color(label, lv_color_hex(color_hex), 0);
    lv_obj_add_flag(label, LV_OBJ_FLAG_HIDDEN); // 初始隐藏
    return label;
}

static bool detect_sdcard(void)
{
    /* 检查 SD 卡挂载目录是否可访问 */
    if (access("/mnt/sdcard", F_OK) == 0) {
        return true;
    }
    return false;
}

static void status_check_cb(lv_timer_t * t)
{
    (void)t;
    
    // 检查耳机
    bool headset_present = detect_usb_headset();
    if (headset_present != last_headset_present) {
        LV_LOG_USER("Headphone state changed: %s", headset_present ? "connected" : "disconnected");
        last_headset_present = headset_present;
        update_icon_visibility(STATUS_ICON_HEADSET, headset_present);
    }

    // 检查SD卡
    bool sdcard_present = detect_sdcard();
    if (sdcard_present != last_sdcard_present) {
        LV_LOG_USER("SD Card state changed: %s", sdcard_present ? "inserted" : "removed");
        last_sdcard_present = sdcard_present;
        update_icon_visibility(STATUS_ICON_SDCARD, sdcard_present);
    }
}

void ui_status_bar_init(lv_obj_t * parent)
{
    if (status_bar) return;
    status_bar = lv_obj_create(parent);
    lv_obj_clear_flag(status_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(status_bar, 120, 20); // 稍微加长 status_bar 以容纳多个图标
    lv_obj_align(status_bar, LV_ALIGN_TOP_LEFT, 4, 4);
    lv_obj_set_style_bg_opa(status_bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(status_bar, 0, 0);

    /* 【修复 1：清除默认内边距】防止父容器的 padding 将图标向下挤压 */
    lv_obj_set_style_pad_all(status_bar, 0, 0); 

    /* 给 status_bar 开启 Flex 行布局 */
    lv_obj_set_layout(status_bar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(status_bar, LV_FLEX_FLOW_ROW);    // 内部子元素水平排列
    
    /* 【修复 2：设置垂直对齐】主轴起点对齐(左)，交叉轴居中对齐(垂直居中) */
    lv_obj_set_flex_align(status_bar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    
    lv_obj_set_style_pad_column(status_bar, 6, 0);         // 设置图标之间的间距为 6 像素

    /* 
     * 此处创建的顺序，就是它们在界面上从左到右显示的最高优先级顺序
     * 必须和 status_icon_id_t 枚举的顺序一致
     * 【修复 3：调整颜色】将 0x00FF00 改为 0x009900 (较深的绿色)
     */
    status_icons[STATUS_ICON_HEADSET] = create_status_icon(status_bar, LV_SYMBOL_VOLUME_MAX, 0x009900, &lv_font_montserrat_16); 
    status_icons[STATUS_ICON_SDCARD]  = create_status_icon(status_bar, LV_SYMBOL_SD_CARD, 0x009900, &lv_font_montserrat_16);
    
    /* 每 1000 ms 轮询一次 */
    status_timer = lv_timer_create(status_check_cb, 1000, NULL);
    lv_timer_ready(status_timer);
}

void ui_status_bar_set_headphone(bool on)
{
    update_icon_visibility(STATUS_ICON_HEADSET, on);
}

void ui_status_bar_set_sdcard(bool on)
{
    update_icon_visibility(STATUS_ICON_SDCARD, on);
}