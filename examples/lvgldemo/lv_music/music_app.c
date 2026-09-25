#include "music_func.h"

// static void auto_step_cb(lv_timer_t * t);

static lv_obj_t * ctrl;
static lv_obj_t * list;

void lv_apps_creat(void)
{
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x343247), 0);
    list = lv_demo_music_list_create(lv_screen_active());
    ctrl = lv_demo_music_main_create(lv_screen_active());
    // lv_timer_create(auto_step_cb, 1000, NULL);
}
#if 0
static void auto_step_cb(lv_timer_t * t)
{
    LV_UNUSED(t);
    static uint32_t state = 0;

#if LV_DEMO_MUSIC_LARGE
    const lv_font_t * font_small = &lv_font_montserrat_22;
    const lv_font_t * font_large = &lv_font_montserrat_32;
#else
    const lv_font_t * font_small = &lv_font_montserrat_14;
    const lv_font_t * font_large = &lv_font_montserrat_14;
#endif

    switch(state) {
        case 5:
            lv_demo_music_album_next(true);
            break;

        case 6:
            lv_demo_music_album_next(true);
            break;
        case 7:
            lv_demo_music_album_next(true);
            break;
        case 8:
            lv_demo_music_play(0);
            break;
#if LV_DEMO_MUSIC_SQUARE || LV_DEMO_MUSIC_ROUND
        case 11:
            lv_obj_scroll_by(ctrl, 0, -LV_VER_RES, LV_ANIM_ON);
            break;
        case 13:
            lv_obj_scroll_by(ctrl, 0, -LV_VER_RES, LV_ANIM_ON);
            break;
#else
        case 12:
            lv_obj_scroll_by(ctrl, 0, -LV_VER_RES, LV_ANIM_ON);
            break;
#endif
        case 15:
            lv_obj_scroll_by(list, 0, -300, LV_ANIM_ON);
            break;
        case 16:
            lv_obj_scroll_by(list, 0, 300, LV_ANIM_ON);
            break;
        case 18:
            lv_demo_music_play(1);
            break;
        case 19:
            lv_obj_scroll_by(ctrl, 0, LV_VER_RES, LV_ANIM_ON);
            break;
#if LV_DEMO_MUSIC_SQUARE || LV_DEMO_MUSIC_ROUND
        case 20:
            lv_obj_scroll_by(ctrl, 0, LV_VER_RES, LV_ANIM_ON);
            break;
#endif
        case 30:
            lv_demo_music_play(2);
            break;
        case 40: {
                lv_obj_t * bg = lv_layer_top();
                lv_obj_set_style_bg_color(bg, lv_color_hex(0x6f8af6), 0);
                lv_obj_set_style_text_color(bg, lv_color_white(), 0);
                lv_obj_set_style_bg_opa(bg, LV_OPA_COVER, 0);
                lv_obj_fade_in(bg, 400, 0);
                lv_obj_t * dsc = lv_label_create(bg);
                lv_obj_set_style_text_font(dsc, font_small, 0);
                lv_label_set_text(dsc, "The average FPS is");
                lv_obj_align(dsc, LV_ALIGN_TOP_MID, 0, 90);

                lv_obj_t * num = lv_label_create(bg);
                lv_obj_set_style_text_font(num, font_large, 0);
#if LV_USE_PERF_MONITOR
                lv_display_t * disp = lv_display_get_default();
                const lv_sysmon_perf_info_t * info = lv_subject_get_pointer(&disp->perf_sysmon_backend.subject);
                lv_label_set_text_fmt(num, "%" LV_PRIu32, info->calculated.fps_avg_total);
#endif
                lv_obj_align(num, LV_ALIGN_TOP_MID, 0, 120);

                lv_obj_t * attr = lv_label_create(bg);
                lv_obj_set_style_text_align(attr, LV_TEXT_ALIGN_CENTER, 0);
                lv_obj_set_style_text_font(attr, font_small, 0);
#if LV_DEMO_MUSIC_SQUARE || LV_DEMO_MUSIC_ROUND
                lv_label_set_text(attr, "Copyright 2020 LVGL Kft.\nwww.lvgl.io | lvgl@lvgl.io");
#else
                lv_label_set_text(attr, "Copyright 2020 LVGL Kft. | www.lvgl.io | lvgl@lvgl.io");
#endif
                lv_obj_align(attr, LV_ALIGN_BOTTOM_MID, 0, -10);
                break;
            }
        case 41:
            lv_screen_load(lv_obj_create(NULL));
            lv_demo_music_pause();
            break;
    }
    state++;
}
#endif