#include <stdint.h>

#include "bsp/display.h"
#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_lcd_panel_io.h"
#include "onx3248g035_bsp.h"

typedef struct _lv_display_t lv_display_t;
typedef struct _lv_indev_t lv_indev_t;
typedef struct bsp_display_cfg_t bsp_display_cfg_t;
typedef struct sdmmc_card_t sdmmc_card_t;
typedef struct esp_lcd_touch *esp_lcd_touch_handle_t;

static const char *TAG = "onx_bsp_compat";

sdmmc_card_t *bsp_sdcard = NULL;

lv_display_t *bsp_display_start_with_config(bsp_display_cfg_t *cfg);

esp_err_t bsp_i2c_init(void)
{
    return onx_bsp_i2c_init();
}

esp_err_t bsp_i2c_deinit(void)
{
    ESP_LOGW(TAG, "bsp_i2c_deinit is not supported on ONX M3 skeleton");
    return ESP_ERR_NOT_SUPPORTED;
}

i2c_master_bus_handle_t bsp_i2c_get_handle(void)
{
    return onx_bsp_i2c_get_handle();
}

esp_err_t bsp_display_new(const bsp_display_config_t *config,
                          esp_lcd_panel_handle_t *ret_panel,
                          esp_lcd_panel_io_handle_t *ret_io)
{
    (void)config;

    if (ret_panel) {
        *ret_panel = NULL;
    }
    if (ret_io) {
        *ret_io = NULL;
    }

    ESP_RETURN_ON_ERROR(onx_bsp_lcd_init(), TAG, "ONX LCD init failed");
    if (ret_io) {
        *ret_io = onx_bsp_lcd_get_io_handle();
    }

    ESP_LOGW(TAG, "esp_lcd_panel_handle_t path is not implemented yet for ONX");
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t bsp_display_brightness_init(void)
{
    return onx_bsp_backlight_init();
}

esp_err_t bsp_display_rotation_set(bsp_display_rotation_t rotation)
{
    if (rotation != BSP_DISPLAY_ROTATE_0) {
        ESP_LOGW(TAG, "ONX M3 skeleton only supports verified portrait rotation");
        return ESP_ERR_NOT_SUPPORTED;
    }

    return ESP_OK;
}

esp_err_t bsp_display_brightness_set(int brightness_percent)
{
    if (brightness_percent < 0 || brightness_percent > 100) {
        return ESP_ERR_INVALID_ARG;
    }

    return onx_bsp_backlight_set((uint8_t)brightness_percent);
}

int bsp_display_brightness_get(void)
{
    return (int)onx_bsp_backlight_get();
}

esp_err_t bsp_display_backlight_on(void)
{
    return bsp_display_brightness_set(100);
}

esp_err_t bsp_display_backlight_off(void)
{
    return bsp_display_brightness_set(0);
}

lv_display_t *bsp_display_start(void)
{
    return bsp_display_start_with_config(NULL);
}

lv_display_t *bsp_display_start_with_config(bsp_display_cfg_t *cfg)
{
    (void)cfg;

    ESP_LOGW(TAG, "ONX LVGL display adapter path is not implemented yet");
    ESP_LOGW(TAG, "Returning NULL so PrintSphere startup treats display as unavailable");
    ESP_ERROR_CHECK_WITHOUT_ABORT(onx_bsp_lcd_init());
    ESP_ERROR_CHECK_WITHOUT_ABORT(onx_bsp_backlight_init());
    return NULL;
}

lv_indev_t *bsp_display_get_input_dev(void)
{
    return NULL;
}

esp_err_t bsp_display_lock(uint32_t timeout_ms)
{
    (void)timeout_ms;
    ESP_LOGW(TAG, "LVGL lock unavailable before ONX adapter path is implemented");
    return ESP_ERR_INVALID_STATE;
}

void bsp_display_unlock(void)
{
}

esp_err_t bsp_touch_new(const bsp_display_cfg_t *cfg, esp_lcd_touch_handle_t *ret_touch)
{
    (void)cfg;

    if (ret_touch) {
        *ret_touch = NULL;
    }

    ESP_RETURN_ON_ERROR(onx_bsp_touch_init(), TAG, "ONX raw CST826 init failed");
    ESP_LOGW(TAG, "esp_lcd_touch_handle_t CST826 path is not implemented yet for ONX");
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t bsp_sdcard_mount(void)
{
    ESP_LOGW(TAG, "SD card is not validated on ONX M3 skeleton");
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t bsp_sdcard_unmount(void)
{
    return ESP_OK;
}
