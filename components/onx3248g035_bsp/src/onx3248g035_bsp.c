#include "onx3248g035_bsp.h"

#include <inttypes.h>
#include <string.h>

#include "driver/i2c_master.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_lcd_io_spi.h"
#include "esp_lcd_panel_commands.h"
#include "esp_lcd_panel_io.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "onx_bsp";

#define ONX_LCD_HOST SPI2_HOST
#define ONX_LCD_PCLK_HZ (80 * 1000 * 1000)
#define ONX_LCD_SPI_MAX_TRANSFER (ONX_LCD_H_RES * 32 * 2)

#define ONX_BL_LEDC_TIMER LEDC_TIMER_1
#define ONX_BL_LEDC_MODE LEDC_LOW_SPEED_MODE
#define ONX_BL_LEDC_CHANNEL LEDC_CHANNEL_0
#define ONX_BL_LEDC_DUTY_RES LEDC_TIMER_10_BIT
#define ONX_BL_LEDC_MAX_DUTY ((1U << 10) - 1U)
#define ONX_BL_LEDC_FREQ_HZ 10000

#define CST826_REG_DATA_START 0x02
#define CST826_REG_CHIP_ID 0xAA

typedef struct {
    uint8_t cmd;
    const uint8_t *data;
    uint8_t data_len;
    uint16_t delay_ms;
} onx_lcd_init_cmd_t;

static i2c_master_bus_handle_t s_i2c_bus;
static i2c_master_dev_handle_t s_pcf8574_dev;
static i2c_master_dev_handle_t s_cst826_dev;
static esp_lcd_panel_io_handle_t s_lcd_io;
static uint8_t s_pcf8574_state = 0xFF;
static bool s_pcf8574_ready;
static bool s_spi_bus_ready;
static bool s_lcd_ready;
static bool s_backlight_ready;
static bool s_touch_ready;

static const uint8_t s_cmd_colmod[] = {0x55};
static const uint8_t s_cmd_f0_c3[] = {0xC3};
static const uint8_t s_cmd_f0_96[] = {0x96};
static const uint8_t s_cmd_b4[] = {0x01};
static const uint8_t s_cmd_b7[] = {0xC6};
static const uint8_t s_cmd_c0[] = {0x80, 0x45};
static const uint8_t s_cmd_c1[] = {0x13};
static const uint8_t s_cmd_c2[] = {0xA7};
static const uint8_t s_cmd_c5[] = {0x0A};
static const uint8_t s_cmd_e8[] = {0x40, 0x8A, 0x00, 0x00, 0x29, 0x19, 0xA5, 0x33};
static const uint8_t s_cmd_e0[] = {
    0xD0, 0x08, 0x0F, 0x06, 0x06, 0x33, 0x30,
    0x33, 0x47, 0x17, 0x13, 0x13, 0x2B, 0x31,
};
static const uint8_t s_cmd_e1[] = {
    0xD0, 0x0A, 0x11, 0x0B, 0x09, 0x07, 0x2F,
    0x33, 0x47, 0x38, 0x15, 0x16, 0x2C, 0x32,
};
static const uint8_t s_cmd_f0_3c[] = {0x3C};
static const uint8_t s_cmd_f0_69[] = {0x69};

static const onx_lcd_init_cmd_t s_lcd_init_cmds[] = {
    {LCD_CMD_SLPOUT, NULL, 0, 120},
    {LCD_CMD_COLMOD, s_cmd_colmod, sizeof(s_cmd_colmod), 0},
    {0xF0, s_cmd_f0_c3, sizeof(s_cmd_f0_c3), 0},
    {0xF0, s_cmd_f0_96, sizeof(s_cmd_f0_96), 0},
    {0xB4, s_cmd_b4, sizeof(s_cmd_b4), 0},
    {0xB7, s_cmd_b7, sizeof(s_cmd_b7), 0},
    {0xC0, s_cmd_c0, sizeof(s_cmd_c0), 0},
    {0xC1, s_cmd_c1, sizeof(s_cmd_c1), 0},
    {0xC2, s_cmd_c2, sizeof(s_cmd_c2), 0},
    {0xC5, s_cmd_c5, sizeof(s_cmd_c5), 0},
    {0xE8, s_cmd_e8, sizeof(s_cmd_e8), 0},
    {0xE0, s_cmd_e0, sizeof(s_cmd_e0), 0},
    {0xE1, s_cmd_e1, sizeof(s_cmd_e1), 0},
    {0xF0, s_cmd_f0_3c, sizeof(s_cmd_f0_3c), 0},
    {0xF0, s_cmd_f0_69, sizeof(s_cmd_f0_69), 120},
    {LCD_CMD_INVON, NULL, 0, 0},
    {LCD_CMD_DISPON, NULL, 0, 0},
};

static esp_err_t i2c_add_device(uint8_t addr, uint32_t speed_hz, i2c_master_dev_handle_t *dev)
{
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = speed_hz,
    };
    return i2c_master_bus_add_device(s_i2c_bus, &dev_cfg, dev);
}

esp_err_t onx_bsp_i2c_init(void)
{
    if (s_i2c_bus) {
        return ESP_OK;
    }

    i2c_master_bus_config_t i2c_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = ONX_I2C_PORT,
        .scl_io_num = ONX_PIN_I2C_SCL,
        .sda_io_num = ONX_PIN_I2C_SDA,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&i2c_cfg, &s_i2c_bus), TAG, "I2C init failed");
    ESP_LOGI(TAG, "I2C ready: port=%d sda=%d scl=%d", ONX_I2C_PORT, ONX_PIN_I2C_SDA, ONX_PIN_I2C_SCL);
    return ESP_OK;
}

esp_err_t onx_bsp_pcf8574_init(void)
{
    ESP_RETURN_ON_ERROR(onx_bsp_i2c_init(), TAG, "I2C init failed");
    if (s_pcf8574_ready) {
        return ESP_OK;
    }

    if (!s_pcf8574_dev) {
        ESP_RETURN_ON_ERROR(i2c_add_device(ONX_PCF8574_ADDR, 100000, &s_pcf8574_dev), TAG, "PCF8574 add failed");
    }

    s_pcf8574_state = 0xFF;
    ESP_RETURN_ON_ERROR(i2c_master_transmit(s_pcf8574_dev, &s_pcf8574_state, 1, 1000), TAG, "PCF8574 write failed");
    s_pcf8574_ready = true;
    ESP_LOGI(TAG, "PCF8574 ready: addr=0x%02X state=0x%02X", ONX_PCF8574_ADDR, s_pcf8574_state);
    return ESP_OK;
}

esp_err_t onx_bsp_pcf8574_write_pin(onx_pcf8574_pin_t pin, bool high)
{
    ESP_RETURN_ON_FALSE(pin >= 1 && pin <= 7, ESP_ERR_INVALID_ARG, TAG, "invalid PCF8574 pin");
    ESP_RETURN_ON_ERROR(onx_bsp_pcf8574_init(), TAG, "PCF8574 init failed");

    const uint8_t mask = (uint8_t)(1U << (pin - 1));
    if (high) {
        s_pcf8574_state |= mask;
    } else {
        s_pcf8574_state &= (uint8_t)~mask;
    }

    ESP_RETURN_ON_ERROR(i2c_master_transmit(s_pcf8574_dev, &s_pcf8574_state, 1, 1000), TAG, "PCF8574 pin write failed");
    ESP_LOGI(TAG, "PCF8574 pin %d=%d state=0x%02X", pin, high ? 1 : 0, s_pcf8574_state);
    return ESP_OK;
}

esp_err_t onx_bsp_pcf8574_read(uint8_t *state)
{
    ESP_RETURN_ON_FALSE(state != NULL, ESP_ERR_INVALID_ARG, TAG, "state is null");
    ESP_RETURN_ON_ERROR(onx_bsp_pcf8574_init(), TAG, "PCF8574 init failed");
    ESP_RETURN_ON_ERROR(i2c_master_receive(s_pcf8574_dev, state, 1, 1000), TAG, "PCF8574 read failed");
    return ESP_OK;
}

static esp_err_t lcd_send_cmds(void)
{
    for (size_t i = 0; i < sizeof(s_lcd_init_cmds) / sizeof(s_lcd_init_cmds[0]); ++i) {
        const onx_lcd_init_cmd_t *cmd = &s_lcd_init_cmds[i];
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(s_lcd_io, cmd->cmd, cmd->data, cmd->data_len), TAG, "LCD cmd failed");
        if (cmd->delay_ms > 0) {
            vTaskDelay(pdMS_TO_TICKS(cmd->delay_ms));
        }
    }
    return ESP_OK;
}

esp_err_t onx_bsp_lcd_init(void)
{
    if (s_lcd_ready) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(onx_bsp_pcf8574_init(), TAG, "PCF8574 init failed");
    ESP_RETURN_ON_ERROR(onx_bsp_pcf8574_write_pin(ONX_PCF8574_PIN_LCD_RST, false), TAG, "LCD reset low failed");
    vTaskDelay(pdMS_TO_TICKS(200));
    ESP_RETURN_ON_ERROR(onx_bsp_pcf8574_write_pin(ONX_PCF8574_PIN_LCD_RST, true), TAG, "LCD reset high failed");
    vTaskDelay(pdMS_TO_TICKS(200));

    if (!s_spi_bus_ready) {
        spi_bus_config_t buscfg = {
            .sclk_io_num = ONX_PIN_LCD_SCLK,
            .mosi_io_num = ONX_PIN_LCD_MOSI,
            .miso_io_num = GPIO_NUM_NC,
            .quadwp_io_num = GPIO_NUM_NC,
            .quadhd_io_num = GPIO_NUM_NC,
            .max_transfer_sz = ONX_LCD_SPI_MAX_TRANSFER,
        };
        ESP_RETURN_ON_ERROR(spi_bus_initialize(ONX_LCD_HOST, &buscfg, SPI_DMA_CH_AUTO), TAG, "SPI bus init failed");
        s_spi_bus_ready = true;
    }

    esp_lcd_panel_io_spi_config_t io_config = {
        .cs_gpio_num = ONX_PIN_LCD_CS,
        .dc_gpio_num = ONX_PIN_LCD_DC,
        .spi_mode = 0,
        .pclk_hz = ONX_LCD_PCLK_HZ,
        .trans_queue_depth = 5,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)ONX_LCD_HOST, &io_config, &s_lcd_io), TAG, "LCD IO init failed");
    ESP_RETURN_ON_ERROR(lcd_send_cmds(), TAG, "ST7796 init sequence failed");
    s_lcd_ready = true;
    ESP_LOGI(TAG, "LCD init complete: ST7796 SPI %dx%d pclk=%d", ONX_LCD_H_RES, ONX_LCD_V_RES, ONX_LCD_PCLK_HZ);
    return ESP_OK;
}

static esp_err_t lcd_draw_window(int x_start, int y_start, int x_end, int y_end, const void *color_data, size_t bytes)
{
    ESP_RETURN_ON_FALSE(s_lcd_ready, ESP_ERR_INVALID_STATE, TAG, "LCD not initialized");
    ESP_RETURN_ON_FALSE(x_start < x_end && y_start < y_end, ESP_ERR_INVALID_ARG, TAG, "invalid LCD window");

    uint8_t caset[] = {
        (uint8_t)((x_start >> 8) & 0xFF),
        (uint8_t)(x_start & 0xFF),
        (uint8_t)(((x_end - 1) >> 8) & 0xFF),
        (uint8_t)((x_end - 1) & 0xFF),
    };
    uint8_t raset[] = {
        (uint8_t)((y_start >> 8) & 0xFF),
        (uint8_t)(y_start & 0xFF),
        (uint8_t)(((y_end - 1) >> 8) & 0xFF),
        (uint8_t)((y_end - 1) & 0xFF),
    };

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(s_lcd_io, LCD_CMD_CASET, caset, sizeof(caset)), TAG, "CASET failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(s_lcd_io, LCD_CMD_RASET, raset, sizeof(raset)), TAG, "RASET failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_color(s_lcd_io, LCD_CMD_RAMWR, color_data, bytes), TAG, "RAMWR failed");
    return ESP_OK;
}

esp_err_t onx_bsp_lcd_fill(uint16_t rgb565)
{
    ESP_RETURN_ON_ERROR(onx_bsp_lcd_init(), TAG, "LCD init failed");

    const int rows = 16;
    const int pixels = ONX_LCD_H_RES * rows;
    uint16_t *buffer = heap_caps_malloc(pixels * sizeof(uint16_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    ESP_RETURN_ON_FALSE(buffer != NULL, ESP_ERR_NO_MEM, TAG, "LCD line buffer allocation failed");

    for (int i = 0; i < pixels; ++i) {
        buffer[i] = rgb565;
    }

    for (int y = 0; y < ONX_LCD_V_RES; y += rows) {
        const int chunk_rows = (y + rows <= ONX_LCD_V_RES) ? rows : (ONX_LCD_V_RES - y);
        esp_err_t err = lcd_draw_window(0, y, ONX_LCD_H_RES, y + chunk_rows, buffer,
                                        ONX_LCD_H_RES * chunk_rows * sizeof(uint16_t));
        if (err != ESP_OK) {
            free(buffer);
            ESP_LOGE(TAG, "LCD fill chunk failed: %s", esp_err_to_name(err));
            return err;
        }
    }

    free(buffer);
    ESP_LOGI(TAG, "LCD fill complete: color=0x%04X", rgb565);
    return ESP_OK;
}

esp_err_t onx_bsp_lcd_fill_bars(void)
{
    static const uint16_t colors[] = {0xF800, 0x07E0, 0x001F, 0xFFFF, 0x0000};
    ESP_RETURN_ON_ERROR(onx_bsp_lcd_init(), TAG, "LCD init failed");

    const int band_h = ONX_LCD_V_RES / (int)(sizeof(colors) / sizeof(colors[0]));
    for (size_t band = 0; band < sizeof(colors) / sizeof(colors[0]); ++band) {
        const int y0 = (int)band * band_h;
        const int y1 = (band == (sizeof(colors) / sizeof(colors[0]) - 1)) ? ONX_LCD_V_RES : y0 + band_h;
        const int rows = y1 - y0;
        const int pixels = ONX_LCD_H_RES * rows;
        uint16_t *buffer = heap_caps_malloc(pixels * sizeof(uint16_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
        ESP_RETURN_ON_FALSE(buffer != NULL, ESP_ERR_NO_MEM, TAG, "LCD bar buffer allocation failed");
        for (int i = 0; i < pixels; ++i) {
            buffer[i] = colors[band];
        }
        esp_err_t err = lcd_draw_window(0, y0, ONX_LCD_H_RES, y1, buffer, pixels * sizeof(uint16_t));
        free(buffer);
        ESP_RETURN_ON_ERROR(err, TAG, "LCD color bar failed");
    }
    ESP_LOGI(TAG, "LCD color bars complete");
    return ESP_OK;
}

esp_err_t onx_bsp_backlight_init(void)
{
    if (s_backlight_ready) {
        return ESP_OK;
    }

    ledc_timer_config_t timer_cfg = {
        .speed_mode = ONX_BL_LEDC_MODE,
        .timer_num = ONX_BL_LEDC_TIMER,
        .duty_resolution = ONX_BL_LEDC_DUTY_RES,
        .freq_hz = ONX_BL_LEDC_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_RETURN_ON_ERROR(ledc_timer_config(&timer_cfg), TAG, "LEDC timer init failed");

    ledc_channel_config_t channel_cfg = {
        .gpio_num = ONX_PIN_LCD_BL,
        .speed_mode = ONX_BL_LEDC_MODE,
        .channel = ONX_BL_LEDC_CHANNEL,
        .timer_sel = ONX_BL_LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_RETURN_ON_ERROR(ledc_channel_config(&channel_cfg), TAG, "LEDC channel init failed");
    s_backlight_ready = true;
    ESP_LOGI(TAG, "Backlight PWM ready: gpio=%d freq=%dHz", ONX_PIN_LCD_BL, ONX_BL_LEDC_FREQ_HZ);
    return ESP_OK;
}

esp_err_t onx_bsp_backlight_set(uint8_t brightness_percent)
{
    ESP_RETURN_ON_ERROR(onx_bsp_backlight_init(), TAG, "Backlight init failed");
    if (brightness_percent > 100) {
        brightness_percent = 100;
    }
    const uint32_t duty = ((uint32_t)brightness_percent * ONX_BL_LEDC_MAX_DUTY) / 100U;
    ESP_RETURN_ON_ERROR(ledc_set_duty(ONX_BL_LEDC_MODE, ONX_BL_LEDC_CHANNEL, duty), TAG, "LEDC set duty failed");
    ESP_RETURN_ON_ERROR(ledc_update_duty(ONX_BL_LEDC_MODE, ONX_BL_LEDC_CHANNEL), TAG, "LEDC update duty failed");
    ESP_LOGI(TAG, "Backlight PWM set: %u%% duty=%" PRIu32, brightness_percent, duty);
    return ESP_OK;
}

static esp_err_t cst826_read(uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(s_cst826_dev, &reg, 1, data, len, 1000);
}

esp_err_t onx_bsp_touch_init(void)
{
    ESP_RETURN_ON_ERROR(onx_bsp_i2c_init(), TAG, "I2C init failed");
    if (s_touch_ready) {
        return ESP_OK;
    }

    if (!s_cst826_dev) {
        ESP_RETURN_ON_ERROR(i2c_add_device(ONX_CST826_ADDR, 400000, &s_cst826_dev), TAG, "CST826 add failed");
    }

    uint8_t id = 0;
    esp_err_t err = cst826_read(CST826_REG_CHIP_ID, &id, 1);
    ESP_RETURN_ON_ERROR(err, TAG, "CST826 chip ID read failed");
    s_touch_ready = true;
    ESP_LOGI(TAG, "Touch init complete: CST826 addr=0x%02X chip_id=0x%02X", ONX_CST826_ADDR, id);
    return ESP_OK;
}

esp_err_t onx_bsp_touch_read(onx_touch_point_t *point)
{
    ESP_RETURN_ON_FALSE(point != NULL, ESP_ERR_INVALID_ARG, TAG, "point is null");
    ESP_RETURN_ON_ERROR(onx_bsp_touch_init(), TAG, "Touch init failed");

    uint8_t data[5] = {};
    ESP_RETURN_ON_ERROR(cst826_read(CST826_REG_DATA_START, data, sizeof(data)), TAG, "CST826 data read failed");

    point->points = data[0] > 2 ? 2 : data[0];
    point->x = (uint16_t)(((data[1] & 0x0F) << 8) | data[2]);
    point->y = (uint16_t)(((data[3] & 0x0F) << 8) | data[4]);
    return ESP_OK;
}

esp_err_t onx_bsp_smoke_run(void)
{
    ESP_LOGI(TAG, "ONX3248G035 BSP smoke start");
    ESP_RETURN_ON_ERROR(onx_bsp_i2c_init(), TAG, "I2C init failed");
    ESP_RETURN_ON_ERROR(onx_bsp_pcf8574_init(), TAG, "PCF8574 init failed");
    ESP_RETURN_ON_ERROR(onx_bsp_backlight_init(), TAG, "Backlight init failed");
    ESP_RETURN_ON_ERROR(onx_bsp_backlight_set(30), TAG, "Backlight 30 failed");
    ESP_RETURN_ON_ERROR(onx_bsp_lcd_init(), TAG, "LCD init failed");

    ESP_RETURN_ON_ERROR(onx_bsp_lcd_fill(0xF800), TAG, "LCD red fill failed");
    vTaskDelay(pdMS_TO_TICKS(500));
    ESP_RETURN_ON_ERROR(onx_bsp_lcd_fill(0x07E0), TAG, "LCD green fill failed");
    vTaskDelay(pdMS_TO_TICKS(500));
    ESP_RETURN_ON_ERROR(onx_bsp_lcd_fill(0x001F), TAG, "LCD blue fill failed");
    vTaskDelay(pdMS_TO_TICKS(500));
    ESP_RETURN_ON_ERROR(onx_bsp_lcd_fill_bars(), TAG, "LCD bars failed");

    ESP_RETURN_ON_ERROR(onx_bsp_backlight_set(100), TAG, "Backlight 100 failed");
    ESP_RETURN_ON_ERROR(onx_bsp_touch_init(), TAG, "Touch init failed");

    ESP_LOGI(TAG, "ONX3248G035 BSP smoke init complete; touch sampling is running");
    ESP_LOGI(TAG, "Tap the four screen corners; coordinates print only when touch is detected");
    uint32_t samples = 0;
    while (true) {
        onx_touch_point_t point = {};
        const esp_err_t err = onx_bsp_touch_read(&point);
        if (err == ESP_OK && point.points > 0) {
            ESP_LOGI(TAG, "Touch: points=%u x=%u y=%u", point.points, point.x, point.y);
        } else if (err != ESP_OK) {
            ESP_LOGW(TAG, "Touch read failed: %s", esp_err_to_name(err));
        }
        if ((++samples % 50) == 0) {
            ESP_LOGI(TAG, "Touch sampler still running; waiting for touch");
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
