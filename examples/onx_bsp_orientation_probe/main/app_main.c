#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_commands.h"
#include "esp_lcd_panel_io.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "onx3248g035_bsp.h"

static const char *TAG = "onx_orientation_probe";

#define RGB565_BLACK 0x0000
#define RGB565_WHITE 0xFFFF
#define RGB565_RED 0xF800
#define RGB565_GREEN 0x07E0
#define RGB565_BLUE 0x001F
#define RGB565_YELLOW 0xFFE0
#define RGB565_CYAN 0x07FF
#define RGB565_DARK_GRAY 0x2104

#define LANDSCAPE_W 480
#define LANDSCAPE_H 320
#define PROBE_PAGE_MS 30000

typedef enum {
    PROBE_LANDSCAPE_MV = 0,
    PROBE_LANDSCAPE_MX_MV = 1,
    PROBE_LANDSCAPE_MY_MV = 2,
    PROBE_LANDSCAPE_MX_MY_MV = 3,
} probe_orientation_t;

typedef struct {
    char letter;
    uint8_t rows[7];
} glyph_t;

typedef struct {
    const char *name;
    int x;
    int y;
} target_t;

typedef struct {
    const char *name;
    probe_orientation_t orientation;
    uint8_t madctl;
} probe_candidate_t;

static const glyph_t s_font[] = {
    {'0', {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}},
    {'1', {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}},
    {'2', {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}},
    {'3', {0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E}},
    {'4', {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}},
    {'5', {0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E}},
    {'6', {0x0E, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x0E}},
    {'7', {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}},
    {'8', {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}},
    {'9', {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x0E}},
    {'A', {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
    {'B', {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E}},
    {'C', {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}},
    {'D', {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E}},
    {'E', {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}},
    {'F', {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10}},
    {'G', {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0E}},
    {'H', {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
    {'I', {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E}},
    {'K', {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}},
    {'L', {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F}},
    {'M', {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11}},
    {'N', {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11}},
    {'O', {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}},
    {'P', {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}},
    {'R', {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11}},
    {'S', {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}},
    {'T', {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}},
    {'U', {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}},
    {'V', {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04}},
    {'W', {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A}},
    {'X', {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11}},
    {'Y', {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04}},
};

static const target_t s_targets[] = {
    {"TOP LEFT", 62, 50},
    {"TOP RIGHT", LANDSCAPE_W - 62, 50},
    {"BOTTOM LEFT", 78, LANDSCAPE_H - 50},
    {"BOTTOM RIGHT", LANDSCAPE_W - 78, LANDSCAPE_H - 50},
    {"CENTER", LANDSCAPE_W / 2, LANDSCAPE_H / 2},
};

static const probe_candidate_t s_candidates[] = {
    {
        .name = "LANDSCAPE MV 28",
        .orientation = PROBE_LANDSCAPE_MV,
        .madctl = LCD_CMD_MV_BIT | LCD_CMD_BGR_BIT,
    },
    {
        .name = "LANDSCAPE MX MV 68",
        .orientation = PROBE_LANDSCAPE_MX_MV,
        .madctl = LCD_CMD_MX_BIT | LCD_CMD_MV_BIT | LCD_CMD_BGR_BIT,
    },
    {
        .name = "LANDSCAPE MY MV A8",
        .orientation = PROBE_LANDSCAPE_MY_MV,
        .madctl = LCD_CMD_MY_BIT | LCD_CMD_MV_BIT | LCD_CMD_BGR_BIT,
    },
    {
        .name = "LANDSCAPE MX MY MV E8",
        .orientation = PROBE_LANDSCAPE_MX_MY_MV,
        .madctl = LCD_CMD_MX_BIT | LCD_CMD_MY_BIT | LCD_CMD_MV_BIT | LCD_CMD_BGR_BIT,
    },
};

static uint16_t pack_rgb565(uint16_t rgb565)
{
    return __builtin_bswap16(rgb565);
}

static esp_err_t tx_param(uint8_t cmd, const void *data, size_t len)
{
    esp_lcd_panel_io_handle_t io = onx_bsp_lcd_get_io_handle();
    ESP_RETURN_ON_FALSE(io != NULL, ESP_ERR_INVALID_STATE, TAG, "LCD IO unavailable");
    return esp_lcd_panel_io_tx_param(io, cmd, data, len);
}

static esp_err_t drain_color_queue(void)
{
    esp_lcd_panel_io_handle_t io = onx_bsp_lcd_get_io_handle();
    ESP_RETURN_ON_FALSE(io != NULL, ESP_ERR_INVALID_STATE, TAG, "LCD IO unavailable");
    return esp_lcd_panel_io_tx_param(io, -1, NULL, 0);
}

static esp_err_t set_madctl(uint8_t madctl)
{
    ESP_RETURN_ON_ERROR(tx_param(LCD_CMD_MADCTL, &madctl, sizeof(madctl)), TAG, "MADCTL set failed");
    ESP_LOGI(TAG, "Orientation probe MADCTL set: 0x%02X", madctl);
    return ESP_OK;
}

static esp_err_t draw_window(int x_start, int y_start, int x_end, int y_end, const void *color_data, size_t bytes)
{
    esp_lcd_panel_io_handle_t io = onx_bsp_lcd_get_io_handle();
    ESP_RETURN_ON_FALSE(io != NULL, ESP_ERR_INVALID_STATE, TAG, "LCD IO unavailable");
    ESP_RETURN_ON_FALSE(x_start < x_end && y_start < y_end, ESP_ERR_INVALID_ARG, TAG, "invalid window");

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

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_CASET, caset, sizeof(caset)), TAG, "CASET failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_RASET, raset, sizeof(raset)), TAG, "RASET failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_color(io, LCD_CMD_RAMWR, color_data, bytes), TAG, "RAMWR failed");
    return drain_color_queue();
}

static esp_err_t fill_rect(int x, int y, int w, int h, uint16_t rgb565)
{
    if (x < 0) {
        w += x;
        x = 0;
    }
    if (y < 0) {
        h += y;
        y = 0;
    }
    if (x + w > LANDSCAPE_W) {
        w = LANDSCAPE_W - x;
    }
    if (y + h > LANDSCAPE_H) {
        h = LANDSCAPE_H - y;
    }
    if (w <= 0 || h <= 0) {
        return ESP_OK;
    }

    const int chunk_rows_max = 16;
    const int max_pixels = w * ((h < chunk_rows_max) ? h : chunk_rows_max);
    uint16_t *buffer = heap_caps_malloc(max_pixels * sizeof(uint16_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    ESP_RETURN_ON_FALSE(buffer != NULL, ESP_ERR_NO_MEM, TAG, "rect buffer alloc failed");
    const uint16_t bus_color = pack_rgb565(rgb565);
    for (int i = 0; i < max_pixels; ++i) {
        buffer[i] = bus_color;
    }

    for (int row = 0; row < h; row += chunk_rows_max) {
        const int chunk_rows = (row + chunk_rows_max <= h) ? chunk_rows_max : (h - row);
        esp_err_t err = draw_window(x, y + row, x + w, y + row + chunk_rows,
                                    buffer, w * chunk_rows * sizeof(uint16_t));
        if (err != ESP_OK) {
            free(buffer);
            return err;
        }
    }
    free(buffer);
    return ESP_OK;
}

static const uint8_t *find_glyph(char letter)
{
    if (letter >= 'a' && letter <= 'z') {
        letter = (char)(letter - 'a' + 'A');
    }
    for (size_t i = 0; i < sizeof(s_font) / sizeof(s_font[0]); ++i) {
        if (s_font[i].letter == letter) {
            return s_font[i].rows;
        }
    }
    return NULL;
}

static int text_width(const char *text, int scale)
{
    int chars = 0;
    for (const char *p = text; *p != '\0'; ++p) {
        ++chars;
    }
    return chars > 0 ? (chars * 6 - 1) * scale : 0;
}

static esp_err_t draw_char(int x, int y, char letter, uint16_t color, int scale)
{
    if (letter == ' ') {
        return ESP_OK;
    }
    const uint8_t *rows = find_glyph(letter);
    if (rows == NULL) {
        return ESP_OK;
    }
    for (int row = 0; row < 7; ++row) {
        for (int col = 0; col < 5; ++col) {
            if (rows[row] & (uint8_t)(1U << (4 - col))) {
                ESP_RETURN_ON_ERROR(fill_rect(x + col * scale, y + row * scale, scale, scale, color),
                                    TAG, "draw glyph pixel failed");
            }
        }
    }
    return ESP_OK;
}

static esp_err_t draw_text(int x, int y, const char *text, uint16_t color, int scale)
{
    for (const char *p = text; *p != '\0'; ++p) {
        ESP_RETURN_ON_ERROR(draw_char(x, y, *p, color, scale), TAG, "draw char failed");
        x += 6 * scale;
    }
    return ESP_OK;
}

static esp_err_t draw_label_centered(int cx, int cy, const char *text, uint16_t color, int scale)
{
    return draw_text(cx - text_width(text, scale) / 2, cy - (7 * scale) / 2, text, color, scale);
}

static esp_err_t draw_target(const target_t *target)
{
    const int half = 24;
    ESP_RETURN_ON_ERROR(fill_rect(target->x - half, target->y - half, half * 2, 4, RGB565_YELLOW), TAG, "target top");
    ESP_RETURN_ON_ERROR(fill_rect(target->x - half, target->y + half - 4, half * 2, 4, RGB565_YELLOW), TAG, "target bottom");
    ESP_RETURN_ON_ERROR(fill_rect(target->x - half, target->y - half, 4, half * 2, RGB565_YELLOW), TAG, "target left");
    ESP_RETURN_ON_ERROR(fill_rect(target->x + half - 4, target->y - half, 4, half * 2, RGB565_YELLOW), TAG, "target right");
    ESP_RETURN_ON_ERROR(fill_rect(target->x - 18, target->y - 2, 36, 4, RGB565_WHITE), TAG, "target cross x");
    ESP_RETURN_ON_ERROR(fill_rect(target->x - 2, target->y - 18, 4, 36, RGB565_WHITE), TAG, "target cross y");

    int label_y = target->y + half + 24;
    if (label_y + 16 >= LANDSCAPE_H) {
        label_y = target->y - half - 24;
    }
    ESP_RETURN_ON_ERROR(draw_label_centered(target->x, label_y - 9, "TAP", RGB565_WHITE, 2), TAG, "target tap label");
    return draw_label_centered(target->x, label_y + 9, target->name, RGB565_WHITE, 2);
}

static esp_err_t draw_probe_page(const probe_candidate_t *candidate)
{
    ESP_RETURN_ON_ERROR(set_madctl(candidate->madctl), TAG, "set orientation failed");
    ESP_RETURN_ON_ERROR(fill_rect(0, 0, LANDSCAPE_W, LANDSCAPE_H, RGB565_DARK_GRAY), TAG, "clear failed");
    ESP_RETURN_ON_ERROR(fill_rect(0, 0, LANDSCAPE_W, 34, RGB565_BLUE), TAG, "top band failed");
    ESP_RETURN_ON_ERROR(fill_rect(0, LANDSCAPE_H - 34, LANDSCAPE_W, 34, RGB565_RED), TAG, "bottom band failed");
    ESP_RETURN_ON_ERROR(fill_rect(0, 0, 34, LANDSCAPE_H, RGB565_GREEN), TAG, "left band failed");
    ESP_RETURN_ON_ERROR(fill_rect(LANDSCAPE_W - 34, 0, 34, LANDSCAPE_H, RGB565_CYAN), TAG, "right band failed");

    ESP_RETURN_ON_ERROR(draw_label_centered(LANDSCAPE_W / 2, 17, "TOP", RGB565_WHITE, 2), TAG, "draw TOP");
    ESP_RETURN_ON_ERROR(draw_label_centered(LANDSCAPE_W / 2, LANDSCAPE_H - 17, "BOTTOM", RGB565_WHITE, 2), TAG, "draw BOTTOM");
    ESP_RETURN_ON_ERROR(draw_label_centered(17, LANDSCAPE_H / 2, "LEFT", RGB565_BLACK, 2), TAG, "draw LEFT");
    ESP_RETURN_ON_ERROR(draw_label_centered(LANDSCAPE_W - 17, LANDSCAPE_H / 2, "RIGHT", RGB565_BLACK, 2), TAG, "draw RIGHT");
    ESP_RETURN_ON_ERROR(draw_label_centered(LANDSCAPE_W / 2, 78, candidate->name, RGB565_WHITE, 3), TAG, "draw title");
    ESP_RETURN_ON_ERROR(draw_label_centered(LANDSCAPE_W / 2, 112, "TAP TARGETS", RGB565_WHITE, 2), TAG, "draw hint");

    for (size_t i = 0; i < sizeof(s_targets) / sizeof(s_targets[0]); ++i) {
        ESP_RETURN_ON_ERROR(draw_target(&s_targets[i]), TAG, "draw target");
    }
    ESP_LOGI(TAG, "Orientation probe page ready: candidate=%s madctl=0x%02X logical=%dx%d",
             candidate->name, candidate->madctl, LANDSCAPE_W, LANDSCAPE_H);
    return ESP_OK;
}

static void map_touch(const probe_candidate_t *candidate,
                      const onx_touch_point_t *raw,
                      int *mapped_x,
                      int *mapped_y)
{
    switch (candidate->orientation) {
    case PROBE_LANDSCAPE_MV:
        *mapped_x = raw->y;
        *mapped_y = raw->x;
        break;
    case PROBE_LANDSCAPE_MX_MV:
        *mapped_x = raw->y;
        *mapped_y = (ONX_LCD_H_RES - 1) - raw->x;
        break;
    case PROBE_LANDSCAPE_MY_MV:
        *mapped_x = (ONX_LCD_V_RES - 1) - raw->y;
        *mapped_y = raw->x;
        break;
    case PROBE_LANDSCAPE_MX_MY_MV:
        *mapped_x = (ONX_LCD_V_RES - 1) - raw->y;
        *mapped_y = (ONX_LCD_H_RES - 1) - raw->x;
        break;
    }
    if (*mapped_x < 0) {
        *mapped_x = 0;
    } else if (*mapped_x >= LANDSCAPE_W) {
        *mapped_x = LANDSCAPE_W - 1;
    }
    if (*mapped_y < 0) {
        *mapped_y = 0;
    } else if (*mapped_y >= LANDSCAPE_H) {
        *mapped_y = LANDSCAPE_H - 1;
    }
}

static const char *transform_for_candidate(const probe_candidate_t *candidate)
{
    switch (candidate->orientation) {
    case PROBE_LANDSCAPE_MV:
        return "x=raw_y y=raw_x";
    case PROBE_LANDSCAPE_MX_MV:
        return "x=raw_y y=319-raw_x";
    case PROBE_LANDSCAPE_MY_MV:
        return "x=479-raw_y y=raw_x";
    case PROBE_LANDSCAPE_MX_MY_MV:
        return "x=479-raw_y y=319-raw_x";
    }
    return "unknown";
}

static const char *target_for_point(int x, int y)
{
    const target_t *best = &s_targets[0];
    int best_dist = INT32_MAX;
    for (size_t i = 0; i < sizeof(s_targets) / sizeof(s_targets[0]); ++i) {
        const int dx = x - s_targets[i].x;
        const int dy = y - s_targets[i].y;
        const int dist = dx * dx + dy * dy;
        if (dist < best_dist) {
            best = &s_targets[i];
            best_dist = dist;
        }
    }
    return best->name;
}

static void run_candidate(const probe_candidate_t *candidate)
{
    ESP_ERROR_CHECK(draw_probe_page(candidate));
    const int loops = PROBE_PAGE_MS / 100;
    onx_touch_point_t last = {};
    bool last_pressed = false;
    for (int i = 0; i < loops; ++i) {
        onx_touch_point_t raw = {};
        esp_err_t err = onx_bsp_touch_read(&raw);
        if (err == ESP_OK && raw.points > 0) {
            int mx = 0;
            int my = 0;
            map_touch(candidate, &raw, &mx, &my);
            if (!last_pressed || raw.x != last.x || raw.y != last.y) {
                ESP_LOGI(TAG,
                         "Touch candidate=%s raw=(%u,%u points=%u) mapped=(%d,%d) target=%s transform=%s",
                         candidate->name, raw.x, raw.y, raw.points, mx, my, target_for_point(mx, my),
                         transform_for_candidate(candidate));
            }
            last = raw;
            last_pressed = true;
        } else {
            last_pressed = false;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "ONX3248G035 orientation probe start");
    ESP_LOGI(TAG, "Scope: direct ST7796/CST826 landscape probe only; no PrintSphere UI layout");
    ESP_ERROR_CHECK(onx_bsp_backlight_init());
    ESP_ERROR_CHECK(onx_bsp_backlight_set(100));
    ESP_ERROR_CHECK(onx_bsp_lcd_init());
    ESP_ERROR_CHECK(onx_bsp_touch_init());

    while (true) {
        for (size_t i = 0; i < sizeof(s_candidates) / sizeof(s_candidates[0]); ++i) {
            run_candidate(&s_candidates[i]);
        }
    }
}
