#pragma once

#include "esp_err.h"

#if __has_include("esp_lcd_touch.h")
#include "esp_lcd_touch.h"
#else
typedef struct esp_lcd_touch *esp_lcd_touch_handle_t;
#endif

typedef struct bsp_display_cfg_t bsp_display_cfg_t;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void *dummy;
} bsp_touch_config_t;

esp_err_t bsp_touch_new(const bsp_display_cfg_t *cfg, esp_lcd_touch_handle_t *ret_touch);

#ifdef __cplusplus
}
#endif
