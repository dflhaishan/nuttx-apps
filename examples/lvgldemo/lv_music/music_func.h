#ifndef MUSIC_FUNC_H
#define MUSIC_FUNC_H

#include <lvgl/lvgl.h>

#  define LV_DEMO_MUSIC_HANDLE_SIZE  20

extern const lv_image_dsc_t img_lv_demo_music_list_border;

extern const lv_image_dsc_t img_lv_demo_music_btn_list_play;

extern const lv_image_dsc_t img_lv_demo_music_btn_list_pause;

extern const lv_image_dsc_t img_lv_demo_music_corner_left;

extern const lv_image_dsc_t img_lv_demo_music_corner_right;

extern const lv_image_dsc_t img_lv_demo_music_wave_bottom;

extern const lv_image_dsc_t img_lv_demo_music_wave_top;

extern const lv_image_dsc_t img_lv_demo_music_cover_1;

extern const lv_image_dsc_t img_lv_demo_music_cover_2;

extern const lv_image_dsc_t img_lv_demo_music_cover_3;

extern const lv_image_dsc_t img_lv_demo_music_btn_loop;

extern const lv_image_dsc_t img_lv_demo_music_btn_next;

extern const lv_image_dsc_t img_lv_demo_music_btn_pause;

extern const lv_image_dsc_t img_lv_demo_music_btn_play;

extern const lv_image_dsc_t img_lv_demo_music_btn_prev;

extern const lv_image_dsc_t img_lv_demo_music_btn_rnd;

extern const lv_image_dsc_t img_lv_demo_music_icon_1;

extern const lv_image_dsc_t img_lv_demo_music_icon_2;

extern const lv_image_dsc_t img_lv_demo_music_icon_3;

extern const lv_image_dsc_t img_lv_demo_music_icon_4;

extern const lv_image_dsc_t img_lv_demo_music_slider_knob;

extern const lv_image_dsc_t img_lv_demo_music_logo;

extern const uint16_t spectrum_1[445][4];

extern const uint16_t spectrum_2[780][4];

extern const uint16_t spectrum_3[1005][4];


lv_obj_t * lv_demo_music_list_create(lv_obj_t * parent);

lv_obj_t * lv_demo_music_main_create(lv_obj_t * parent);

void lv_demo_music_list_button_check(uint32_t track_id, bool state);

const char * lv_demo_music_get_title(uint32_t track_id);

const char * lv_demo_music_get_artist(uint32_t track_id);

const char * lv_demo_music_get_genre(uint32_t track_id);

uint32_t lv_demo_music_get_track_length(uint32_t track_id);

void lv_demo_music_play(uint32_t id);

void lv_demo_music_album_next(bool next);

void lv_demo_music_pause(void);

void lv_demo_music_resume(void);

void lv_apps_creat(void);

#endif