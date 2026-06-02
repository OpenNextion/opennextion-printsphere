#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/i2c_types.h"
#include "esp_err.h"
#include "esp_lcd_panel_io.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ONX_LCD_H_RES 320
#define ONX_LCD_V_RES 480

#define ONX_PIN_LCD_SCLK GPIO_NUM_5
#define ONX_PIN_LCD_MOSI GPIO_NUM_1
#define ONX_PIN_LCD_DC GPIO_NUM_3
#define ONX_PIN_LCD_CS GPIO_NUM_2
#define ONX_PIN_LCD_BL GPIO_NUM_6

#define ONX_PIN_I2C_SCL GPIO_NUM_7
#define ONX_PIN_I2C_SDA GPIO_NUM_8
#define ONX_PIN_BAT_ADC GPIO_NUM_4

#define ONX_I2C_PORT I2C_NUM_0
#define ONX_PCF8574_ADDR 0x38
#define ONX_CST826_ADDR 0x15
#define ONX_BAT_ADC_UNIT 1
#define ONX_BAT_ADC_CHANNEL 3

typedef enum {
    ONX_PCF8574_PIN_I2S_CTRL = 1,
    ONX_PCF8574_PIN_RTC_INT = 2,
    ONX_PCF8574_PIN_CHG_N = 3,
    ONX_PCF8574_PIN_CAM_PWDN = 4,
    ONX_PCF8574_PIN_TP_INT = 5,
    ONX_PCF8574_PIN_LCD_RST = 6,
    ONX_PCF8574_PIN_SDCS = 7,
} onx_pcf8574_pin_t;

typedef struct {
    uint16_t x;
    uint16_t y;
    uint8_t points;
} onx_touch_point_t;

esp_err_t onx_bsp_i2c_init(void);
i2c_master_bus_handle_t onx_bsp_i2c_get_handle(void);
esp_err_t onx_bsp_pcf8574_init(void);
esp_err_t onx_bsp_pcf8574_write_pin(onx_pcf8574_pin_t pin, bool high);
esp_err_t onx_bsp_pcf8574_read(uint8_t *state);
esp_err_t onx_bsp_lcd_init(void);
esp_lcd_panel_io_handle_t onx_bsp_lcd_get_io_handle(void);
esp_err_t onx_bsp_lcd_fill(uint16_t rgb565);
esp_err_t onx_bsp_lcd_fill_bars(void);
esp_err_t onx_bsp_backlight_init(void);
esp_err_t onx_bsp_backlight_set(uint8_t brightness_percent);
uint8_t onx_bsp_backlight_get(void);
esp_err_t onx_bsp_battery_adc_read_raw(int *raw, int *adc_mv_uncalibrated);
esp_err_t onx_bsp_touch_init(void);
esp_err_t onx_bsp_touch_read(onx_touch_point_t *point);
esp_err_t onx_bsp_smoke_run(void);

#ifdef __cplusplus
}
#endif
