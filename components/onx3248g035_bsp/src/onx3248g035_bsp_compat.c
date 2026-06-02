#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/cdefs.h>

#include "bsp/display.h"
#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_commands.h"
#include "esp_lcd_panel_interface.h"
#include "onx3248g035_bsp.h"

typedef struct _lv_display_t lv_display_t;
typedef struct _lv_indev_t lv_indev_t;
typedef struct bsp_display_cfg_t bsp_display_cfg_t;
typedef struct sdmmc_card_t sdmmc_card_t;
typedef struct esp_lcd_touch *esp_lcd_touch_handle_t;

static const char *TAG = "onx_bsp_compat";

sdmmc_card_t *bsp_sdcard = NULL;

lv_display_t *bsp_display_start_with_config(bsp_display_cfg_t *cfg);
esp_err_t bsp_display_brightness_set(int brightness_percent);

#ifndef ONX_BSP_HAS_LVGL
#define ONX_BSP_HAS_LVGL 0
#endif

#define ONX_PANEL_RECOVERY_A_MADCTL (LCD_CMD_MX_BIT | LCD_CMD_BGR_BIT)

#if ONX_BSP_HAS_LVGL
lv_display_t *onx_bsp_lvgl_start(bsp_display_cfg_t *cfg);
lv_indev_t *onx_bsp_lvgl_get_input_dev(void);
esp_err_t onx_bsp_lvgl_lock(uint32_t timeout_ms);
void onx_bsp_lvgl_unlock(void);
esp_err_t onx_bsp_lvgl_pause(int32_t timeout_ms);
esp_err_t onx_bsp_lvgl_resume(void);
#endif

typedef struct {
    esp_lcd_panel_t base;
    esp_lcd_panel_io_handle_t io;
    uint8_t madctl;
} onx_lcd_panel_t;

static esp_lcd_panel_handle_t s_panel_handle;
static uint32_t s_panel_draw_count;

static uint16_t panel_pack_rgb565(uint16_t rgb565)
{
    /*
     * The ONX SPI panel expects RGB565 payload bytes in bus order. Keep this
     * conversion in the panel wrapper so both LVGL and esp_lcd_panel callers
     * use the same transport path.
     */
    return __builtin_bswap16(rgb565);
}

static esp_err_t panel_tx_window(esp_lcd_panel_io_handle_t io,
                                 int x_start,
                                 int y_start,
                                 int x_end,
                                 int y_end,
                                 const void *color_data,
                                 size_t bytes)
{
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

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_CASET, caset, sizeof(caset)),
                        TAG, "panel CASET failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_RASET, raset, sizeof(raset)),
                        TAG, "panel RASET failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_color(io, LCD_CMD_RAMWR, color_data, bytes),
                        TAG, "panel RAMWR failed");
    return ESP_OK;
}

static esp_err_t panel_drain_color_queue(esp_lcd_panel_io_handle_t io)
{
    /*
     * ESP-IDF's SPI panel IO queues tx_color() as a DMA background transfer.
     * A tx_param() call with lcd_cmd=-1 is documented to wait until queued
     * color transfers finish before returning, without sending an LCD command.
     */
    return esp_lcd_panel_io_tx_param(io, -1, NULL, 0);
}

static esp_err_t onx_panel_del(esp_lcd_panel_t *panel)
{
    if (panel == NULL) {
        return ESP_OK;
    }
    if (s_panel_handle == panel) {
        s_panel_handle = NULL;
    }
    free(__containerof(panel, onx_lcd_panel_t, base));
    return ESP_OK;
}

static esp_err_t onx_panel_reset(esp_lcd_panel_t *panel)
{
    (void)panel;
    return onx_bsp_lcd_init();
}

static esp_err_t onx_panel_init(esp_lcd_panel_t *panel)
{
    (void)panel;
    return onx_bsp_lcd_init();
}

static esp_err_t onx_panel_draw_bitmap(esp_lcd_panel_t *panel,
                                       int x_start,
                                       int y_start,
                                       int x_end,
                                       int y_end,
                                       const void *color_data)
{
    ESP_RETURN_ON_FALSE(panel != NULL && color_data != NULL, ESP_ERR_INVALID_ARG, TAG, "invalid panel draw args");
    ESP_RETURN_ON_FALSE(x_start >= 0 && y_start >= 0 && x_end <= ONX_LCD_H_RES && y_end <= ONX_LCD_V_RES,
                        ESP_ERR_INVALID_ARG, TAG, "panel draw window out of range");
    ESP_RETURN_ON_FALSE(x_start < x_end && y_start < y_end, ESP_ERR_INVALID_ARG, TAG, "empty panel draw window");

    onx_lcd_panel_t *onx_panel = __containerof(panel, onx_lcd_panel_t, base);
    const uint32_t draw_id = ++s_panel_draw_count;
    const int width = x_end - x_start;
    const int height = y_end - y_start;
    const int chunk_rows_max = 16;
    const int chunk_pixels = width * ((height < chunk_rows_max) ? height : chunk_rows_max);
    uint16_t *staging = heap_caps_malloc(chunk_pixels * sizeof(uint16_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    ESP_RETURN_ON_FALSE(staging != NULL, ESP_ERR_NO_MEM, TAG, "panel staging allocation failed");
    if (draw_id <= 10) {
        ESP_LOGI(TAG,
                 "panel draw #%" PRIu32 ": window=(%d,%d)-(%d,%d) size=%dx%d chunk_rows=%d chunk_pixels=%d staging=%p madctl=0x%02X",
                 draw_id, x_start, y_start, x_end, y_end, width, height,
                 chunk_rows_max, chunk_pixels, staging, onx_panel->madctl);
    }

    const uint16_t *src = (const uint16_t *)color_data;
    for (int row = 0; row < height; row += chunk_rows_max) {
        const int chunk_rows = (row + chunk_rows_max <= height) ? chunk_rows_max : (height - row);
        const int pixels = width * chunk_rows;
        for (int i = 0; i < pixels; ++i) {
            staging[i] = panel_pack_rgb565(src[row * width + i]);
        }

        esp_err_t err = panel_tx_window(onx_panel->io,
                                        x_start,
                                        y_start + row,
                                        x_end,
                                        y_start + row + chunk_rows,
                                        staging,
                                        pixels * sizeof(uint16_t));
        if (draw_id <= 3) {
            ESP_LOGI(TAG, "panel draw #%" PRIu32 " chunk row=%d rows=%d pixels=%d tx=%s",
                     draw_id, row, chunk_rows, pixels, esp_err_to_name(err));
        }
        if (err != ESP_OK) {
            free(staging);
            return err;
        }
        err = panel_drain_color_queue(onx_panel->io);
        if (draw_id <= 3) {
            ESP_LOGI(TAG, "panel draw #%" PRIu32 " chunk row=%d drain=%s",
                     draw_id, row, esp_err_to_name(err));
        }
        if (err != ESP_OK) {
            free(staging);
            return err;
        }
    }

    free(staging);
    return ESP_OK;
}

static esp_err_t onx_panel_invert_color(esp_lcd_panel_t *panel, bool invert_color_data)
{
    ESP_RETURN_ON_FALSE(panel != NULL, ESP_ERR_INVALID_ARG, TAG, "panel is null");
    if (invert_color_data) {
        ESP_LOGW(TAG, "ONX verified config requires INVOFF; INVON is not supported in M3");
        return ESP_ERR_NOT_SUPPORTED;
    }
    onx_lcd_panel_t *onx_panel = __containerof(panel, onx_lcd_panel_t, base);
    return esp_lcd_panel_io_tx_param(onx_panel->io, LCD_CMD_INVOFF, NULL, 0);
}

static esp_err_t onx_panel_mirror(esp_lcd_panel_t *panel, bool x_axis, bool y_axis)
{
    ESP_RETURN_ON_FALSE(panel != NULL, ESP_ERR_INVALID_ARG, TAG, "panel is null");
    if (y_axis) {
        ESP_LOGW(TAG, "ONX panel mirror_y is not enabled without a dedicated visual validation run");
        return ESP_ERR_NOT_SUPPORTED;
    }

    onx_lcd_panel_t *onx_panel = __containerof(panel, onx_lcd_panel_t, base);
    uint8_t madctl = onx_panel->madctl;
    madctl |= LCD_CMD_BGR_BIT;
    madctl = x_axis ? (madctl | LCD_CMD_MX_BIT) : madctl;

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(onx_panel->io, LCD_CMD_MADCTL, &madctl, sizeof(madctl)),
                        TAG, "panel MADCTL mirror update failed");
    onx_panel->madctl = madctl;
    ESP_LOGI(TAG, "ONX panel MADCTL updated by mirror: value=0x%02X mirror_x=%d mirror_y=0",
             madctl, x_axis ? 1 : 0);
    return ESP_OK;
}

static esp_err_t onx_panel_swap_xy(esp_lcd_panel_t *panel, bool swap_axes)
{
    (void)panel;
    if (!swap_axes) {
        return ESP_OK;
    }
    ESP_LOGW(TAG, "ONX M3 panel keeps portrait MV=0");
    return ESP_ERR_NOT_SUPPORTED;
}

static esp_err_t onx_panel_set_gap(esp_lcd_panel_t *panel, int x_gap, int y_gap)
{
    (void)panel;
    if (x_gap == 0 && y_gap == 0) {
        return ESP_OK;
    }
    ESP_LOGW(TAG, "ONX M3 panel uses standard CASET/RASET with no gap");
    return ESP_ERR_NOT_SUPPORTED;
}

static esp_err_t onx_panel_disp_on_off(esp_lcd_panel_t *panel, bool on_off)
{
    ESP_RETURN_ON_FALSE(panel != NULL, ESP_ERR_INVALID_ARG, TAG, "panel is null");
    onx_lcd_panel_t *onx_panel = __containerof(panel, onx_lcd_panel_t, base);
    return esp_lcd_panel_io_tx_param(onx_panel->io, on_off ? LCD_CMD_DISPON : LCD_CMD_DISPOFF, NULL, 0);
}

static esp_err_t onx_panel_sleep(esp_lcd_panel_t *panel, bool sleep)
{
    ESP_RETURN_ON_FALSE(panel != NULL, ESP_ERR_INVALID_ARG, TAG, "panel is null");
    onx_lcd_panel_t *onx_panel = __containerof(panel, onx_lcd_panel_t, base);
    return esp_lcd_panel_io_tx_param(onx_panel->io, sleep ? LCD_CMD_SLPIN : LCD_CMD_SLPOUT, NULL, 0);
}

static esp_err_t onx_panel_set_brightness(esp_lcd_panel_t *panel, int brightness)
{
    (void)panel;
    return bsp_display_brightness_set(brightness);
}

static esp_err_t onx_panel_new(esp_lcd_panel_handle_t *ret_panel, esp_lcd_panel_io_handle_t *ret_io)
{
    ESP_RETURN_ON_FALSE(ret_panel != NULL, ESP_ERR_INVALID_ARG, TAG, "ret_panel is null");
    ESP_RETURN_ON_ERROR(onx_bsp_lcd_init(), TAG, "ONX LCD init failed");

    esp_lcd_panel_io_handle_t io = onx_bsp_lcd_get_io_handle();
    ESP_RETURN_ON_FALSE(io != NULL, ESP_ERR_INVALID_STATE, TAG, "LCD IO handle is null");

    if (s_panel_handle == NULL) {
        onx_lcd_panel_t *panel = calloc(1, sizeof(onx_lcd_panel_t));
        ESP_RETURN_ON_FALSE(panel != NULL, ESP_ERR_NO_MEM, TAG, "panel allocation failed");
        panel->io = io;
        panel->madctl = ONX_PANEL_RECOVERY_A_MADCTL;
        panel->base.del = onx_panel_del;
        panel->base.reset = onx_panel_reset;
        panel->base.init = onx_panel_init;
        panel->base.draw_bitmap = onx_panel_draw_bitmap;
        panel->base.invert_color = onx_panel_invert_color;
        panel->base.mirror = onx_panel_mirror;
        panel->base.swap_xy = onx_panel_swap_xy;
        panel->base.set_gap = onx_panel_set_gap;
        panel->base.disp_on_off = onx_panel_disp_on_off;
        panel->base.disp_sleep = onx_panel_sleep;
        panel->base.set_brightness = onx_panel_set_brightness;
        s_panel_handle = &panel->base;
        ESP_LOGI(TAG, "ONX esp_lcd_panel_t wrapper ready");
    }

    *ret_panel = s_panel_handle;
    if (ret_io) {
        *ret_io = io;
    }
    return ESP_OK;
}

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

    return onx_panel_new(ret_panel, ret_io);
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
#if ONX_BSP_HAS_LVGL
    return onx_bsp_lvgl_start(cfg);
#else
    (void)cfg;
    ESP_LOGW(TAG, "ONX LVGL display adapter path is not implemented yet");
    ESP_LOGW(TAG, "Returning NULL so PrintSphere startup treats display as unavailable");
    ESP_ERROR_CHECK_WITHOUT_ABORT(onx_bsp_lcd_init());
    ESP_ERROR_CHECK_WITHOUT_ABORT(onx_bsp_backlight_init());
    return NULL;
#endif
}

lv_indev_t *bsp_display_get_input_dev(void)
{
#if ONX_BSP_HAS_LVGL
    return onx_bsp_lvgl_get_input_dev();
#else
    return NULL;
#endif
}

esp_err_t bsp_display_lock(uint32_t timeout_ms)
{
#if ONX_BSP_HAS_LVGL
    return onx_bsp_lvgl_lock(timeout_ms);
#else
    (void)timeout_ms;
    ESP_LOGW(TAG, "LVGL lock unavailable before ONX adapter path is implemented");
    return ESP_ERR_INVALID_STATE;
#endif
}

void bsp_display_unlock(void)
{
#if ONX_BSP_HAS_LVGL
    onx_bsp_lvgl_unlock();
#endif
}

esp_err_t bsp_display_pause(int32_t timeout_ms)
{
#if ONX_BSP_HAS_LVGL
    return onx_bsp_lvgl_pause(timeout_ms);
#else
    (void)timeout_ms;
    ESP_LOGW(TAG, "LVGL pause unavailable before ONX adapter path is implemented");
    return ESP_ERR_INVALID_STATE;
#endif
}

esp_err_t bsp_display_resume(void)
{
#if ONX_BSP_HAS_LVGL
    return onx_bsp_lvgl_resume();
#else
    ESP_LOGW(TAG, "LVGL resume unavailable before ONX adapter path is implemented");
    return ESP_ERR_INVALID_STATE;
#endif
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
