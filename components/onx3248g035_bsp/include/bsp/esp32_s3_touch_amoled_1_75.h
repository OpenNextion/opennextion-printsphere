#pragma once

#include "sdkconfig.h"

#include "bsp/config.h"
#include "bsp/display.h"
#include "bsp/touch.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"

#if __has_include("driver/sdmmc_host.h")
#include "driver/sdmmc_host.h"
#else
typedef struct sdmmc_card_t sdmmc_card_t;
#endif

#include "hal/spi_types.h"
#include "esp_err.h"

#if __has_include("esp_lv_adapter.h")
#include "esp_lv_adapter.h"
#else
typedef struct {
    int task_core_id;
} esp_lv_adapter_config_t;

typedef enum {
    ESP_LV_ADAPTER_ROTATE_0 = 0,
    ESP_LV_ADAPTER_ROTATE_90 = 1,
    ESP_LV_ADAPTER_ROTATE_180 = 2,
    ESP_LV_ADAPTER_ROTATE_270 = 3,
} esp_lv_adapter_rotation_t;

typedef enum {
    ESP_LV_ADAPTER_TEAR_AVOID_MODE_NONE = 0,
    ESP_LV_ADAPTER_TEAR_AVOID_MODE_TE_SYNC = 1,
} esp_lv_adapter_tear_avoid_mode_t;

#define ESP_LV_ADAPTER_DEFAULT_CONFIG() ((esp_lv_adapter_config_t){0})
#endif

#if __has_include("lvgl.h")
#include "lvgl.h"
#else
typedef struct _lv_display_t lv_display_t;
typedef struct _lv_indev_t lv_indev_t;
#endif

#include "onx3248g035_bsp.h"

/*
 * Temporary compatibility header for PrintSphere M3.
 *
 * The application still includes the Waveshare BSP header path. For ONX builds,
 * Build/Release must select only this component so the same include path resolves
 * to the ONX profile.
 */

#define BSP_CAPS_DISPLAY 1
#define BSP_CAPS_TOUCH 1
#define BSP_CAPS_BUTTONS 0
#define BSP_CAPS_AUDIO 0
#define BSP_CAPS_AUDIO_SPEAKER 0
#define BSP_CAPS_AUDIO_MIC 0
#define BSP_CAPS_SDCARD 0
#define BSP_CAPS_IMU 0

#define BSP_I2C_NUM ONX_I2C_PORT
#define BSP_I2C_SCL ONX_PIN_I2C_SCL
#define BSP_I2C_SDA ONX_PIN_I2C_SDA

#define BSP_LCD_SPI_NUM SPI2_HOST
#define BSP_LCD_CS ONX_PIN_LCD_CS
#define BSP_LCD_PCLK ONX_PIN_LCD_SCLK
#define BSP_LCD_DATA0 ONX_PIN_LCD_MOSI
#define BSP_LCD_BACKLIGHT ONX_PIN_LCD_BL
#define BSP_LCD_RST GPIO_NUM_NC
#define BSP_LCD_TOUCH_RST GPIO_NUM_NC

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t bsp_i2c_init(void);
esp_err_t bsp_i2c_deinit(void);
i2c_master_bus_handle_t bsp_i2c_get_handle(void);

extern sdmmc_card_t *bsp_sdcard;
esp_err_t bsp_sdcard_mount(void);
esp_err_t bsp_sdcard_unmount(void);

#if (BSP_CONFIG_NO_GRAPHIC_LIB == 0)
typedef struct bsp_display_cfg_t {
    esp_lv_adapter_config_t lv_adapter_cfg;
    esp_lv_adapter_rotation_t rotation;
    esp_lv_adapter_tear_avoid_mode_t tear_avoid_mode;
    struct {
        unsigned int swap_xy;
        unsigned int mirror_x;
        unsigned int mirror_y;
    } touch_flags;
} bsp_display_cfg_t;

lv_display_t *bsp_display_start(void);
lv_display_t *bsp_display_start_with_config(bsp_display_cfg_t *cfg);
lv_indev_t *bsp_display_get_input_dev(void);
esp_err_t bsp_display_lock(uint32_t timeout_ms);
void bsp_display_unlock(void);
#endif

#ifdef __cplusplus
}
#endif
