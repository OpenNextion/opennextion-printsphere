#pragma once

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_lcd_types.h"

/* LCD color formats */
#ifndef ESP_LCD_COLOR_FORMAT_RGB565
#define ESP_LCD_COLOR_FORMAT_RGB565 (1)
#endif
#ifndef ESP_LCD_COLOR_FORMAT_RGB888
#define ESP_LCD_COLOR_FORMAT_RGB888 (2)
#endif

/* ONX3248G035 ST7796U compile-time display definition. */
#if defined(PRINTSPHERE_ONX_ORIENTATION_LANDSCAPE) && PRINTSPHERE_ONX_ORIENTATION_LANDSCAPE
#define BSP_LCD_H_RES 480
#define BSP_LCD_V_RES 320
#else
#define BSP_LCD_H_RES 320
#define BSP_LCD_V_RES 480
#endif
#define BSP_LCD_COLOR_FORMAT (ESP_LCD_COLOR_FORMAT_RGB565)
#define BSP_LCD_BIGENDIAN 0
#define BSP_LCD_BITS_PER_PIXEL 16
#define BSP_LCD_TE_GPIO GPIO_NUM_NC

/*
 * Public touch interrupt shape kept for PrintSphere compatibility. ONX routes
 * TP_INT through PCF8574, not a direct ESP32-S3 GPIO.
 */
#define BSP_LCD_TOUCH_INT GPIO_NUM_NC

#ifdef ESP_LCD_COLOR_SPACE_RGB
#define BSP_LCD_COLOR_SPACE ESP_LCD_COLOR_SPACE_RGB
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int max_transfer_sz;
} bsp_display_config_t;

typedef enum {
    BSP_DISPLAY_ROTATE_0 = 0,
    BSP_DISPLAY_ROTATE_90 = 1,
    BSP_DISPLAY_ROTATE_180 = 2,
    BSP_DISPLAY_ROTATE_270 = 3,
} bsp_display_rotation_t;

esp_err_t bsp_display_new(const bsp_display_config_t *config,
                          esp_lcd_panel_handle_t *ret_panel,
                          esp_lcd_panel_io_handle_t *ret_io);
esp_err_t bsp_display_brightness_init(void);
esp_err_t bsp_display_rotation_set(bsp_display_rotation_t rotation);
esp_err_t bsp_display_brightness_set(int brightness_percent);
int bsp_display_brightness_get(void);
esp_err_t bsp_display_backlight_on(void);
esp_err_t bsp_display_backlight_off(void);

#ifdef __cplusplus
}
#endif
