#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_io_additions.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_memory_utils.h"
#include "esp_timer.h"
#include "esp_vfs_fat.h"
#include "esp_spiffs.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_lcd_co5300.h"
#include "esp_lcd_touch_cst9217.h"

#include "esp_codec_dev_defaults.h"
#include "bsp/esp32_s3_touch_amoled_1_75.h"
#include "bsp_err_check.h"
#include "bsp/display.h"
#include "bsp/touch.h"

static const char *TAG = "ESP32-S3-Touch-AMOLED-1.75";

static i2c_master_bus_handle_t i2c_handle = NULL; // I2C Handle
static bool i2c_initialized = false;
static esp_io_expander_handle_t io_expander = NULL; // IO expander tca9554 handle
static lv_indev_t *disp_indev = NULL;
sdmmc_card_t *bsp_sdcard = NULL; // Global uSD card handler
static esp_lcd_touch_handle_t tp = NULL;
static esp_lcd_panel_handle_t panel_handle = NULL; // LCD panel handle
static esp_lcd_panel_handle_t lvgl_panel_handle = NULL; // Rotation-aware panel handle for LVGL
static esp_lcd_panel_io_handle_t io_handle = NULL;
static lv_display_t *lvgl_display_handle = NULL;
uint8_t brightness;
static i2s_chan_handle_t i2s_tx_chan = NULL;
static i2s_chan_handle_t i2s_rx_chan = NULL;
static const audio_codec_data_if_t *i2s_data_if = NULL; /* Codec data interface */

#define BSP_ES7210_CODEC_ADDR ES7210_CODEC_DEFAULT_ADDR
#define BSP_I2S_GPIO_CFG       \
    {                          \
        .mclk = BSP_I2S_MCLK,  \
        .bclk = BSP_I2S_SCLK,  \
        .ws = BSP_I2S_LCLK,    \
        .dout = BSP_I2S_DOUT,  \
        .din = BSP_I2S_DSIN,   \
        .invert_flags = {      \
            .mclk_inv = false, \
            .bclk_inv = false, \
            .ws_inv = false,   \
        },                     \
    }

static const co5300_lcd_init_cmd_t lcd_init_cmds[] = {
    {0xFE, (uint8_t[]){0x20}, 1, 0},
    {0x19, (uint8_t[]){0x10}, 1, 0},
    {0x1C, (uint8_t[]){0xA0}, 1, 0},

    {0xFE, (uint8_t[]){0x00}, 1, 0},
    {0xC4, (uint8_t[]){0x80}, 1, 0},
    {0x3A, (uint8_t[]){0x55}, 1, 0},
    {0x35, (uint8_t[]){0x00}, 1, 0},
    {0x53, (uint8_t[]){0x20}, 1, 0},
    {0x51, (uint8_t[]){0xFF}, 1, 0},
    {0x63, (uint8_t[]){0xFF}, 1, 0},
    {0x2A, (uint8_t[]){0x00, 0x06, 0x01, 0xD7}, 4, 0},
    {0x2B, (uint8_t[]){0x00, 0x00, 0x01, 0xD1}, 4, 600},
    {0x11, NULL, 0, 600},
    {0x29, NULL, 0, 0},
};

typedef struct {
    esp_lcd_panel_t base;
    esp_lcd_panel_handle_t target;
    bsp_display_rotation_t rotation;
    uint16_t *rotation_buffer;
    size_t rotation_buffer_pixels;
    SemaphoreHandle_t te_sem;        /* binary semaphore, given on TE rising edge */
    int te_gpio;                     /* configured TE GPIO, -1 if disabled */
    uint32_t te_timeout_ticks;       /* xSemaphoreTake timeout */
    volatile uint32_t te_pulse_count;/* incremented in ISR for diagnostics */
    uint32_t te_log_next_count;      /* threshold to log diagnostic */
} bsp_rotation_panel_t;

static bsp_rotation_panel_t rotation_panel = {0};

static void IRAM_ATTR bsp_rotation_panel_te_isr(void *arg)
{
    bsp_rotation_panel_t *ctx = (bsp_rotation_panel_t *)arg;
    ctx->te_pulse_count++;
    BaseType_t hpw = pdFALSE;
    xSemaphoreGiveFromISR(ctx->te_sem, &hpw);
    if (hpw == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

static esp_err_t bsp_rotation_panel_te_setup(bsp_rotation_panel_t *ctx, int te_gpio,
                                             uint32_t timeout_ms)
{
    if (te_gpio < 0) {
        ctx->te_gpio = -1;
        return ESP_OK;
    }
    if (ctx->te_sem == NULL) {
        ctx->te_sem = xSemaphoreCreateBinary();
        if (ctx->te_sem == NULL) {
            return ESP_ERR_NO_MEM;
        }
    }
    ctx->te_gpio = te_gpio;
    ctx->te_timeout_ticks = pdMS_TO_TICKS(timeout_ms > 0 ? timeout_ms : 50);

    const gpio_config_t io_cfg = {
        .pin_bit_mask = 1ULL << te_gpio,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_POSEDGE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&io_cfg), TAG, "TE gpio_config failed");

    /* gpio_install_isr_service may already be installed by another component;
     * treat ESP_ERR_INVALID_STATE as success.
     *
     * IMPORTANT: Do NOT pass ESP_INTR_FLAG_IRAM here. The shared GPIO ISR
     * dispatcher fans out to every registered handler, and esp_lvgl_port's
     * touch interrupt callback (lvgl_port_touch_interrupt_callback) is NOT
     * located in IRAM. With ESP_INTR_FLAG_IRAM the dispatcher would also
     * run while the SPI flash cache is disabled (NVS commits, OTA writes,
     * mbedTLS allocator quirks, ...) and crash with "Cache disabled but
     * cached memory region accessed" the moment any touch event arrives.
     *
     * Without the flag the whole GPIO ISR path is masked during cache-off
     * windows; we may drop the occasional TE pulse, but bsp_rotation_panel_wait_te
     * already has a 50 ms fallback timeout for exactly that case. */
    esp_err_t err = gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "gpio_install_isr_service failed: %d", err);
        return err;
    }
    ESP_RETURN_ON_ERROR(gpio_isr_handler_add(te_gpio, bsp_rotation_panel_te_isr, ctx),
                        TAG, "gpio_isr_handler_add failed");
    return ESP_OK;
}

/* Wait for the next TE pulse, draining any stale pulse first so the wait
 * synchronizes with the *upcoming* V-blank. Returns immediately if TE is
 * disabled or the semaphore is missing. */
static inline void bsp_rotation_panel_wait_te(bsp_rotation_panel_t *ctx)
{
    if (ctx->te_gpio < 0 || ctx->te_sem == NULL) {
        return;
    }
    /* Diagnostic: log the pulse rate after the first ~1024 pulses so we can
     * verify TE is actually firing. If it stays at 0, GPIO13 isn't TE. */
    const uint32_t pulses = ctx->te_pulse_count;
    if (pulses >= ctx->te_log_next_count) {
        ESP_LOGI(TAG, "TE diagnostic: %" PRIu32 " pulses observed", pulses);
        ctx->te_log_next_count = pulses + 1024;
    }
    /* Drain stale pulse from a previous frame. */
    (void)xSemaphoreTake(ctx->te_sem, 0);
    /* Wait for the fresh pulse; timeout falls through to keep LVGL responsive
     * even if TE stops (e.g. after disp_off). */
    (void)xSemaphoreTake(ctx->te_sem, ctx->te_timeout_ticks);
}

static bsp_rotation_panel_t *bsp_rotation_panel_from_base(esp_lcd_panel_t *panel)
{
    return panel ? (bsp_rotation_panel_t *)panel->user_data : NULL;
}

static esp_err_t bsp_rotation_panel_reset(esp_lcd_panel_t *panel)
{
    bsp_rotation_panel_t *ctx = bsp_rotation_panel_from_base(panel);
    return ctx && ctx->target ? esp_lcd_panel_reset(ctx->target) : ESP_ERR_INVALID_STATE;
}

static esp_err_t bsp_rotation_panel_init(esp_lcd_panel_t *panel)
{
    bsp_rotation_panel_t *ctx = bsp_rotation_panel_from_base(panel);
    return ctx && ctx->target ? esp_lcd_panel_init(ctx->target) : ESP_ERR_INVALID_STATE;
}

static esp_err_t bsp_rotation_panel_del(esp_lcd_panel_t *panel)
{
    bsp_rotation_panel_t *ctx = bsp_rotation_panel_from_base(panel);
    if (!ctx || !ctx->target)
    {
        return ESP_ERR_INVALID_STATE;
    }

    free(ctx->rotation_buffer);
    ctx->rotation_buffer = NULL;
    ctx->rotation_buffer_pixels = 0;
    return esp_lcd_panel_del(ctx->target);
}

static bool bsp_rotation_panel_ensure_buffer(bsp_rotation_panel_t *ctx, size_t pixels)
{
    if (ctx->rotation_buffer_pixels >= pixels && ctx->rotation_buffer != NULL)
    {
        return true;
    }

    free(ctx->rotation_buffer);
    ctx->rotation_buffer = heap_caps_malloc(pixels * sizeof(uint16_t),
                                            MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    ctx->rotation_buffer_pixels = ctx->rotation_buffer ? pixels : 0;
    return ctx->rotation_buffer != NULL;
}

static void bsp_rotation_panel_rotate_region_90(const uint16_t *src, uint16_t *dst,
                                                int src_w, int src_h)
{
    // Write the rotated output row-contiguously. On PSRAM this is much cheaper
    // than the naive source-row loop, which writes one pixel every destination row.
    for (int dst_y = 0; dst_y < src_w; ++dst_y)
    {
        const uint16_t *src_col = src + dst_y;
        uint16_t *dst_row = dst + ((size_t)dst_y * src_h);
        for (int dst_x = 0; dst_x < src_h; ++dst_x)
        {
            dst_row[dst_x] = src_col[(size_t)(src_h - 1 - dst_x) * src_w];
        }
    }
}

static void bsp_rotation_panel_rotate_region_270(const uint16_t *src, uint16_t *dst,
                                                 int src_w, int src_h)
{
    // Same layout strategy as 90 deg: contiguous writes into the rotated buffer.
    for (int dst_y = 0; dst_y < src_w; ++dst_y)
    {
        const int src_x = src_w - 1 - dst_y;
        uint16_t *dst_row = dst + ((size_t)dst_y * src_h);
        for (int dst_x = 0; dst_x < src_h; ++dst_x)
        {
            dst_row[dst_x] = src[((size_t)dst_x * src_w) + src_x];
        }
    }
}

static esp_err_t bsp_rotation_panel_draw_staged(bsp_rotation_panel_t *ctx, int x_start, int y_start,
                                                int x_end, int y_end, const uint16_t *src)
{
    const int width = x_end - x_start;
    const int height = y_end - y_start;
    if (width <= 0 || height <= 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    /* Source is in PSRAM. Direct PSRAM-source QSPI DMA at 80 MHz under WiFi
     * load triggers "DMA TX underflow" because PSRAM cache misses cannot
     * refill the SPI FIFO fast enough. We therefore copy the chunk into a
     * pre-allocated internal-RAM staging buffer first and DMA from there.
     *
     * For full_refresh / direct_mode the input area can be much larger than
     * the staging buffer (typically the full 466x466 frame). We loop over
     * horizontal stripes that fit into the pre-allocated buffer, pushing
     * each stripe in raster order. The underlying esp_lcd_panel_io_spi has
     * queue_depth=1 (CONFIG_BSP_LCD_TRANS_QUEUE_DEPTH=1) so each call blocks
     * until the prior DMA completes -- this is exactly the property the TE
     * sync hook relies on to keep the writer raster-ordered ahead of the
     * panel scanline. */
    if (ctx->rotation_buffer == NULL || ctx->rotation_buffer_pixels == 0)
    {
        ESP_LOGE(TAG, "Staging buffer not pre-allocated");
        return ESP_ERR_INVALID_STATE;
    }

    const size_t lines_per_chunk = ctx->rotation_buffer_pixels / (size_t)width;
    if (lines_per_chunk == 0)
    {
        ESP_LOGE(TAG, "Staging buffer too small for width=%d (have %zu px)",
                 width, ctx->rotation_buffer_pixels);
        return ESP_ERR_INVALID_SIZE;
    }

    int y = y_start;
    while (y < y_end)
    {
        int chunk_y_end = y + (int)lines_per_chunk;
        if (chunk_y_end > y_end)
        {
            chunk_y_end = y_end;
        }
        const int chunk_h = chunk_y_end - y;
        const size_t pixel_count = (size_t)width * (size_t)chunk_h;
        const uint16_t *src_chunk = src + (size_t)(y - y_start) * (size_t)width;

        memcpy(ctx->rotation_buffer, src_chunk, pixel_count * sizeof(uint16_t));
        esp_err_t err = esp_lcd_panel_draw_bitmap(ctx->target, x_start, y, x_end, chunk_y_end,
                                                  ctx->rotation_buffer);
        if (err != ESP_OK)
        {
            return err;
        }
        y = chunk_y_end;
    }
    return ESP_OK;
}

static esp_err_t bsp_rotation_panel_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start,
                                                int x_end, int y_end, const void *color_data)
{
    bsp_rotation_panel_t *ctx = bsp_rotation_panel_from_base(panel);
    if (!ctx || !ctx->target)
    {
        return ESP_ERR_INVALID_STATE;
    }

    /* TE-sync is handled once per LVGL refresh cycle via the
     * LV_EVENT_REFR_START callback registered in bsp_display_lcd_init();
     * combined with full_refresh + raster-ordered staged stripes below this
     * keeps the writer ahead of the panel scanout without per-flush waits. */

    if (ctx->rotation != BSP_DISPLAY_ROTATE_90 && ctx->rotation != BSP_DISPLAY_ROTATE_270)
    {
        if (color_data && esp_ptr_external_ram(color_data))
        {
            return bsp_rotation_panel_draw_staged(ctx, x_start, y_start, x_end, y_end,
                                                 (const uint16_t *)color_data);
        }
        return esp_lcd_panel_draw_bitmap(ctx->target, x_start, y_start, x_end, y_end, color_data);
    }

    const int src_w = x_end - x_start;
    const int src_h = y_end - y_start;
    if (src_w <= 0 || src_h <= 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    const size_t pixels = (size_t)src_w * src_h;
    if (!bsp_rotation_panel_ensure_buffer(ctx, pixels))
    {
        ESP_LOGE(TAG, "Unable to allocate %" PRIu32 " px rotation buffer", (uint32_t)pixels);
        return ESP_ERR_NO_MEM;
    }

    const uint16_t *src = (const uint16_t *)color_data;
    int dst_x_start = 0;
    int dst_y_start = 0;
    int dst_x_end = 0;
    int dst_y_end = 0;
    if (ctx->rotation == BSP_DISPLAY_ROTATE_90)
    {
        bsp_rotation_panel_rotate_region_90(src, ctx->rotation_buffer, src_w, src_h);
        dst_x_start = BSP_LCD_H_RES - y_end;
        dst_x_end = BSP_LCD_H_RES - y_start;
        dst_y_start = x_start;
        dst_y_end = x_end;
    }
    else
    {
        bsp_rotation_panel_rotate_region_270(src, ctx->rotation_buffer, src_w, src_h);
        dst_x_start = y_start;
        dst_x_end = y_end;
        dst_y_start = BSP_LCD_V_RES - x_end;
        dst_y_end = BSP_LCD_V_RES - x_start;
    }

    return esp_lcd_panel_draw_bitmap(ctx->target, dst_x_start, dst_y_start, dst_x_end, dst_y_end,
                                     ctx->rotation_buffer);
}

static esp_err_t bsp_rotation_panel_mirror(esp_lcd_panel_t *panel, bool x_axis, bool y_axis)
{
    bsp_rotation_panel_t *ctx = bsp_rotation_panel_from_base(panel);
    return ctx && ctx->target ? esp_lcd_panel_mirror(ctx->target, x_axis, y_axis)
                              : ESP_ERR_INVALID_STATE;
}

static esp_err_t bsp_rotation_panel_swap_xy(esp_lcd_panel_t *panel, bool swap_axes)
{
    bsp_rotation_panel_t *ctx = bsp_rotation_panel_from_base(panel);
    return ctx && ctx->target ? esp_lcd_panel_swap_xy(ctx->target, swap_axes)
                              : ESP_ERR_INVALID_STATE;
}

static esp_err_t bsp_rotation_panel_set_gap(esp_lcd_panel_t *panel, int x_gap, int y_gap)
{
    bsp_rotation_panel_t *ctx = bsp_rotation_panel_from_base(panel);
    return ctx && ctx->target ? esp_lcd_panel_set_gap(ctx->target, x_gap, y_gap)
                              : ESP_ERR_INVALID_STATE;
}

static esp_err_t bsp_rotation_panel_invert_color(esp_lcd_panel_t *panel, bool invert_color_data)
{
    bsp_rotation_panel_t *ctx = bsp_rotation_panel_from_base(panel);
    return ctx && ctx->target ? esp_lcd_panel_invert_color(ctx->target, invert_color_data)
                              : ESP_ERR_INVALID_STATE;
}

static esp_err_t bsp_rotation_panel_disp_on_off(esp_lcd_panel_t *panel, bool on_off)
{
    bsp_rotation_panel_t *ctx = bsp_rotation_panel_from_base(panel);
    return ctx && ctx->target ? esp_lcd_panel_disp_on_off(ctx->target, on_off)
                              : ESP_ERR_INVALID_STATE;
}

static esp_err_t bsp_rotation_panel_disp_sleep(esp_lcd_panel_t *panel, bool sleep)
{
    bsp_rotation_panel_t *ctx = bsp_rotation_panel_from_base(panel);
    return ctx && ctx->target ? esp_lcd_panel_disp_sleep(ctx->target, sleep)
                              : ESP_ERR_INVALID_STATE;
}

static esp_lcd_panel_handle_t bsp_rotation_panel_wrap(esp_lcd_panel_handle_t target)
{
    rotation_panel.base.reset = bsp_rotation_panel_reset;
    rotation_panel.base.init = bsp_rotation_panel_init;
    rotation_panel.base.del = bsp_rotation_panel_del;
    rotation_panel.base.draw_bitmap = bsp_rotation_panel_draw_bitmap;
    rotation_panel.base.mirror = bsp_rotation_panel_mirror;
    rotation_panel.base.swap_xy = bsp_rotation_panel_swap_xy;
    rotation_panel.base.set_gap = bsp_rotation_panel_set_gap;
    rotation_panel.base.invert_color = bsp_rotation_panel_invert_color;
    rotation_panel.base.disp_on_off = bsp_rotation_panel_disp_on_off;
    rotation_panel.base.disp_sleep = bsp_rotation_panel_disp_sleep;
    rotation_panel.base.user_data = &rotation_panel;
    rotation_panel.target = target;
    rotation_panel.rotation = BSP_DISPLAY_ROTATE_0;
    rotation_panel.te_gpio = -1;
    rotation_panel.te_timeout_ticks = pdMS_TO_TICKS(50);
    return &rotation_panel.base;
}

#define BSP_I2S_DUPLEX_MONO_CFG(_sample_rate)                                                         \
    {                                                                                                 \
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(_sample_rate),                                          \
        .slot_cfg = I2S_STD_PHILIP_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO), \
        .gpio_cfg = BSP_I2S_GPIO_CFG,                                                                 \
    }

/**************************************************************************************************
 *
 * I2C Function
 *
 **************************************************************************************************/
esp_err_t bsp_i2c_init(void)
{
    /* I2C was initialized before */
    if (i2c_initialized)
    {
        return ESP_OK;
    }

    i2c_master_bus_config_t i2c_bus_conf = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .sda_io_num = BSP_I2C_SDA,
        .scl_io_num = BSP_I2C_SCL,
        .i2c_port = BSP_I2C_NUM,
    };
    BSP_ERROR_CHECK_RETURN_ERR(i2c_new_master_bus(&i2c_bus_conf, &i2c_handle));

    i2c_initialized = true;

    return ESP_OK;
}

esp_err_t bsp_i2c_deinit(void)
{
    BSP_ERROR_CHECK_RETURN_ERR(i2c_del_master_bus(i2c_handle));
    i2c_initialized = false;
    return ESP_OK;
}

i2c_master_bus_handle_t bsp_i2c_get_handle(void)
{
    bsp_i2c_init();
    return i2c_handle;
}

esp_err_t bsp_spiffs_mount(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = CONFIG_BSP_SPIFFS_MOUNT_POINT,
        .partition_label = CONFIG_BSP_SPIFFS_PARTITION_LABEL,
        .max_files = CONFIG_BSP_SPIFFS_MAX_FILES,
#ifdef CONFIG_BSP_SPIFFS_FORMAT_ON_MOUNT_FAIL
        .format_if_mount_failed = true,
#else
        .format_if_mount_failed = false,
#endif
    };

    esp_err_t ret_val = esp_vfs_spiffs_register(&conf);

    BSP_ERROR_CHECK_RETURN_ERR(ret_val);

    size_t total = 0, used = 0;
    ret_val = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret_val != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret_val));
    }
    else
    {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    return ret_val;
}

esp_err_t bsp_spiffs_unmount(void)
{
    return esp_vfs_spiffs_unregister(CONFIG_BSP_SPIFFS_PARTITION_LABEL);
}

esp_err_t bsp_sdcard_mount(void)
{
    const esp_vfs_fat_sdmmc_mount_config_t mount_config = {
#ifdef CONFIG_BSP_SD_FORMAT_ON_MOUNT_FAIL
        .format_if_mount_failed = true,
#else
        .format_if_mount_failed = false,
#endif
        .max_files = 5,
        .allocation_unit_size = 16 * 1024};

    const sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    const sdmmc_slot_config_t slot_config = {
        .clk = BSP_SD_CLK,
        .cmd = BSP_SD_CMD,
        .d0 = BSP_SD_D0,
        .d1 = GPIO_NUM_NC,
        .d2 = GPIO_NUM_NC,
        .d3 = GPIO_NUM_NC,
        .d4 = GPIO_NUM_NC,
        .d5 = GPIO_NUM_NC,
        .d6 = GPIO_NUM_NC,
        .d7 = GPIO_NUM_NC,
        .cd = SDMMC_SLOT_NO_CD,
        .wp = SDMMC_SLOT_NO_WP,
        .width = 1,
        .flags = 0,
    };

#if !CONFIG_FATFS_LONG_FILENAMES
    ESP_LOGW(TAG, "Warning: Long filenames on SD card are disabled in menuconfig!");
#endif

    return esp_vfs_fat_sdmmc_mount(BSP_SD_MOUNT_POINT, &host, &slot_config, &mount_config, &bsp_sdcard);
}

esp_err_t bsp_sdcard_unmount(void)
{
    return esp_vfs_fat_sdcard_unmount(BSP_SD_MOUNT_POINT, bsp_sdcard);
}

/**************************************************************************************************
 *
 * I2S Audio Function
 *
 **************************************************************************************************/
esp_err_t bsp_audio_init(const i2s_std_config_t *i2s_config)
{
    esp_err_t ret = ESP_FAIL;
    if (i2s_tx_chan && i2s_rx_chan) {
        /* Audio was initialized before */
        return ESP_OK;
    }

    /* Setup I2S peripheral */
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(CONFIG_BSP_I2S_NUM, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true; // Auto clear the legacy data in the DMA buffer
    BSP_ERROR_CHECK_RETURN_ERR(i2s_new_channel(&chan_cfg, &i2s_tx_chan, &i2s_rx_chan));

    /* Setup I2S channels */
    const i2s_std_config_t std_cfg_default = BSP_I2S_DUPLEX_MONO_CFG(22050);
    const i2s_std_config_t *p_i2s_cfg = &std_cfg_default;
    if (i2s_config != NULL) {
        p_i2s_cfg = i2s_config;
    }

    if (i2s_tx_chan != NULL) {
        ESP_GOTO_ON_ERROR(i2s_channel_init_std_mode(i2s_tx_chan, p_i2s_cfg), err, TAG, "I2S channel initialization failed");
        ESP_GOTO_ON_ERROR(i2s_channel_enable(i2s_tx_chan), err, TAG, "I2S enabling failed");
    }
    if (i2s_rx_chan != NULL) {
        ESP_GOTO_ON_ERROR(i2s_channel_init_std_mode(i2s_rx_chan, p_i2s_cfg), err, TAG, "I2S channel initialization failed");
        ESP_GOTO_ON_ERROR(i2s_channel_enable(i2s_rx_chan), err, TAG, "I2S enabling failed");
    }

    audio_codec_i2s_cfg_t i2s_cfg = {
        .port = CONFIG_BSP_I2S_NUM,
        .rx_handle = i2s_rx_chan,
        .tx_handle = i2s_tx_chan,
    };
    i2s_data_if = audio_codec_new_i2s_data(&i2s_cfg);
    BSP_NULL_CHECK_GOTO(i2s_data_if, err);

    return ESP_OK;

err:
    if (i2s_tx_chan) {
        i2s_del_channel(i2s_tx_chan);
    }
    if (i2s_rx_chan) {
        i2s_del_channel(i2s_rx_chan);
    }

    return ret;
}

esp_codec_dev_handle_t bsp_audio_codec_speaker_init(void)
{
    if (i2s_data_if == NULL) {
        /* Initilize I2C */
        BSP_ERROR_CHECK_RETURN_NULL(bsp_i2c_init());
        /* Configure I2S peripheral and Power Amplifier */
        BSP_ERROR_CHECK_RETURN_NULL(bsp_audio_init(NULL));
    }
    assert(i2s_data_if);

    const audio_codec_gpio_if_t *gpio_if = audio_codec_new_gpio();

    audio_codec_i2c_cfg_t i2c_cfg = {
        .port = BSP_I2C_NUM,
        .addr = ES8311_CODEC_DEFAULT_ADDR,
        .bus_handle = i2c_handle,
    };
    const audio_codec_ctrl_if_t *i2c_ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);
    BSP_NULL_CHECK(i2c_ctrl_if, NULL);

    esp_codec_dev_hw_gain_t gain = {
        .pa_voltage = 5.0,
        .codec_dac_voltage = 3.3,
    };

    es8311_codec_cfg_t es8311_cfg = {
        .ctrl_if = i2c_ctrl_if,
        .gpio_if = gpio_if,
        .codec_mode = ESP_CODEC_DEV_WORK_MODE_DAC,
        .pa_pin = BSP_POWER_AMP_IO,
        .pa_reverted = false,
        .master_mode = false,
        .use_mclk = true,
        .digital_mic = false,
        .invert_mclk = false,
        .invert_sclk = false,
        .hw_gain = gain,
    };
    const audio_codec_if_t *es8311_dev = es8311_codec_new(&es8311_cfg);
    BSP_NULL_CHECK(es8311_dev, NULL);

    esp_codec_dev_cfg_t codec_dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_OUT,
        .codec_if = es8311_dev,
        .data_if = i2s_data_if,
    };
    return esp_codec_dev_new(&codec_dev_cfg);
}

esp_codec_dev_handle_t bsp_audio_codec_microphone_init(void)
{
    if (i2s_data_if == NULL) {
        /* Initilize I2C */
        BSP_ERROR_CHECK_RETURN_NULL(bsp_i2c_init());
        /* Configure I2S peripheral and Power Amplifier */
        BSP_ERROR_CHECK_RETURN_NULL(bsp_audio_init(NULL));
    }
    assert(i2s_data_if);

    audio_codec_i2c_cfg_t i2c_cfg = {
        .port = BSP_I2C_NUM,
        .addr = BSP_ES7210_CODEC_ADDR,
        .bus_handle = i2c_handle,
    };
    const audio_codec_ctrl_if_t *i2c_ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);
    BSP_NULL_CHECK(i2c_ctrl_if, NULL);

    es7210_codec_cfg_t es7210_cfg = {
        .ctrl_if = i2c_ctrl_if,
    };
    const audio_codec_if_t *es7210_dev = es7210_codec_new(&es7210_cfg);
    BSP_NULL_CHECK(es7210_dev, NULL);

    esp_codec_dev_cfg_t codec_es7210_dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_IN,
        .codec_if = es7210_dev,
        .data_if = i2s_data_if,
    };
    return esp_codec_dev_new(&codec_es7210_dev_cfg);
}

#define LCD_CMD_BITS (8)
#define LCD_PARAM_BITS (8)
#define LCD_LEDC_CH (CONFIG_BSP_DISPLAY_BRIGHTNESS_LEDC_CH)
#define LVGL_TICK_PERIOD_MS (CONFIG_BSP_DISPLAY_LVGL_TICK)
#define LVGL_MAX_SLEEP_MS (CONFIG_BSP_DISPLAY_LVGL_MAX_SLEEP)
#define BSP_LCD_QSPI_PCLK_HZ (80 * 1000 * 1000)
/* 48 lines per chunk = ~44 KB per QSPI DMA transfer. Larger single-shot
 * transfers (e.g. full-frame 434 KB) cause "DMA TX underflow" because PSRAM
 * cache misses can't refill the SPI FIFO fast enough at 80 MHz QSPI under
 * WiFi load. With smart-wait (one TE wait per frame in draw_bitmap), the
 * 10 chunks per frame are written in ~10 ms total, which is still inside
 * the 16.7 ms 60 Hz frame interval, so the writer stays ahead of the
 * scanout beam and tearing is eliminated. */
#define BSP_LVGL_DEFAULT_BUFFER_LINES (48)

esp_err_t bsp_display_brightness_init(void)
{
    bsp_display_brightness_set(100);
    return ESP_OK;
}

esp_err_t bsp_display_brightness_set(int brightness_percent)
{
    if (panel_handle == NULL)
    {
        ESP_LOGE(TAG, "Panel handle is not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (brightness_percent < 0 || brightness_percent > 100)
    {
        ESP_LOGE(TAG, "Invalid brightness percentage. Should be between 0 and 100.");
        return ESP_ERR_INVALID_ARG;
    }

    brightness = (uint8_t)(brightness_percent * 255 / 100);

    uint32_t lcd_cmd = 0x51;
    lcd_cmd &= 0xff;
    lcd_cmd <<= 8;
    lcd_cmd |= 0x02 << 24;
    uint8_t param = brightness;
    esp_lcd_panel_io_tx_param(io_handle, lcd_cmd, &param, 1);

    return ESP_OK;
}

int bsp_display_brightness_get(void)
{
    if (panel_handle == NULL)
    {
        ESP_LOGE(TAG, "Panel handle is not initialized");
        return -1;
    }

    return brightness * 100 / 255;
}

esp_err_t bsp_display_backlight_off(void)
{
    ESP_LOGI(TAG, "Backlight off");
    return bsp_display_brightness_set(0);
}

esp_err_t bsp_display_backlight_on(void)
{
    ESP_LOGI(TAG, "Backlight on");
    return bsp_display_brightness_set(100);
}
#if LVGL_VERSION_MAJOR >= 9
static void rounder_event_cb(lv_event_t *e)
{
    lv_area_t *area = (lv_area_t *)lv_event_get_param(e);
    uint16_t x1 = area->x1;
    uint16_t x2 = area->x2;

    uint16_t y1 = area->y1;
    uint16_t y2 = area->y2;

    // round the start of coordinate down to the nearest 2M number
    area->x1 = (x1 >> 1) << 1;
    area->y1 = (y1 >> 1) << 1;
    // round the end of coordinate up to the nearest 2N+1 number
    area->x2 = ((x2 >> 1) << 1) + 1;
    area->y2 = ((y2 >> 1) << 1) + 1;
}

/* Fires once at the start of every LVGL refresh cycle, before any flush.
 * This is the correct place to wait for the panel TE pulse: it pairs the
 * upcoming frame's flushes with a fresh V-blank, regardless of how LVGL
 * orders the dirty rectangles internally. */
static void bsp_lvgl_refr_start_cb(lv_event_t *e)
{
    bsp_rotation_panel_t *ctx = (bsp_rotation_panel_t *)lv_event_get_user_data(e);
    if (ctx) {
        bsp_rotation_panel_wait_te(ctx);
    }
}
#else
static void bsp_lvgl_rounder_cb(lv_disp_drv_t *disp_drv, lv_area_t *area)
{
    uint16_t x1 = area->x1;
    uint16_t x2 = area->x2;

    uint16_t y1 = area->y1;
    uint16_t y2 = area->y2;

    // round the start of coordinate down to the nearest 2M number
    area->x1 = (x1 >> 1) << 1;
    area->y1 = (y1 >> 1) << 1;
    // round the end of coordinate up to the nearest 2N+1 number
    area->x2 = ((x2 >> 1) << 1) + 1;
    area->y2 = ((y2 >> 1) << 1) + 1;
}
#endif
esp_err_t bsp_display_new(const bsp_display_config_t *config, esp_lcd_panel_handle_t *ret_panel, esp_lcd_panel_io_handle_t *ret_io)
{
    esp_err_t ret = ESP_OK;
    assert(config != NULL && config->max_transfer_sz > 0);

    ESP_LOGI(TAG, "Initialize SPI bus");
    const spi_bus_config_t buscfg = CO5300_PANEL_BUS_QSPI_CONFIG(BSP_LCD_PCLK,
                                                                 BSP_LCD_DATA0,
                                                                 BSP_LCD_DATA1,
                                                                 BSP_LCD_DATA2,
                                                                 BSP_LCD_DATA3,
                                                                 config->max_transfer_sz);
    ESP_ERROR_CHECK(spi_bus_initialize(BSP_LCD_SPI_NUM, &buscfg, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_spi_config_t io_config = CO5300_PANEL_IO_QSPI_CONFIG(BSP_LCD_CS, NULL, NULL);
    io_config.pclk_hz = BSP_LCD_QSPI_PCLK_HZ;
    io_config.trans_queue_depth = CONFIG_BSP_LCD_TRANS_QUEUE_DEPTH;
    co5300_vendor_config_t vendor_config = {
        .init_cmds = lcd_init_cmds,
        .init_cmds_size = sizeof(lcd_init_cmds) / sizeof(lcd_init_cmds[0]),
        .flags = {
            .use_qspi_interface = 1,
        },
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)BSP_LCD_SPI_NUM, &io_config, &io_handle));
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = BSP_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = BSP_LCD_BITS_PER_PIXEL,
        .vendor_config = &vendor_config,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_co5300(io_handle, &panel_config, &panel_handle));
    esp_lcd_panel_set_gap(panel_handle, 0x06, 0);
    esp_lcd_panel_reset(panel_handle);
    esp_lcd_panel_init(panel_handle);
    esp_lcd_panel_disp_on_off(panel_handle, true);

    if (ret_panel)
    {
        *ret_panel = panel_handle;
    }
    if (ret_io)
    {
        *ret_io = io_handle;
    }
    return ret;
}

esp_err_t bsp_touch_new(const bsp_display_cfg_t *cfg, esp_lcd_touch_handle_t *ret_touch)
{
    assert(cfg != NULL);
    /* Initilize I2C */
    BSP_ERROR_CHECK_RETURN_ERR(bsp_i2c_init());

    /* Initialize touch */
    const esp_lcd_touch_config_t tp_cfg = {
        .x_max = BSP_LCD_H_RES,
        .y_max = BSP_LCD_V_RES,
        .rst_gpio_num = BSP_LCD_TOUCH_RST, // Shared with LCD reset
        .int_gpio_num = BSP_LCD_TOUCH_INT,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = cfg->touch_flags.swap_xy,
            .mirror_x = cfg->touch_flags.mirror_x,
            .mirror_y = cfg->touch_flags.mirror_y,
        },
    };
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_CST9217_CONFIG();
    tp_io_config.scl_speed_hz = CONFIG_BSP_I2C_CLK_SPEED_HZ;
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c(i2c_handle, &tp_io_config, &tp_io_handle), TAG, "");
    return esp_lcd_touch_new_i2c_cst9217(tp_io_handle, &tp_cfg, ret_touch);
}

/**************************************************************************************************
 *
 * IO Expander Function
 *
 **************************************************************************************************/
esp_io_expander_handle_t bsp_io_expander_init(void)
{
    BSP_ERROR_CHECK_RETURN_ERR(bsp_i2c_init());
    if (!io_expander)
    {
        BSP_ERROR_CHECK_RETURN_NULL(esp_io_expander_new_i2c_tca9554(i2c_handle, BSP_IO_EXPANDER_I2C_ADDRESS, &io_expander));
    }
    return io_expander;
}

static lv_display_t *bsp_display_lcd_init(const bsp_display_cfg_t *cfg)
{
    assert(cfg != NULL);

    const uint32_t buffer_height = (cfg->buffer_height_lines > 0)
                                       ? cfg->buffer_height_lines
                                       : BSP_LVGL_DEFAULT_BUFFER_LINES;
    const size_t max_transfer_sz = BSP_LCD_H_RES * buffer_height * BSP_LCD_BITS_PER_PIXEL / 8;
    const bsp_display_config_t disp_config = {
        .max_transfer_sz = max_transfer_sz,
    };

    BSP_ERROR_CHECK_RETURN_NULL(bsp_display_new(&disp_config, &panel_handle, &io_handle));
    lvgl_panel_handle = bsp_rotation_panel_wrap(panel_handle);

    /* Pre-allocate the rotation_panel internal-RAM staging buffer NOW, before
     * any other subsystem (camera JPEG decode, MQTT TLS) has a chance to
     * fragment internal heap. Used by both the rotation paths and the
     * PSRAM-source non-rotated path (avoids QSPI DMA TX underflow). */
    {
        const size_t pixels = (size_t)BSP_LCD_H_RES * buffer_height;
        if (!bsp_rotation_panel_ensure_buffer(&rotation_panel, pixels)) {
            ESP_LOGE(TAG, "Failed to pre-allocate %zu px rotation staging buffer", pixels);
            return NULL;
        }
        ESP_LOGI(TAG, "Reserved %zu B internal-RAM rotation staging buffer",
                 pixels * sizeof(uint16_t));
    }

    if (cfg->te_sync_enabled && cfg->te_gpio >= 0) {
        esp_err_t te_err = bsp_rotation_panel_te_setup(&rotation_panel, cfg->te_gpio,
                                                       cfg->te_timeout_ms);
        if (te_err != ESP_OK) {
            ESP_LOGW(TAG, "TE sync setup failed (gpio=%d, err=%d), tearing protection disabled",
                     cfg->te_gpio, te_err);
        } else {
            ESP_LOGI(TAG, "TE sync enabled on GPIO%d (timeout=%ums)",
                     cfg->te_gpio, (unsigned)cfg->te_timeout_ms);
        }
    } else {
        ESP_LOGI(TAG, "TE sync disabled");
    }

    ESP_LOGI(TAG,
             "LVGL display via esp_lvgl_port: full_refresh=yes fb=%dx%d staging_lines=%" PRIu32
             " max_transfer=%u qspi=%uMHz queue_depth=%d double_buffer=%s buff_spiram=yes",
             BSP_LCD_H_RES, BSP_LCD_V_RES,
             buffer_height,
             (unsigned int)max_transfer_sz,
             (unsigned int)(BSP_LCD_QSPI_PCLK_HZ / 1000000),
             CONFIG_BSP_LCD_TRANS_QUEUE_DEPTH,
             cfg->double_buffer ? "yes" : "no");

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = lvgl_panel_handle,
        /* full_refresh requires buffer_size == hres*vres so that LVGL renders
         * the entire frame into one PSRAM buffer per refresh. esp_lvgl_port
         * then issues a SINGLE esp_lcd_panel_draw_bitmap covering the whole
         * screen, which our rotation_panel wrapper streams to the LCD in
         * raster-ordered stripes through the internal-RAM staging buffer.
         * Combined with the LV_EVENT_REFR_START -> wait_te hook, the writer
         * is started right at panel V-blank and raster-races the scanline
         * top-to-bottom -- the only configuration that yields actually
         * tearing-free output on a CO5300 QSPI panel without RGB framebuffer
         * support. (Partial mode flushes dirty rects in invalidation order,
         * which causes visible tearing whenever there are multiple disjoint
         * dirty rectangles per refresh.) */
        .buffer_size = (uint32_t)BSP_LCD_H_RES * (uint32_t)BSP_LCD_V_RES,
        .double_buffer = cfg->double_buffer,
        .hres = BSP_LCD_H_RES,
        .vres = BSP_LCD_V_RES,
        .monochrome = false,
        .color_format = LV_COLOR_FORMAT_RGB565,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = false,
            .buff_spiram = true,
            .sw_rotate = false,
            // CO5300 QSPI expects MSB-first RGB565; LVGL produces little-endian
            // RGB565, so swap bytes inside the port before pushing to the panel.
            .swap_bytes = true,
            .full_refresh = true,
            .direct_mode = false,
        },
    };

    lv_display_t *disp = lvgl_port_add_disp(&disp_cfg);
    if (!disp) {
        ESP_LOGE(TAG, "lvgl_port_add_disp failed");
        return NULL;
    }
    lvgl_display_handle = disp;

#if LVGL_VERSION_MAJOR >= 9
    lv_display_add_event_cb(disp, rounder_event_cb, LV_EVENT_INVALIDATE_AREA, NULL);
    /* Wait for TE exactly once at the start of every refresh cycle. */
    if (cfg->te_sync_enabled && rotation_panel.te_gpio >= 0) {
        lv_display_add_event_cb(disp, bsp_lvgl_refr_start_cb,
                                LV_EVENT_REFR_START, &rotation_panel);
    }
#else
    lv_disp_t *disp_v8 = (lv_disp_t *)disp;
    if (disp_v8 && disp_v8->driver) {
        disp_v8->driver->rounder_cb = bsp_lvgl_rounder_cb;
    }
#endif

    return disp;
}

// Non-fatal LVGL touchpad read callback. The upstream esp_lvgl_port driver
// invokes ESP_ERROR_CHECK() on the underlying esp_lcd_touch_read_data() call,
// which abort()s the firmware on transient I2C errors (notably 0x108
// ESP_ERR_INVALID_RESPONSE during sleep/wake or noisy bus conditions). Here
// we treat any read failure as "no touch" and rate-limit the warning log so
// it does not flood the console.
static void bsp_touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    esp_lcd_touch_handle_t handle = (esp_lcd_touch_handle_t)lv_indev_get_driver_data(indev);
    data->state = LV_INDEV_STATE_RELEASED;
    if (handle == NULL) {
        return;
    }

    const esp_err_t read_err = esp_lcd_touch_read_data(handle);
    if (read_err != ESP_OK) {
        static int64_t last_warn_us = 0;
        const int64_t now_us = esp_timer_get_time();
        if (now_us - last_warn_us > 5LL * 1000 * 1000) {
            ESP_LOGW(TAG, "touch read failed: %s — ignoring", esp_err_to_name(read_err));
            last_warn_us = now_us;
        }
        return;
    }

    uint16_t x = 0;
    uint16_t y = 0;
    uint16_t strength = 0;
    uint8_t count = 0;
    const bool pressed = esp_lcd_touch_get_coordinates(handle, &x, &y, &strength, &count, 1);
    if (pressed && count > 0) {
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PRESSED;
    }
}

static lv_indev_t *bsp_display_indev_init(const bsp_display_cfg_t *cfg, lv_display_t *disp)
{
    assert(cfg != NULL);
    BSP_ERROR_CHECK_RETURN_NULL(bsp_touch_new(cfg, &tp));
    assert(tp);

    // The upstream lvgl_port_add_touch() wraps esp_lcd_touch_read_data() in
    // ESP_ERROR_CHECK(), which abort()s the firmware on transient I2C glitches
    // (e.g. ESP_ERR_INVALID_RESPONSE 0x108 at idle/wake transitions). We
    // create the LVGL input device manually with a non-fatal read callback so
    // a single bad I2C frame is logged and dropped instead of crashing.
    if (lvgl_port_lock(0) != true) {
        ESP_LOGE(TAG, "lvgl_port_lock failed for touch indev create");
        return NULL;
    }
    lv_indev_t *indev = lv_indev_create();
    if (indev != NULL) {
        lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
        lv_indev_set_read_cb(indev, bsp_touch_read_cb);
        lv_indev_set_display(indev, disp);
        lv_indev_set_driver_data(indev, tp);
    }
    lvgl_port_unlock();
    return indev;
}
/**********************************************************************************************************
 *
 * Display Function
 *
 **********************************************************************************************************/
lv_display_t *bsp_display_start(void)
{
    bsp_display_cfg_t cfg = BSP_DISPLAY_CFG_DEFAULT();
    return bsp_display_start_with_config(&cfg);
}


lv_display_t *bsp_display_start_with_config(bsp_display_cfg_t *cfg)
{
    lv_display_t *disp;

    assert(cfg != NULL);

    const lvgl_port_cfg_t port_cfg = {
        .task_priority = (cfg->task_priority > 0) ? cfg->task_priority : 4,
        .task_stack = (cfg->task_stack > 0) ? cfg->task_stack : 6144,
        .task_affinity = cfg->task_affinity,
        .task_max_sleep_ms = (cfg->task_max_sleep_ms > 0) ? cfg->task_max_sleep_ms : 500,
        .timer_period_ms = (cfg->timer_period_ms > 0) ? cfg->timer_period_ms : 5,
        // Place the LVGL task stack in PSRAM by default: after Wi-Fi init the
        // largest contiguous internal block is typically <10 KB, which is too
        // small for a stack that must accommodate libpng / nested LVGL widgets.
        .task_stack_caps = (cfg->task_stack_caps != 0) ? cfg->task_stack_caps
                                                       : (MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT),
    };
    BSP_ERROR_CHECK_RETURN_NULL(lvgl_port_init(&port_cfg));

    BSP_NULL_CHECK(disp = bsp_display_lcd_init(cfg), NULL);

    BSP_NULL_CHECK(disp_indev = bsp_display_indev_init(cfg, disp), NULL);

    BSP_ERROR_CHECK_RETURN_NULL(bsp_display_brightness_init());

    return disp;
}

lv_indev_t *bsp_display_get_input_dev(void)
{
    return disp_indev;
}

esp_err_t bsp_display_rotation_set(bsp_display_rotation_t rotation)
{
    if (panel_handle == NULL || io_handle == NULL)
    {
        ESP_LOGE(TAG, "Panel or IO handle is not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t madctl = 0x00;
    int x_gap = 0;
    int y_gap = 0;

    switch (rotation)
    {
    case BSP_DISPLAY_ROTATE_0:
        madctl = 0x00;
        // Panel active area starts at native column 6; no row offset.
        x_gap = 6; y_gap = 0;
        break;
    case BSP_DISPLAY_ROTATE_90:
        // 90 deg is handled by the BSP panel wrapper. Keep the real panel in
        // its native active window to avoid exposing guard columns.
        madctl = 0x00;
        x_gap = 6; y_gap = 0;
        break;
    case BSP_DISPLAY_ROTATE_180:
        madctl = 0xC0;
        // Hardware 180 deg is clean on this panel with the measured column gap.
        x_gap = 8; y_gap = 0;
        break;
    case BSP_DISPLAY_ROTATE_270:
        // 270 deg is handled by the BSP panel wrapper.
        madctl = 0x00;
        x_gap = 6; y_gap = 0;
        break;
    default:
        ESP_LOGE(TAG, "Invalid rotation value: %d", rotation);
        return ESP_ERR_INVALID_ARG;
    }

    rotation_panel.rotation = rotation;

    // Re-apply the draw offset expected by the current panel orientation.
    esp_lcd_panel_set_gap(panel_handle, x_gap, y_gap);

    uint32_t lcd_cmd = 0x36;
    lcd_cmd &= 0xff;
    lcd_cmd <<= 8;
    lcd_cmd |= 0x02 << 24;

    ESP_LOGI(TAG, "Set display rotation: %d (MADCTL=0x%02X gap=%d,%d)",
             rotation, madctl, x_gap, y_gap);

    return esp_lcd_panel_io_tx_param(io_handle, lcd_cmd, &madctl, 1);
}

esp_err_t bsp_display_lock(uint32_t timeout_ms)
{
    return lvgl_port_lock(timeout_ms) ? ESP_OK : ESP_ERR_TIMEOUT;
}

void bsp_display_unlock(void)
{
    lvgl_port_unlock();
}

esp_err_t bsp_display_pause(uint32_t timeout_ms)
{
    (void)timeout_ms;
    return lvgl_port_stop();
}

esp_err_t bsp_display_resume(void)
{
    return lvgl_port_resume();
}
