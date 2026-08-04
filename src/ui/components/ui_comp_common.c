#include "ui/components/ui_comp_common.h"
#include "lvgl/lvgl.h"

LV_FONT_DECLARE(nes_font_16);

lv_style_t ui_comp_style_nes_cjk;
static lv_style_t style_compact_list;
static lv_style_t style_compact_btn;

void ui_comp_init_styles(void)
{
    lv_style_init(&style_compact_list);
    lv_style_set_radius(&style_compact_list, 0);
    lv_style_set_pad_all(&style_compact_list, 0);
    lv_style_set_pad_row(&style_compact_list, 2);
    lv_style_set_border_width(&style_compact_list, 0);

    lv_style_init(&style_compact_btn);
    lv_style_set_radius(&style_compact_btn, 4);
    lv_style_set_text_font(&style_compact_btn, &lv_font_montserrat_14);
    lv_style_set_pad_ver(&style_compact_btn, 12);
    lv_style_set_height(&style_compact_btn, LV_SIZE_CONTENT);
    lv_style_set_border_width(&style_compact_btn, 0);

    lv_style_init(&ui_comp_style_nes_cjk);
    lv_style_set_text_font(&ui_comp_style_nes_cjk, &nes_font_16);
}

lv_obj_t * ui_comp_create_styled_list_btn(lv_obj_t * parent, const char * text)
{
    lv_obj_t * btn = lv_list_add_btn(parent, NULL, text);
    lv_obj_add_style(btn, &style_compact_btn, 0);
    return btn;
}

lv_obj_t * ui_comp_create_nes_list_btn(lv_obj_t * parent, const char * text)
{
    lv_obj_t * btn = lv_list_add_btn(parent, NULL, text);
    lv_obj_add_style(btn, &style_compact_btn, 0);
    /* Try to set the label font inside the list button to nes_font_16 */
    lv_obj_t * lbl = lv_obj_get_child(btn, 0);
    if (lbl) {
        lv_obj_set_style_text_font(lbl, &nes_font_16, 0);
    }
    return btn;
}

void ui_comp_create_generic_option_page(const char* title, const char** options, lv_event_cb_t event_cb, lv_obj_t* parent_to_hide, lv_event_cb_t close_cb)
{
    if (parent_to_hide) {
        lv_obj_add_flag(parent_to_hide, LV_OBJ_FLAG_HIDDEN);
    }

    lv_obj_t * page = lv_obj_create(lv_scr_act());
    lv_obj_set_size(page, LV_HOR_RES, LV_VER_RES);
    lv_obj_add_event_cb(page, close_cb, LV_EVENT_DELETE, NULL);
    lv_obj_set_style_pad_all(page, 0, 0);

    lv_obj_t * list = lv_list_create(page);
    lv_obj_set_size(list, 280, 200);
    lv_obj_align(list, LV_ALIGN_CENTER, 0, 10);

    lv_obj_t* title_lbl = lv_list_add_text(list, title);
    lv_obj_set_style_text_font(title_lbl, &lv_font_montserrat_12, 0);

    lv_group_t * g = lv_group_get_default();
    for (int i = 0; options[i] != NULL; i++) {
        lv_obj_t* btn = ui_comp_create_styled_list_btn(list, options[i]);
        lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, page);
        lv_group_add_obj(g, btn);
    }

    if (lv_obj_get_child_cnt(list) > 1) {
        lv_group_focus_obj(lv_obj_get_child(list, 1));
    }
}

void ui_comp_generic_delete_event_cb(lv_event_t * e)
{
    lv_obj_t * obj_to_delete = lv_event_get_user_data(e);
    if (obj_to_delete) {
        lv_obj_del(obj_to_delete);
    }
}
