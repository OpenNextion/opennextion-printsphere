#include "bsp/esp32_s3_touch_amoled_1_75.h"
#include "esp_log.h"
#include "lvgl.h"
#include <stdio.h>

static const char *TAG = "onx_bsp_lvgl_smoke";

static lv_obj_t *s_touch_status;

static const char *target_for_point(int32_t x, int32_t y)
{
    if (x <= 110 && y <= 130) {
        return "TOUCH TOP LEFT";
    }
    if (x >= 210 && y >= 350) {
        return "TOUCH BOTTOM RIGHT";
    }
    if (x >= 105 && x <= 215 && y >= 185 && y <= 295) {
        return "TOUCH CENTER";
    }
    return "UNMARKED";
}

static void screen_touch_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_PRESSED) {
        return;
    }

    lv_indev_t *indev = lv_indev_get_act();
    if (indev == NULL) {
        return;
    }

    lv_point_t point = {};
    lv_indev_get_point(indev, &point);
    const char *target = target_for_point(point.x, point.y);
    ESP_LOGI(TAG, "Touch x=%ld y=%ld target=%s", (long)point.x, (long)point.y, target);

    if (s_touch_status) {
        char text[96];
        snprintf(text, sizeof(text), "%s\nx=%ld y=%ld", target, (long)point.x, (long)point.y);
        lv_label_set_text(s_touch_status, text);
    }
}

static void create_color_band(lv_obj_t *parent,
                              const char *label_text,
                              uint32_t bg_color,
                              uint32_t text_color,
                              int y,
                              int height)
{
    lv_obj_t *band = lv_obj_create(parent);
    lv_obj_set_size(band, BSP_LCD_H_RES, height);
    lv_obj_set_pos(band, 0, y);
    lv_obj_set_style_radius(band, 0, 0);
    lv_obj_set_style_border_width(band, 0, 0);
    lv_obj_set_style_pad_all(band, 0, 0);
    lv_obj_set_style_bg_color(band, lv_color_hex(bg_color), 0);
    lv_obj_set_style_bg_opa(band, LV_OPA_COVER, 0);
    lv_obj_clear_flag(band, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *label = lv_label_create(band);
    lv_label_set_text(label, label_text);
    lv_obj_set_style_text_color(label, lv_color_hex(text_color), 0);
    lv_obj_set_style_text_font(label, LV_FONT_DEFAULT, 0);
    lv_obj_center(label);
}

static void create_touch_target(lv_obj_t *parent,
                                const char *label_text,
                                lv_align_t align,
                                int x_offset,
                                int y_offset)
{
    lv_obj_t *box = lv_obj_create(parent);
    lv_obj_set_size(box, 112, 56);
    lv_obj_align(box, align, x_offset, y_offset);
    lv_obj_set_style_radius(box, 0, 0);
    lv_obj_set_style_bg_color(box, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(box, LV_OPA_70, 0);
    lv_obj_set_style_border_color(box, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(box, 2, 0);
    lv_obj_set_style_pad_all(box, 4, 0);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(box, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *label = lv_label_create(box);
    lv_label_set_text(label, label_text);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label, 100);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(label);
}

static void create_validation_screen(void)
{
    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(screen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(screen, screen_touch_cb, LV_EVENT_PRESSED, NULL);

    create_color_band(screen, "RED", 0xFF0000, 0xFFFFFF, 0, 72);
    create_color_band(screen, "GREEN", 0x00FF00, 0x000000, 72, 72);
    create_color_band(screen, "BLUE", 0x0000FF, 0xFFFFFF, 144, 72);
    create_color_band(screen, "WHITE", 0xFFFFFF, 0x000000, 216, 72);
    create_color_band(screen, "BLACK", 0x000000, 0xFFFFFF, 288, 72);

    create_touch_target(screen, "TOUCH\nTOP LEFT", LV_ALIGN_TOP_LEFT, 8, 8);
    create_touch_target(screen, "TOUCH\nCENTER", LV_ALIGN_CENTER, 0, 16);
    create_touch_target(screen, "TOUCH\nBOTTOM RIGHT", LV_ALIGN_BOTTOM_RIGHT, -8, -8);

    s_touch_status = lv_label_create(screen);
    lv_label_set_text(s_touch_status, "Touch a labeled target");
    lv_label_set_long_mode(s_touch_status, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s_touch_status, 170);
    lv_obj_set_style_text_align(s_touch_status, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(s_touch_status, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(s_touch_status, LV_ALIGN_BOTTOM_LEFT, 8, -20);
}

void app_main(void)
{
    bsp_display_cfg_t cfg = {
        .lv_adapter_cfg = ESP_LV_ADAPTER_DEFAULT_CONFIG(),
        .rotation = ESP_LV_ADAPTER_ROTATE_0,
        .tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_NONE,
        .touch_flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };

    lv_display_t *display = bsp_display_start_with_config(&cfg);
    lv_indev_t *input = bsp_display_get_input_dev();

    ESP_LOGI(TAG, "display=%p input=%p", display, input);
    if (display == NULL) {
        ESP_LOGE(TAG, "ONX BSP LVGL display did not start");
        return;
    }

    if (bsp_display_lock(2000) != ESP_OK) {
        ESP_LOGE(TAG, "LVGL lock failed; validation screen not created");
        return;
    }
    create_validation_screen();
    bsp_display_unlock();

    ESP_LOGI(TAG, "LVGL validation screen ready: labeled color bands and touch targets");
}
