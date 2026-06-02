#include <inttypes.h>
#include <stdint.h>

#include "bsp/esp32_s3_touch_amoled_1_75.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "esp_memory_utils.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "onx3248g035_bsp.h"

static const char *TAG = "onx_bsp_lvgl";

#define ONX_LVGL_BUFFER_ROWS 40
#define ONX_LVGL_TASK_STACK 8192
#define ONX_LVGL_TASK_PRIORITY 4
#define ONX_LVGL_TASK_START_DELAY_MS 500

static esp_lcd_panel_handle_t s_lvgl_panel;
static lv_display_t *s_lvgl_display;
static lv_indev_t *s_lvgl_indev;
static SemaphoreHandle_t s_lvgl_mutex;
static TaskHandle_t s_lvgl_task;
static uint16_t *s_lvgl_buf;
static onx_touch_point_t s_last_touch;
static uint32_t s_lvgl_flush_count;
static uint32_t s_lvgl_timer_count;

static uint32_t onx_lvgl_tick_get(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

static bool onx_lvgl_take(uint32_t timeout_ms)
{
    if (s_lvgl_mutex == NULL) {
        return false;
    }

    const TickType_t ticks = (timeout_ms == UINT32_MAX) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTakeRecursive(s_lvgl_mutex, ticks) == pdTRUE;
}

static void onx_lvgl_give(void)
{
    if (s_lvgl_mutex) {
        xSemaphoreGiveRecursive(s_lvgl_mutex);
    }
}

static void onx_lvgl_log_pixel_summary(uint32_t flush_id, const uint8_t *px_map, int pixels)
{
    if (flush_id > 10 || px_map == NULL || pixels <= 0) {
        return;
    }

    const uint16_t *rgb565 = (const uint16_t *)px_map;
    const int sample_count = pixels < 256 ? pixels : 256;
    int non_black = 0;
    for (int i = 0; i < sample_count; ++i) {
        if (rgb565[i] != 0x0000) {
            ++non_black;
        }
    }

    const uint16_t p0 = pixels > 0 ? rgb565[0] : 0;
    const uint16_t p1 = pixels > 1 ? rgb565[1] : 0;
    const uint16_t p2 = pixels > 2 ? rgb565[2] : 0;
    const uint16_t p3 = pixels > 3 ? rgb565[3] : 0;
    ESP_LOGI(TAG,
             "LVGL flush #%" PRIu32 " pixel summary: sample=%d non_black=%d first=%04X,%04X,%04X,%04X",
             flush_id, sample_count, non_black, p0, p1, p2, p3);
}

static void onx_lvgl_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    const uint32_t flush_id = ++s_lvgl_flush_count;
    if (s_lvgl_panel == NULL || area == NULL || px_map == NULL) {
        ESP_LOGW(TAG, "LVGL flush #%" PRIu32 " skipped: panel=%p area=%p px=%p",
                 flush_id, s_lvgl_panel, area, px_map);
        lv_display_flush_ready(disp);
        return;
    }

    const int x1 = area->x1 < 0 ? 0 : area->x1;
    const int y1 = area->y1 < 0 ? 0 : area->y1;
    const int x2 = area->x2 >= ONX_LCD_H_RES ? (ONX_LCD_H_RES - 1) : area->x2;
    const int y2 = area->y2 >= ONX_LCD_V_RES ? (ONX_LCD_V_RES - 1) : area->y2;
    if (x1 <= x2 && y1 <= y2) {
        const int pixels = (x2 - x1 + 1) * (y2 - y1 + 1);
        if (flush_id <= 10) {
            ESP_LOGI(TAG,
                     "LVGL flush #%" PRIu32 ": raw=(%d,%d)-(%d,%d) clamped=(%d,%d)-(%d,%d) pixels=%d bytes=%d",
                     flush_id, area->x1, area->y1, area->x2, area->y2,
                     x1, y1, x2, y2, pixels, pixels * 2);
            onx_lvgl_log_pixel_summary(flush_id, px_map, pixels);
        }
        if (x1 != area->x1 || y1 != area->y1 || x2 != area->x2 || y2 != area->y2) {
            ESP_LOGW(TAG, "LVGL flush #%" PRIu32 " area clamped", flush_id);
        }
        esp_err_t err = esp_lcd_panel_draw_bitmap(s_lvgl_panel, x1, y1, x2 + 1, y2 + 1, px_map);
        if (flush_id <= 10 || err != ESP_OK) {
            ESP_LOGI(TAG, "LVGL flush #%" PRIu32 " draw_bitmap=%s", flush_id, esp_err_to_name(err));
        }
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "LVGL flush failed: %s", esp_err_to_name(err));
        }
    } else {
        ESP_LOGW(TAG, "LVGL flush #%" PRIu32 " invalid after clamp: raw=(%d,%d)-(%d,%d) clamped=(%d,%d)-(%d,%d)",
                 flush_id, area->x1, area->y1, area->x2, area->y2, x1, y1, x2, y2);
    }

    lv_display_flush_ready(disp);
}

static void onx_lvgl_touch_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    if (data == NULL) {
        return;
    }

    onx_touch_point_t point = {};
    esp_err_t err = onx_bsp_touch_read(&point);
    if (err == ESP_OK && point.points > 0) {
        if (point.x >= ONX_LCD_H_RES) {
            point.x = ONX_LCD_H_RES - 1;
        }
        if (point.y >= ONX_LCD_V_RES) {
            point.y = ONX_LCD_V_RES - 1;
        }
        s_last_touch = point;
        data->point.x = (int32_t)point.x;
        data->point.y = (int32_t)point.y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->point.x = (int32_t)s_last_touch.x;
        data->point.y = (int32_t)s_last_touch.y;
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static void onx_lvgl_task(void *arg)
{
    (void)arg;
    vTaskDelay(pdMS_TO_TICKS(ONX_LVGL_TASK_START_DELAY_MS));

    while (true) {
        uint32_t delay_ms = 10;
        if (onx_lvgl_take(1000)) {
            delay_ms = lv_timer_handler();
            s_lvgl_timer_count++;
            if (s_lvgl_timer_count <= 10 || (s_lvgl_timer_count % 200) == 0) {
                ESP_LOGI(TAG, "LVGL task heartbeat: timers=%" PRIu32 " flushes=%" PRIu32 " next_delay=%" PRIu32,
                         s_lvgl_timer_count, s_lvgl_flush_count, delay_ms);
            }
            onx_lvgl_give();
        } else if ((s_lvgl_timer_count % 200) == 0) {
            ESP_LOGW(TAG, "LVGL task failed to take mutex: timers=%" PRIu32 " flushes=%" PRIu32,
                     s_lvgl_timer_count, s_lvgl_flush_count);
        }

        if (delay_ms < 5) {
            delay_ms = 5;
        } else if (delay_ms > 50) {
            delay_ms = 50;
        }
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

static esp_err_t onx_lvgl_alloc_buffer(void)
{
    if (s_lvgl_buf != NULL) {
        return ESP_OK;
    }

    const size_t pixels = ONX_LCD_H_RES * ONX_LVGL_BUFFER_ROWS;
    const size_t bytes = pixels * sizeof(uint16_t);
    s_lvgl_buf = heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (s_lvgl_buf == NULL) {
        s_lvgl_buf = heap_caps_malloc(bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    }
    ESP_RETURN_ON_FALSE(s_lvgl_buf != NULL, ESP_ERR_NO_MEM, TAG, "LVGL buffer allocation failed");
    ESP_LOGI(TAG, "LVGL buffer ready: rows=%d bytes=%u region=%s",
             ONX_LVGL_BUFFER_ROWS, (unsigned)bytes,
             esp_ptr_external_ram(s_lvgl_buf) ? "psram" : "internal");
    return ESP_OK;
}

lv_display_t *onx_bsp_lvgl_start(bsp_display_cfg_t *cfg)
{
    ESP_LOGI(TAG, "LVGL start begin: cfg=%p", cfg);
    if (s_lvgl_display) {
        ESP_LOGI(TAG, "LVGL start reuse existing display=%p input=%p", s_lvgl_display, s_lvgl_indev);
        return s_lvgl_display;
    }

    if (cfg) {
        if (cfg->rotation != ESP_LV_ADAPTER_ROTATE_0) {
            ESP_LOGW(TAG, "ONX M3 starts in fixed portrait; cfg rotation=%d ignored", (int)cfg->rotation);
        }
        if (cfg->tear_avoid_mode != ESP_LV_ADAPTER_TEAR_AVOID_MODE_NONE) {
            ESP_LOGW(TAG, "ONX ST7796 path has no TE GPIO; tear_avoid_mode=%d ignored",
                     (int)cfg->tear_avoid_mode);
        }
        if (cfg->touch_flags.swap_xy || cfg->touch_flags.mirror_x || cfg->touch_flags.mirror_y) {
            ESP_LOGW(TAG, "ONX touch uses verified portrait mapping; cfg touch flags swap=%u mx=%u my=%u ignored",
                     cfg->touch_flags.swap_xy, cfg->touch_flags.mirror_x, cfg->touch_flags.mirror_y);
        }
    }

    if (s_lvgl_mutex == NULL) {
        ESP_LOGI(TAG, "LVGL start: create recursive mutex");
        s_lvgl_mutex = xSemaphoreCreateRecursiveMutex();
        if (s_lvgl_mutex == NULL) {
            ESP_LOGE(TAG, "LVGL mutex allocation failed");
            return NULL;
        }
    }

    if (!lv_is_initialized()) {
        ESP_LOGI(TAG, "LVGL start: lv_init");
        lv_init();
    } else {
        ESP_LOGI(TAG, "LVGL start: lv already initialized");
    }
    lv_tick_set_cb(onx_lvgl_tick_get);
    ESP_LOGI(TAG, "LVGL start: tick callback installed");

    esp_lcd_panel_io_handle_t io = NULL;
    esp_err_t err = bsp_display_new(NULL, &s_lvgl_panel, &io);
    ESP_LOGI(TAG, "LVGL start: bsp_display_new=%s panel=%p io=%p",
             esp_err_to_name(err), s_lvgl_panel, io);
    if (err != ESP_OK || s_lvgl_panel == NULL) {
        ESP_LOGE(TAG, "ONX panel creation failed");
        return NULL;
    }
    ESP_LOGI(TAG, "ONX LVGL Recovery A: madctl=0x48 mirror_x=0 mirror_y=0 rgb565_swap=panel_wrapper drain=1");

    err = bsp_display_brightness_init();
    ESP_LOGI(TAG, "LVGL start: brightness init=%s", esp_err_to_name(err));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ONX backlight init failed");
        return NULL;
    }
    err = bsp_display_backlight_on();
    ESP_LOGI(TAG, "LVGL start: backlight on=%s brightness=%d",
             esp_err_to_name(err), bsp_display_brightness_get());
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ONX backlight enable failed");
        return NULL;
    }
    err = onx_bsp_touch_init();
    ESP_LOGI(TAG, "LVGL start: touch init=%s", esp_err_to_name(err));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ONX touch init failed");
        return NULL;
    }
    err = onx_lvgl_alloc_buffer();
    ESP_LOGI(TAG, "LVGL start: buffer alloc=%s buf=%p", esp_err_to_name(err), s_lvgl_buf);
    if (err != ESP_OK) {
        return NULL;
    }

    ESP_LOGI(TAG, "LVGL start: take mutex for display creation");
    if (!onx_lvgl_take(UINT32_MAX)) {
        ESP_LOGE(TAG, "LVGL mutex take failed during start");
        return NULL;
    }

    s_lvgl_display = lv_display_create(ONX_LCD_H_RES, ONX_LCD_V_RES);
    ESP_LOGI(TAG, "LVGL start: lv_display_create=%p", s_lvgl_display);
    if (s_lvgl_display == NULL) {
        onx_lvgl_give();
        ESP_LOGE(TAG, "lv_display_create failed");
        return NULL;
    }
    lv_display_set_color_format(s_lvgl_display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(s_lvgl_display, onx_lvgl_flush);
    lv_display_set_buffers(s_lvgl_display,
                           s_lvgl_buf,
                           NULL,
                           ONX_LCD_H_RES * ONX_LVGL_BUFFER_ROWS * sizeof(uint16_t),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    ESP_LOGI(TAG, "LVGL start: display buffers set bytes=%u mode=partial",
             (unsigned)(ONX_LCD_H_RES * ONX_LVGL_BUFFER_ROWS * sizeof(uint16_t)));
    lv_display_set_default(s_lvgl_display);

    s_lvgl_indev = lv_indev_create();
    ESP_LOGI(TAG, "LVGL start: lv_indev_create=%p", s_lvgl_indev);
    if (s_lvgl_indev) {
        lv_indev_set_type(s_lvgl_indev, LV_INDEV_TYPE_POINTER);
        lv_indev_set_display(s_lvgl_indev, s_lvgl_display);
        lv_indev_set_read_cb(s_lvgl_indev, onx_lvgl_touch_read);
    } else {
        ESP_LOGW(TAG, "lv_indev_create failed; display will start without touch input");
    }
    onx_lvgl_give();

    if (s_lvgl_task == NULL) {
        BaseType_t ok = xTaskCreatePinnedToCore(onx_lvgl_task,
                                                "onx_lvgl",
                                                ONX_LVGL_TASK_STACK,
                                                NULL,
                                                ONX_LVGL_TASK_PRIORITY,
                                                &s_lvgl_task,
                                                (cfg && cfg->lv_adapter_cfg.task_core_id >= 0 &&
                                                 cfg->lv_adapter_cfg.task_core_id <= 1)
                                                    ? cfg->lv_adapter_cfg.task_core_id
                                                    : tskNO_AFFINITY);
        ESP_LOGI(TAG, "LVGL start: task create result=%d handle=%p", (int)ok, s_lvgl_task);
        if (ok != pdPASS) {
            ESP_LOGE(TAG, "LVGL task creation failed");
            return NULL;
        }
    }

    ESP_LOGI(TAG, "ONX LVGL display started: %dx%d RGB565 partial rows=%d",
             ONX_LCD_H_RES, ONX_LCD_V_RES, ONX_LVGL_BUFFER_ROWS);
    return s_lvgl_display;
}

lv_indev_t *onx_bsp_lvgl_get_input_dev(void)
{
    return s_lvgl_indev;
}

esp_err_t onx_bsp_lvgl_lock(uint32_t timeout_ms)
{
    return onx_lvgl_take(timeout_ms) ? ESP_OK : ESP_ERR_TIMEOUT;
}

void onx_bsp_lvgl_unlock(void)
{
    onx_lvgl_give();
}
