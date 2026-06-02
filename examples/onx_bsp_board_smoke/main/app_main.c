#include <stdint.h>

#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "onx3248g035_bsp.h"

static const char *TAG = "onx_board_smoke";

static int pcf_pin_level(uint8_t state, onx_pcf8574_pin_t pin)
{
    return (state >> (pin - 1)) & 0x01;
}

static esp_err_t read_and_log_pcf8574(const char *label, uint8_t *state)
{
    ESP_RETURN_ON_ERROR(onx_bsp_pcf8574_read(state), TAG, "PCF8574 read failed");
    ESP_LOGI(TAG, "%s PCF8574 raw state=0x%02X", label, *state);
    ESP_LOGI(TAG, "PCF8574 pin map: pin1 I2S_CTRL=%d pin2 RTC_INT=%d pin3 CHG_N=%d pin4 CAM_PWDN=%d pin5 TP_INT=%d pin6 LCD_RST=%d pin7 SDCS=%d",
             pcf_pin_level(*state, ONX_PCF8574_PIN_I2S_CTRL),
             pcf_pin_level(*state, ONX_PCF8574_PIN_RTC_INT),
             pcf_pin_level(*state, ONX_PCF8574_PIN_CHG_N),
             pcf_pin_level(*state, ONX_PCF8574_PIN_CAM_PWDN),
             pcf_pin_level(*state, ONX_PCF8574_PIN_TP_INT),
             pcf_pin_level(*state, ONX_PCF8574_PIN_LCD_RST),
             pcf_pin_level(*state, ONX_PCF8574_PIN_SDCS));
    return ESP_OK;
}

static void log_touch_sample(void)
{
    esp_err_t err = onx_bsp_touch_init();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Touch polling deferred: init failed err=%s", esp_err_to_name(err));
        return;
    }

    onx_touch_point_t point = {};
    err = onx_bsp_touch_read(&point);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Touch polling sample: points=%u x=%u y=%u", point.points, point.x, point.y);
    } else {
        ESP_LOGW(TAG, "Touch polling sample failed: %s", esp_err_to_name(err));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "ONX3248G035 board diagnostics smoke start");
    ESP_LOGI(TAG, "Scope: board-level raw diagnostics only; no PrintSphere app, Wi-Fi, Bambu, Web Config, OTA, camera, or SD mount");

    ESP_ERROR_CHECK(onx_bsp_i2c_init());
    ESP_LOGI(TAG, "I2C bus ready evidence: port=%d SDA=GPIO%d SCL=GPIO%d",
             ONX_I2C_PORT, ONX_PIN_I2C_SDA, ONX_PIN_I2C_SCL);

    ESP_ERROR_CHECK(onx_bsp_pcf8574_init());
    ESP_ERROR_CHECK(onx_bsp_pcf8574_write_pin(ONX_PCF8574_PIN_SDCS, true));
    ESP_LOGI(TAG, "SDCS policy: PCF8574 pin7 held high; SD mount deferred in this smoke");

    uint8_t pcf_state = 0;
    ESP_ERROR_CHECK(read_and_log_pcf8574("Initial", &pcf_state));
    ESP_LOGI(TAG, "CHG_N raw: level=%d assumption=active_low_pending_validation no_charging_boolean_reported",
             pcf_pin_level(pcf_state, ONX_PCF8574_PIN_CHG_N));
    ESP_LOGI(TAG, "TP_INT raw: level=%d source=PCF8574 pin5 touch remains polling in this smoke",
             pcf_pin_level(pcf_state, ONX_PCF8574_PIN_TP_INT));
    ESP_LOGI(TAG, "SDCS raw: level=%d expected=1 high_not_selected",
             pcf_pin_level(pcf_state, ONX_PCF8574_PIN_SDCS));

    int raw = 0;
    int adc_mv_uncalibrated = 0;
    esp_err_t adc_err = onx_bsp_battery_adc_read_raw(&raw, &adc_mv_uncalibrated);
    if (adc_err == ESP_OK) {
        ESP_LOGI(TAG, "Battery ADC raw-only: gpio=GPIO%d adc1_ch%d raw=%d adc_mv_uncalibrated=%d divider_unknown=true percent_deferred=true",
                 ONX_PIN_BAT_ADC, ONX_BAT_ADC_CHANNEL, raw, adc_mv_uncalibrated);
    } else {
        ESP_LOGW(TAG, "Battery ADC raw-only read failed: %s", esp_err_to_name(adc_err));
    }

    ESP_ERROR_CHECK(onx_bsp_backlight_init());
    ESP_ERROR_CHECK(onx_bsp_backlight_set(30));
    ESP_LOGI(TAG, "Backlight check: set=30 get=%u", onx_bsp_backlight_get());
    vTaskDelay(pdMS_TO_TICKS(300));
    ESP_ERROR_CHECK(onx_bsp_backlight_set(100));
    ESP_LOGI(TAG, "Backlight check: set=100 get=%u", onx_bsp_backlight_get());
    vTaskDelay(pdMS_TO_TICKS(300));
    ESP_ERROR_CHECK(onx_bsp_backlight_set(30));
    ESP_LOGI(TAG, "Backlight check: set=30 get=%u", onx_bsp_backlight_get());

    log_touch_sample();

    ESP_ERROR_CHECK(read_and_log_pcf8574("Final", &pcf_state));
    ESP_LOGI(TAG, "Board diagnostics smoke complete: raw-only battery/charge/TP_INT, SDCS high, no SD mount attempted");

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        ESP_LOGI(TAG, "I2C bus ready evidence: port=%d SDA=GPIO%d SCL=GPIO%d",
                 ONX_I2C_PORT, ONX_PIN_I2C_SDA, ONX_PIN_I2C_SCL);
        esp_err_t pcf_err = onx_bsp_pcf8574_read(&pcf_state);
        if (pcf_err == ESP_OK) {
            ESP_LOGI(TAG, "Board diagnostics still running: pcf8574=0x%02X CHG_N_raw=%d TP_INT_raw=%d SDCS_raw=%d backlight=%u",
                     pcf_state,
                     pcf_pin_level(pcf_state, ONX_PCF8574_PIN_CHG_N),
                     pcf_pin_level(pcf_state, ONX_PCF8574_PIN_TP_INT),
                     pcf_pin_level(pcf_state, ONX_PCF8574_PIN_SDCS),
                     onx_bsp_backlight_get());
            ESP_LOGI(TAG, "PCF8574 pin map: pin1 I2S_CTRL=%d pin2 RTC_INT=%d pin3 CHG_N=%d pin4 CAM_PWDN=%d pin5 TP_INT=%d pin6 LCD_RST=%d pin7 SDCS=%d",
                     pcf_pin_level(pcf_state, ONX_PCF8574_PIN_I2S_CTRL),
                     pcf_pin_level(pcf_state, ONX_PCF8574_PIN_RTC_INT),
                     pcf_pin_level(pcf_state, ONX_PCF8574_PIN_CHG_N),
                     pcf_pin_level(pcf_state, ONX_PCF8574_PIN_CAM_PWDN),
                     pcf_pin_level(pcf_state, ONX_PCF8574_PIN_TP_INT),
                     pcf_pin_level(pcf_state, ONX_PCF8574_PIN_LCD_RST),
                     pcf_pin_level(pcf_state, ONX_PCF8574_PIN_SDCS));
            ESP_LOGI(TAG, "Backlight sequence evidence: set/get 30->100->30 completed; current get=%u",
                     onx_bsp_backlight_get());
        } else {
            ESP_LOGW(TAG, "Board diagnostics still running: PCF8574 read failed: %s", esp_err_to_name(pcf_err));
        }

        adc_err = onx_bsp_battery_adc_read_raw(&raw, &adc_mv_uncalibrated);
        if (adc_err == ESP_OK) {
            ESP_LOGI(TAG, "Battery ADC repeat raw-only: gpio=GPIO%d adc1_ch%d raw=%d adc_mv_uncalibrated=%d divider_unknown=true percent_deferred=true",
                     ONX_PIN_BAT_ADC, ONX_BAT_ADC_CHANNEL, raw, adc_mv_uncalibrated);
        }
    }
}
