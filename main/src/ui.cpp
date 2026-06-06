#include "printsphere/ui.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <ctime>
#include <string>

#include "misc/cache/instance/lv_image_cache.h"
#include "bsp/esp32_s3_touch_amoled_1_75.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_memory_utils.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "png.h"
#include "printsphere/board_config.hpp"

extern "C" {
extern const lv_image_dsc_t bambuicon_small;
extern const lv_font_t dosis_20;
extern const lv_font_t dosis_32;
extern const lv_font_t dosis_40;
extern const lv_font_t lv_font_montserrat_20;
extern const lv_font_t mdi_30;
extern const lv_font_t mdi_40;
extern const lv_font_t onx_cjk_16;
}

namespace printsphere {

namespace {

constexpr char kTag[] = "printsphere.ui";
constexpr size_t kImagePersistentReserveBytes = 20U * 1024U;
constexpr int kDefaultBrightnessPercent = 80;
constexpr int kRingStrokeWidth = 22;
constexpr int kRemainingRowY = 172;
constexpr int kPage2PreviewSize = 320;
constexpr int kPage2PreviewYOffset = -12;
constexpr int kPage2NoteWithImageY = 138;
constexpr int kPage2SubnoteWithImageY = 188;
constexpr int kPage3CameraWidth = 400;
constexpr int kPage3CameraHeight = 224;
constexpr int kPage3CameraYOffset = 0;
constexpr int kPage3NoteWithImageY = 150;
constexpr int kPage3SubnoteWithImageY = 182;
// Status text shown above the camera JPEG when an image is loaded.
// Image is 224 high and centered at y=0, so its top is ~y=-112; the
// status sits above with comfortable breathing room.
constexpr int kPage3StatusAboveImageY = -138;
constexpr int kAuxTempRowY = 28;
constexpr int kSwipeThresholdPx = 24;
constexpr int kGestureAxisLockMarginPx = 16;
constexpr int kBrightnessHorizontalTolerancePx = 18;
constexpr int kRotatedVisualOffsetX = 0;
constexpr int kRotatedVisualOffsetY = 0;
constexpr int kManualMinBrightnessPercent = 4;
constexpr uint8_t kRingPulseDepthPercent = 55U;
constexpr uint32_t kDotPulseDurationMs = 1500U;
constexpr int32_t kDotPulseOpaMin = 80;
constexpr int32_t kDotPulseOpaMax = 255;
constexpr int32_t kParallaxTitleMaxY = -18;
constexpr int32_t kParallaxCardsMaxY = -8;
constexpr uint32_t kBatteryDimTimeoutIdleMs = 20000U;
constexpr uint32_t kBatteryOffTimeoutIdleMs = 60000U;
constexpr uint32_t kBatteryDimTimeoutActiveMs = 30000U;
constexpr uint32_t kBatteryOffTimeoutActiveMs = 120000U;
constexpr uint64_t kPortalHintIntroMs = 5ULL * 60ULL * 1000ULL;
constexpr uint32_t kCardRevealDurationMs = 300U;
constexpr int32_t kCardRevealYStart = 28;
constexpr uint32_t kCardRevealStaggerMs = 55U;
constexpr uint32_t kRingBaseDark = 0x101010;
constexpr uint32_t kRingIdleSolid = 0x404040;
constexpr char kDegreeC[] = "\xC2\xB0""C";

#if defined(PRINTSPHERE_BOARD_ONX3248G035) && PRINTSPHERE_BOARD_ONX3248G035
constexpr bool kOnxUiLayout = true;
#else
constexpr bool kOnxUiLayout = false;
#endif
#if defined(PRINTSPHERE_ONX_ORIENTATION_LANDSCAPE) && PRINTSPHERE_ONX_ORIENTATION_LANDSCAPE
constexpr bool kOnxLandscapeLayout = true;
#else
constexpr bool kOnxLandscapeLayout = false;
#endif

constexpr int kOnxPagePad = 12;
constexpr int kOnxContentWidth = kOnxLandscapeLayout ? 456 : 296;
constexpr int kOnxTopbarY = kOnxLandscapeLayout ? 10 : 12;
constexpr int kOnxTopbarHeight = kOnxLandscapeLayout ? 28 : 30;
constexpr int kOnxBottomHintY = kOnxLandscapeLayout ? 292 : 432;
constexpr int kOnxBottomHintHeight = kOnxLandscapeLayout ? 20 : 36;
constexpr int kOnxPreviewImageSize = kOnxLandscapeLayout ? 240 : 296;
constexpr int kOnxCameraImageWidth = kOnxLandscapeLayout ? 300 : 296;
constexpr int kOnxCameraImageHeight = kOnxLandscapeLayout ? 224 : 166;
constexpr int kOnxMainLeftPanelX = 12;
constexpr int kOnxMainLeftPanelY = kOnxLandscapeLayout ? 48 : 140;
constexpr int kOnxMainLeftPanelW = kOnxLandscapeLayout ? 196 : 296;
constexpr int kOnxMainLeftPanelH = kOnxLandscapeLayout ? 190 : 104;
constexpr int kOnxPrinterListX = 12;
constexpr int kOnxPrinterListY = kOnxLandscapeLayout ? 48 : 60;
constexpr int kOnxPrinterListW = kOnxLandscapeLayout ? 300 : 296;
constexpr int kOnxPrinterListH = kOnxLandscapeLayout ? 224 : 348;
constexpr int kOnxPrinterCardW = kOnxLandscapeLayout ? 300 : 296;
constexpr int kOnxPrinterCardH = kOnxLandscapeLayout ? 92 : 76;
constexpr uint32_t kOnxColorBg = 0x050607;
constexpr uint32_t kOnxColorPanel = 0x111418;
constexpr uint32_t kOnxColorPanel2 = 0x171B20;
constexpr uint32_t kOnxColorText = 0xF7FAFC;
constexpr uint32_t kOnxColorSoft = 0xC8D1DC;
constexpr uint32_t kOnxColorMuted = 0x8B98A8;
constexpr uint32_t kOnxColorLine = 0x2D333B;
constexpr uint32_t kOnxColorCyan = 0x87CEEB;
constexpr char kMdiClock[] = "\xF3\xB1\x91\x8E";
constexpr char kMdiNozzle[] = "\xF3\xB0\xB9\x9B";
constexpr char kMdiBed[] = "\xF3\xB1\xA1\x9B";
constexpr char kMdiSync[] = "\xF3\xB1\x9B\x87";
constexpr char kMdiBatteryCharging0[] = "\xF3\xB0\xA0\x92";
constexpr char kMdiBatteryCharging10[] = "\xF3\xB0\xA0\x88";
constexpr char kMdiBatteryCharging20[] = "\xF3\xB0\xA0\x89";
constexpr char kMdiBatteryCharging30[] = "\xF3\xB0\xA0\x8A";
constexpr char kMdiBatteryCharging40[] = "\xF3\xB0\xA0\x8B";
constexpr char kMdiBatteryCharging50[] = "\xF3\xB0\xA0\x8C";
constexpr char kMdiBatteryCharging60[] = "\xF3\xB0\xA0\x8D";
constexpr char kMdiBatteryCharging70[] = "\xF3\xB0\xA0\x8E";
constexpr char kMdiBatteryCharging80[] = "\xF3\xB0\xA0\x8F";
constexpr char kMdiBatteryCharging90[] = "\xF3\xB0\xA0\x90";
constexpr char kMdiBatteryCharging100[] = "\xF3\xB0\xA0\x87";
constexpr char kMdiBatteryAlert[] = "\xF3\xB1\x83\x8D";
constexpr char kMdiBattery10[] = "\xF3\xB0\x81\xBA";
constexpr char kMdiBattery20[] = "\xF3\xB0\x81\xBB";
constexpr char kMdiBattery30[] = "\xF3\xB0\x81\xBC";
constexpr char kMdiBattery40[] = "\xF3\xB0\x81\xBD";
constexpr char kMdiBattery50[] = "\xF3\xB0\x81\xBE";
constexpr char kMdiBattery60[] = "\xF3\xB0\x81\xBF";
constexpr char kMdiBattery70[] = "\xF3\xB0\x82\x80";
constexpr char kMdiBattery80[] = "\xF3\xB0\x82\x81";
constexpr char kMdiBattery90[] = "\xF3\xB0\x82\x82";
constexpr char kMdiBattery100[] = "\xF3\xB0\x81\xB9";

class LvglLockGuard {
 public:
  explicit LvglLockGuard(uint32_t timeout_ms, const char* caller = "?")
      : caller_(caller) {
    const uint32_t before = esp_log_timestamp();
    locked_ = bsp_display_lock(timeout_ms) == ESP_OK;
    const uint32_t wait_ms = esp_log_timestamp() - before;
    if (!locked_) {
      ESP_LOGW(kTag, "LVGL lock FAILED after %lums (timeout=%lu, caller=%s) "
               "heap_int=%lu heap_dma=%lu",
               (unsigned long)wait_ms, (unsigned long)timeout_ms, caller_,
               (unsigned long)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
               (unsigned long)heap_caps_get_free_size(MALLOC_CAP_DMA));
      lock_fail_count_++;
      if (lock_fail_count_ >= 5 && (lock_fail_count_ % 10) == 0) {
        ESP_LOGE(kTag, "LVGL lock failed %lu times consecutively — worker task may be stuck",
                 (unsigned long)lock_fail_count_);
      }
    } else {
      if (lock_fail_count_ > 0) {
        ESP_LOGI(kTag, "LVGL lock recovered after %lu failures (wait=%lums, caller=%s)",
                 (unsigned long)lock_fail_count_, (unsigned long)wait_ms, caller_);
      }
      lock_fail_count_ = 0;
      if (wait_ms > 150) {
        ESP_LOGW(kTag, "LVGL lock waited %lums (caller=%s)",
                 (unsigned long)wait_ms, caller_);
      }
    }
    acquired_ts_ = esp_log_timestamp();
  }

  ~LvglLockGuard() {
    if (locked_) {
      const uint32_t held_ms = esp_log_timestamp() - acquired_ts_;
      bsp_display_unlock();
      if (held_ms > 80) {
        ESP_LOGW(kTag, "LVGL lock held %lums (caller=%s)",
                 (unsigned long)held_ms, caller_);
      }
    }
  }

  bool locked() const { return locked_; }

  static uint32_t lock_fail_count_;

 private:
  const char* caller_ = "?";
  uint32_t acquired_ts_ = 0;
  bool locked_ = false;
};

uint32_t LvglLockGuard::lock_fail_count_ = 0;

bool raw_touch_irq_asserted() {
#if defined(BSP_LCD_TOUCH_INT)
  constexpr gpio_num_t kRawTouchIrqGpio = BSP_LCD_TOUCH_INT;
#else
  constexpr gpio_num_t kRawTouchIrqGpio = GPIO_NUM_NC;
#endif
  if (kRawTouchIrqGpio == GPIO_NUM_NC) {
    return false;
  }
  return gpio_get_level(kRawTouchIrqGpio) == 0;
}

const char* ram_region(const void* ptr) {
  if (ptr == nullptr) {
    return "null";
  }
#if defined(CONFIG_SPIRAM) && CONFIG_SPIRAM
  return esp_ptr_external_ram(ptr) ? "psram" : "internal";
#else
  return "internal";
#endif
}

size_t allocated_size(const void* ptr) {
  return ptr == nullptr ? 0U : heap_caps_get_allocated_size(const_cast<void*>(ptr));
}

void log_heap_diag(const char* context) {
  ESP_LOGI(kTag,
           "[RAM] %s: int_free=%u int_largest=%u dma_free=%u dma_largest=%u "
           "psram_free=%u psram_largest=%u",
           context != nullptr ? context : "-",
           static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL)),
           static_cast<unsigned>(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL)),
           static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_DMA)),
           static_cast<unsigned>(heap_caps_get_largest_free_block(MALLOC_CAP_DMA)),
           static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM)),
           static_cast<unsigned>(heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM)));
}

void log_blob_diag(const char* context, const std::shared_ptr<std::vector<uint8_t>>& blob) {
  const void* data = blob && !blob->empty() ? blob->data() : nullptr;
  ESP_LOGI(kTag,
           "[RAM] %s: size=%u capacity=%u alloc=%u ram=%s data=%p",
           context != nullptr ? context : "-",
           static_cast<unsigned>(blob ? blob->size() : 0U),
           static_cast<unsigned>(blob ? blob->capacity() : 0U),
           static_cast<unsigned>(allocated_size(data)),
           ram_region(data), data);
  log_heap_diag(context);
}

bsp_display_rotation_t bsp_rotation_for(DisplayRotation rotation) {
  switch (rotation) {
    case DisplayRotation::k90:
      return BSP_DISPLAY_ROTATE_90;
    case DisplayRotation::k180:
      return BSP_DISPLAY_ROTATE_180;
    case DisplayRotation::k270:
      return BSP_DISPLAY_ROTATE_270;
    case DisplayRotation::k0:
    default:
      return BSP_DISPLAY_ROTATE_0;
  }
}

void apply_touch_rotation_flags(DisplayRotation rotation, bsp_display_cfg_t* cfg) {
  if (cfg == nullptr) {
    return;
  }

  switch (rotation) {
    case DisplayRotation::k90:
      cfg->touch_flags.swap_xy = 1;
      cfg->touch_flags.mirror_x = 0;
      cfg->touch_flags.mirror_y = 1;
      break;
    case DisplayRotation::k180:
      cfg->touch_flags.swap_xy = 0;
      cfg->touch_flags.mirror_x = 0;
      cfg->touch_flags.mirror_y = 0;
      break;
    case DisplayRotation::k270:
      cfg->touch_flags.swap_xy = 1;
      cfg->touch_flags.mirror_x = 1;
      cfg->touch_flags.mirror_y = 0;
      break;
    case DisplayRotation::k0:
    default:
      cfg->touch_flags.swap_xy = 0;
      cfg->touch_flags.mirror_x = 1;
      cfg->touch_flags.mirror_y = 1;
      break;
  }
}

void make_transparent(lv_obj_t* obj) {
  lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_opa(obj, LV_OPA_TRANSP, 0);
  lv_obj_set_style_pad_all(obj, 0, 0);
}

void apply_page_root_style(lv_obj_t* page) {
  if (kOnxUiLayout) {
    lv_obj_set_style_bg_color(page, lv_color_hex(kOnxColorBg), 0);
    lv_obj_set_style_bg_opa(page, LV_OPA_COVER, 0);
    lv_obj_set_style_border_opa(page, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(page, 0, 0);
  } else {
    make_transparent(page);
  }
}

void set_hidden(lv_obj_t* obj, bool hidden) {
  if (obj == nullptr) {
    return;
  }

  const bool currently_hidden = lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN);
  if (currently_hidden == hidden) {
    return;
  }

  if (hidden) {
    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
  }
}

void set_clickable(lv_obj_t* obj, bool clickable) {
  if (obj == nullptr) {
    return;
  }

  const bool currently_clickable = lv_obj_has_flag(obj, LV_OBJ_FLAG_CLICKABLE);
  if (currently_clickable == clickable) {
    return;
  }

  if (clickable) {
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
  } else {
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
  }
}

void enable_touch_bubble(lv_obj_t* obj) {
  if (obj == nullptr) {
    return;
  }
  lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);
  lv_obj_add_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE);
}

// Draw callback for the green/red triangle slot indicator.
// The triangle is an upward-pointing isosceles triangle centered in the object.
// The bg_color style controls the triangle color; the bg_opa style controls
// the triangle opacity (used for the pulse animation when an HMS error is
// flagged on the slot).
void ams_arrow_draw_cb(lv_event_t* e) {
  auto* obj = static_cast<lv_obj_t*>(lv_event_get_target(e));
  lv_layer_t* layer = lv_event_get_layer(e);
  if (obj == nullptr || layer == nullptr) return;

  lv_area_t coords;
  lv_obj_get_coords(obj, &coords);
  const int32_t w = coords.x2 - coords.x1;
  const int32_t cx = coords.x1 + w / 2;

  const lv_color_t color = lv_obj_get_style_bg_color(obj, LV_PART_MAIN);

  lv_draw_triangle_dsc_t dsc;
  lv_draw_triangle_dsc_init(&dsc);
  dsc.p[0] = {cx, coords.y1};               // top center (tip)
  dsc.p[1] = {coords.x1, coords.y2};        // bottom left
  dsc.p[2] = {coords.x1 + w, coords.y2};   // bottom right
  dsc.color = color;
  dsc.opa = LV_OPA_COVER;
  lv_draw_triangle(layer, &dsc);
}

// Draw callback that overlays a rhombus (diamond stripe) pattern on the
// AMS pill rect when an HMS/Error code is mapped to that slot. Activated by
// setting the user-data flag pointer to a `bool*` that evaluates to true.
void ams_pill_error_overlay_cb(lv_event_t* e) {
  auto* obj = static_cast<lv_obj_t*>(lv_event_get_target(e));
  lv_layer_t* layer = lv_event_get_layer(e);
  if (obj == nullptr || layer == nullptr) return;
  const bool* flag = static_cast<const bool*>(lv_event_get_user_data(e));
  if (flag == nullptr || !*flag) return;

  lv_area_t coords;
  lv_obj_get_coords(obj, &coords);
  const int32_t x1 = coords.x1;
  const int32_t y1 = coords.y1;
  const int32_t x2 = coords.x2;
  const int32_t y2 = coords.y2;
  const int32_t w = x2 - x1;
  const int32_t h = y2 - y1;

  // Draw a diagonal stripe pattern (two crossing line sets = rhombus grid).
  lv_draw_line_dsc_t line;
  lv_draw_line_dsc_init(&line);
  line.color = lv_color_hex(0xFFFFFF);
  line.opa = LV_OPA_50;
  line.width = 2;
  const int32_t spacing = 12;
  // Forward diagonals
  for (int32_t k = -h; k < w; k += spacing) {
    int32_t a_x = x1 + k;
    int32_t a_y = y1;
    int32_t b_x = x1 + k + h;
    int32_t b_y = y2;
    if (a_x < x1) { a_y += (x1 - a_x); a_x = x1; }
    if (b_x > x2) { b_y -= (b_x - x2); b_x = x2; }
    line.p1 = {a_x, a_y};
    line.p2 = {b_x, b_y};
    lv_draw_line(layer, &line);
  }
  // Backward diagonals
  for (int32_t k = 0; k < w + h; k += spacing) {
    int32_t a_x = x1 + k;
    int32_t a_y = y1;
    int32_t b_x = x1 + k - h;
    int32_t b_y = y2;
    if (a_x > x2) { a_y += (a_x - x2); a_x = x2; }
    if (b_x < x1) { b_y -= (x1 - b_x); b_x = x1; }
    line.p1 = {a_x, a_y};
    line.p2 = {b_x, b_y};
    lv_draw_line(layer, &line);
  }
}

int display_rotation_visual_offset_x(DisplayRotation rotation) {
  switch (rotation) {
    case DisplayRotation::k90:
    case DisplayRotation::k270:
      return kRotatedVisualOffsetX;
    case DisplayRotation::k0:
    case DisplayRotation::k180:
    default:
      return 0;
  }
}

int display_rotation_visual_offset_y(DisplayRotation rotation) {
  switch (rotation) {
    case DisplayRotation::k90:
    case DisplayRotation::k270:
      return kRotatedVisualOffsetY;
    case DisplayRotation::k0:
    case DisplayRotation::k180:
    default:
      return 0;
  }
}

void apply_display_rotation_visual_offset(lv_obj_t* obj, DisplayRotation rotation) {
  if (obj == nullptr) {
    return;
  }

  lv_obj_set_style_translate_x(obj, display_rotation_visual_offset_x(rotation), 0);
  lv_obj_set_style_translate_y(obj, display_rotation_visual_offset_y(rotation), 0);
}

void set_label_text_if_changed(lv_obj_t* label, const char* text) {
  if (label == nullptr || text == nullptr) {
    return;
  }

  const char* current = lv_label_get_text(label);
  if (current != nullptr && std::strcmp(current, text) == 0) {
    return;
  }

  lv_label_set_text(label, text);
}

void set_label_text_if_changed(lv_obj_t* label, const std::string& text) {
  set_label_text_if_changed(label, text.c_str());
}

void add_onx_page_chrome(lv_obj_t* page,
                         const char* title,
                         const char* meta,
                         const char* hint,
                         const lv_font_t* title_font,
                         const lv_font_t* hint_font) {
  if (!kOnxUiLayout || page == nullptr) {
    return;
  }

  lv_obj_t* topbar = lv_obj_create(page);
  lv_obj_set_pos(topbar, kOnxPagePad, kOnxTopbarY);
  lv_obj_set_size(topbar, kOnxContentWidth, kOnxTopbarHeight);
  make_transparent(topbar);
  lv_obj_clear_flag(topbar, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(topbar, LV_OBJ_FLAG_CLICKABLE);
  enable_touch_bubble(topbar);

  lv_obj_t* title_label = lv_label_create(topbar);
  set_label_text_if_changed(title_label, title);
  const bool ams_page = title != nullptr && std::strncmp(title, "AMS ", 4) == 0;
  lv_obj_set_width(title_label, kOnxLandscapeLayout ? (ams_page ? 120 : 260) : 220);
  lv_label_set_long_mode(title_label, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_font(title_label, title_font, 0);
  lv_obj_set_style_text_color(title_label, lv_color_hex(kOnxColorText), 0);
  lv_obj_set_style_text_align(title_label, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_align(title_label, LV_ALIGN_LEFT_MID, 0, 0);

  lv_obj_t* meta_label = lv_label_create(topbar);
  set_label_text_if_changed(meta_label, meta);
  lv_obj_set_width(meta_label, kOnxLandscapeLayout ? (ams_page ? 60 : 176) : 68);
  lv_label_set_long_mode(meta_label, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_font(meta_label, hint_font, 0);
  lv_obj_set_style_text_color(meta_label, lv_color_hex(kOnxColorMuted), 0);
  lv_obj_set_style_text_align(meta_label, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_align(meta_label, LV_ALIGN_RIGHT_MID, 0, 0);

  if (hint == nullptr || hint[0] == '\0') {
    return;
  }

  const bool cover_page = std::strcmp(title, "Cover") == 0;
  const int hint_y = kOnxLandscapeLayout ? kOnxBottomHintY : (cover_page ? 448 : kOnxBottomHintY);
  const int hint_h = kOnxLandscapeLayout ? kOnxBottomHintHeight : (cover_page ? 20 : kOnxBottomHintHeight);
  lv_obj_t* hint_line = lv_obj_create(page);
  lv_obj_set_pos(hint_line, kOnxPagePad, hint_y);
  lv_obj_set_size(hint_line, kOnxContentWidth, hint_h);
  make_transparent(hint_line);
  lv_obj_set_style_border_color(hint_line, lv_color_hex(kOnxColorLine), 0);
  lv_obj_set_style_border_opa(hint_line, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(hint_line, 1, 0);
  lv_obj_set_style_border_side(hint_line, LV_BORDER_SIDE_TOP, 0);
  lv_obj_set_style_pad_top(hint_line, (cover_page || kOnxLandscapeLayout) ? 0 : 8, 0);
  lv_obj_clear_flag(hint_line, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(hint_line, LV_OBJ_FLAG_CLICKABLE);
  enable_touch_bubble(hint_line);

  lv_obj_t* hint_label = lv_label_create(hint_line);
  set_label_text_if_changed(hint_label, hint);
  lv_obj_set_width(hint_label, kOnxContentWidth);
  lv_label_set_long_mode(hint_label, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_font(hint_label, hint_font, 0);
  lv_obj_set_style_text_color(hint_label, lv_color_hex(kOnxColorMuted), 0);
  lv_obj_set_style_text_align(hint_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(hint_label, LV_ALIGN_TOP_MID, 0, 0);
}

lv_obj_t* create_onx_panel(lv_obj_t* parent, int x, int y, int w, int h, uint32_t bg_hex) {
  lv_obj_t* panel = lv_obj_create(parent);
  lv_obj_set_pos(panel, x, y);
  lv_obj_set_size(panel, w, h);
  lv_obj_set_style_radius(panel, 8, 0);
  lv_obj_set_style_bg_color(panel, lv_color_hex(bg_hex), 0);
  lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(panel, lv_color_hex(kOnxColorLine), 0);
  lv_obj_set_style_border_opa(panel, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(panel, 1, 0);
  lv_obj_set_style_pad_all(panel, 0, 0);
  lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(panel, LV_OBJ_FLAG_CLICKABLE);
  enable_touch_bubble(panel);
  return panel;
}

lv_obj_t* create_onx_caption(lv_obj_t* parent, const char* text, int x, int y, int w,
                             const lv_font_t* font) {
  lv_obj_t* label = lv_label_create(parent);
  set_label_text_if_changed(label, text);
  lv_obj_set_pos(label, x, y);
  lv_obj_set_width(label, w);
  lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_font(label, font, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(kOnxColorMuted), 0);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);
  return label;
}

std::string optional_temperature_text(const char* label, float temperature_c, bool known = false) {
  if (label == nullptr || (!known && temperature_c <= 0.0f)) {
    return {};
  }

  char buffer[40] = {};
  std::snprintf(buffer, sizeof(buffer), "%s %.0f%s", label, temperature_c, kDegreeC);
  return buffer;
}

bool decode_preview_png(const std::shared_ptr<std::vector<uint8_t>>& encoded_blob,
                        std::shared_ptr<std::vector<uint8_t>>* decoded_blob,
                        lv_image_dsc_t* image_dsc) {
  if (!encoded_blob || encoded_blob->empty() || decoded_blob == nullptr || image_dsc == nullptr) {
    return false;
  }

  png_image image;
  std::memset(&image, 0, sizeof(image));
  image.version = PNG_IMAGE_VERSION;

  if (!png_image_begin_read_from_memory(&image, encoded_blob->data(), encoded_blob->size())) {
    ESP_LOGW(kTag, "Preview PNG header decode failed");
    return false;
  }

  image.format = PNG_FORMAT_BGRA;
  const size_t row_stride = static_cast<size_t>(image.width) * 4U;
  const size_t decoded_size = PNG_IMAGE_SIZE(image);
  auto raw = std::make_shared<std::vector<uint8_t>>();
  raw->reserve(std::max(decoded_size, kImagePersistentReserveBytes));
  raw->resize(decoded_size);

  const bool ok = png_image_finish_read(&image, nullptr, raw->data(),
                                        static_cast<png_int_32>(row_stride), nullptr) != 0;
  if (!ok) {
    ESP_LOGW(kTag, "Preview PNG decode failed: %s", image.message);
    png_image_free(&image);
    return false;
  }

  png_image_free(&image);

  std::memset(image_dsc, 0, sizeof(*image_dsc));
  image_dsc->header.magic = LV_IMAGE_HEADER_MAGIC;
  image_dsc->header.cf = LV_COLOR_FORMAT_ARGB8888;
  image_dsc->header.flags = 0;
  image_dsc->header.w = static_cast<uint16_t>(image.width);
  image_dsc->header.h = static_cast<uint16_t>(image.height);
  image_dsc->header.stride = static_cast<uint16_t>(row_stride);
  image_dsc->data_size = static_cast<uint32_t>(raw->size());
  image_dsc->data = raw->data();
  log_blob_diag("ui preview encoded png", encoded_blob);
  log_blob_diag("ui preview decoded raw", raw);
  *decoded_blob = std::move(raw);
  return true;
}

bool configure_camera_rgb565(const std::shared_ptr<std::vector<uint8_t>>& decoded_blob,
                             uint16_t width, uint16_t height, lv_image_dsc_t* image_dsc) {
  if (!decoded_blob || decoded_blob->empty() || image_dsc == nullptr || width == 0U || height == 0U) {
    return false;
  }

  std::memset(image_dsc, 0, sizeof(*image_dsc));
  image_dsc->header.magic = LV_IMAGE_HEADER_MAGIC;
  image_dsc->header.cf = LV_COLOR_FORMAT_RGB565;
  image_dsc->header.flags = 0;
  image_dsc->header.w = width;
  image_dsc->header.h = height;
  image_dsc->header.stride = static_cast<uint16_t>(width * sizeof(uint16_t));
  image_dsc->data_size = static_cast<uint32_t>(decoded_blob->size());
  image_dsc->data = decoded_blob->data();
  return true;
}

uint32_t scale_color(uint32_t color, uint16_t scale_0_to_255) {
  const uint8_t r = static_cast<uint8_t>((color >> 16) & 0xFF);
  const uint8_t g = static_cast<uint8_t>((color >> 8) & 0xFF);
  const uint8_t b = static_cast<uint8_t>(color & 0xFF);
  const uint8_t sr =
      static_cast<uint8_t>((static_cast<uint32_t>(r) * scale_0_to_255 + 127U) / 255U);
  const uint8_t sg =
      static_cast<uint8_t>((static_cast<uint32_t>(g) * scale_0_to_255 + 127U) / 255U);
  const uint8_t sb =
      static_cast<uint8_t>((static_cast<uint32_t>(b) * scale_0_to_255 + 127U) / 255U);
  return (static_cast<uint32_t>(sr) << 16) | (static_cast<uint32_t>(sg) << 8) | sb;
}

bool color_is_dark(uint32_t rgb) {
  return ((rgb >> 16) & 0xFF) * 299 +
         ((rgb >> 8) & 0xFF) * 587 +
         (rgb & 0xFF) * 114 < 128000;
}

uint32_t hash_mix(uint32_t hash, uint32_t value) {
  hash ^= value + 0x9E3779B9U + (hash << 6) + (hash >> 2);
  return hash;
}

uint32_t hash_string(uint32_t hash, const std::string& value) {
  for (char ch : value) {
    hash = hash_mix(hash, static_cast<uint8_t>(ch));
  }
  return hash_mix(hash, static_cast<uint32_t>(value.size()));
}

uint32_t ams_ui_signature(const PrinterSnapshot& snapshot) {
  uint32_t hash = 2166136261U;
  hash = hash_mix(hash, snapshot.tray_now < 0 ? 0xFFFFFFFFU : static_cast<uint32_t>(snapshot.tray_now));
  hash = hash_mix(hash, snapshot.tray_tar < 0 ? 0xFFFFFFFFU : static_cast<uint32_t>(snapshot.tray_tar));
  if (!snapshot.ams) {
    return hash_mix(hash, 0);
  }

  hash = hash_mix(hash, snapshot.ams->count);
  if (snapshot.ams->count == 0) {
    return hash;
  }

  for (uint8_t u = 0; u < snapshot.ams->count && u < kMaxAmsUnits; ++u) {
    const AmsUnitInfo& unit = snapshot.ams->units[u];
    hash = hash_mix(hash, unit.humidity_pct < 0 ? 0xFFFFFFFFU : static_cast<uint32_t>(unit.humidity_pct));
    hash = hash_mix(hash, static_cast<uint32_t>(std::lround(unit.temperature_c * 10.0f)));
    for (int i = 0; i < kMaxAmsTrays; ++i) {
      const AmsTrayInfo& tray = unit.trays[i];
      hash = hash_mix(hash, tray.present ? 1U : 0U);
      hash = hash_mix(hash, tray.active ? 1U : 0U);
      hash = hash_mix(hash, tray.color_rgba);
      hash = hash_mix(hash, tray.remain_pct < 0 ? 0xFFFFFFFFU : static_cast<uint32_t>(tray.remain_pct));
      hash = hash_string(hash, tray.material_type);
      hash = hash_string(hash, tray.material_name);
    }
  }

  const AmsTrayInfo& ext = snapshot.ams->external_spool;
  hash = hash_mix(hash, ext.present ? 1U : 0U);
  hash = hash_mix(hash, ext.active ? 1U : 0U);
  hash = hash_mix(hash, ext.color_rgba);
  hash = hash_mix(hash, ext.remain_pct < 0 ? 0xFFFFFFFFU : static_cast<uint32_t>(ext.remain_pct));
  hash = hash_string(hash, ext.material_type);
  hash = hash_string(hash, ext.material_name);

  // Fold HMS codes (subset relevant to AMS) into the signature so the error
  // overlay updates when codes appear/disappear.
  for (uint64_t code : snapshot.hms_codes) {
    const uint32_t attr = static_cast<uint32_t>(code >> 32);
    if (((attr >> 24) & 0xFFU) == 0x07U) {
      hash = hash_mix(hash, static_cast<uint32_t>(code & 0xFFFFFFFFU));
      hash = hash_mix(hash, attr);
    }
  }
  return hash;
}

// Ambient sweep (kAmbientSweep) removed — the rotating arc during printing caused
// excessive LVGL redraws.  Filament load/unload and color pulse animations are kept.

enum class RingAnimKind : uint8_t {
  kNone,           // Static — no animation
  kFilamentLoad,   // Arc value sweeps 0→100, repeat
  kFilamentUnload, // Arc value sweeps 100→0, repeat
  kPulseBoth,      // Both MAIN & INDICATOR color pulse
  kPulseIndicator, // Only INDICATOR color pulses
  // kAmbientSweep removed — caused excessive LVGL redraws
};

struct RingVisual {
  uint32_t main_hex = kRingBaseDark;
  uint32_t indicator_hex = 0xFFFFFF;
  int value_override = -1;
  RingAnimKind anim_kind = RingAnimKind::kNone;
  uint32_t pulse_base_hex = 0;
  uint32_t pulse_period_ms = 0;
  bool animated() const { return anim_kind != RingAnimKind::kNone; }
};

RingVisual lifecycle_ring_visual(const PrinterSnapshot& snapshot, const ArcColorScheme& colors) {
  const int progress = std::clamp(static_cast<int>(std::lround(snapshot.progress_percent)), 0, 100);
  RingVisual visual = {};

  // Filament load/unload animation — direction derived from resolver's ui_status.
  if (snapshot.ui_status == "loading" || snapshot.ui_status == "unloading") {
    visual.main_hex = kRingBaseDark;
    visual.indicator_hex = colors.filament;
    visual.anim_kind = (snapshot.ui_status == "loading") ? RingAnimKind::kFilamentLoad
                                                         : RingAnimKind::kFilamentUnload;
    return visual;
  }

  // Connection-level states (independent of print status).
  if (snapshot.connection == PrinterConnectionState::kWaitingForCredentials) {
    visual.main_hex = colors.setup;
    visual.indicator_hex = colors.setup;
    return visual;
  }
  if (snapshot.connection == PrinterConnectionState::kError || snapshot.has_error ||
      snapshot.lifecycle == PrintLifecycleState::kError) {
    visual.main_hex = colors.error;
    visual.indicator_hex = colors.error;
    visual.anim_kind = RingAnimKind::kPulseBoth;
    visual.pulse_base_hex = colors.error;
    visual.pulse_period_ms = 1600U;
    return visual;
  }
  if (!snapshot.wifi_connected) {
    visual.main_hex = colors.offline;
    visual.indicator_hex = colors.offline;
    return visual;
  }

  // All remaining classifications come from the resolver (ui_status / lifecycle).
  if (snapshot.ui_status == "done" || snapshot.lifecycle == PrintLifecycleState::kFinished) {
    visual.main_hex = colors.done;
    visual.indicator_hex = colors.done;
    return visual;
  }

  if (snapshot.ui_status == "downloading") {
    visual.main_hex = kRingBaseDark;
    visual.indicator_hex = colors.preheat;
    visual.value_override = progress;
    return visual;
  }

  if (snapshot.ui_status == "preheating" || snapshot.ui_status == "preparing" ||
      snapshot.lifecycle == PrintLifecycleState::kPreparing) {
    visual.main_hex = colors.preheat;
    visual.indicator_hex = colors.preheat;
    visual.anim_kind = RingAnimKind::kPulseBoth;
    visual.pulse_base_hex = colors.preheat;
    visual.pulse_period_ms = 1400U;
    return visual;
  }

  if (snapshot.ui_status == "clean nozzle") {
    visual.main_hex = colors.clean;
    visual.indicator_hex = colors.clean;
    visual.anim_kind = RingAnimKind::kPulseBoth;
    visual.pulse_base_hex = colors.clean;
    visual.pulse_period_ms = 1200U;
    return visual;
  }

  if (snapshot.ui_status == "bed level") {
    visual.main_hex = colors.level;
    visual.indicator_hex = colors.level;
    visual.anim_kind = RingAnimKind::kPulseBoth;
    visual.pulse_base_hex = colors.level;
    visual.pulse_period_ms = 1400U;
    return visual;
  }

  if (snapshot.ui_status == "cooling") {
    visual.main_hex = colors.cool;
    visual.indicator_hex = colors.cool;
    return visual;
  }

  if (snapshot.ui_status == "offline") {
    visual.main_hex = colors.offline;
    visual.indicator_hex = colors.offline;
    return visual;
  }

  // Lifecycle-based fallback for states where ui_status is a free-form string.
  switch (snapshot.lifecycle) {
    case PrintLifecycleState::kPrinting:
    case PrintLifecycleState::kPaused:
      visual.main_hex = kRingBaseDark;
      visual.indicator_hex = colors.printing;
      return visual;
    case PrintLifecycleState::kFinished:
      visual.main_hex = colors.done;
      visual.indicator_hex = colors.done;
      return visual;
    case PrintLifecycleState::kIdle:
      if (snapshot.print_active) {
        visual.main_hex = kRingBaseDark;
        visual.indicator_hex = colors.idle_active;
      } else {
        visual.main_hex = colors.idle;
        visual.indicator_hex = colors.idle;
      }
      return visual;
    case PrintLifecycleState::kUnknown:
    default:
      break;
  }

  if (snapshot.connection == PrinterConnectionState::kBooting ||
      snapshot.connection == PrinterConnectionState::kConnecting ||
      snapshot.connection == PrinterConnectionState::kReadyForLanConnect) {
    visual.main_hex = colors.setup;
    visual.indicator_hex = colors.setup;
    return visual;
  }

  visual.main_hex = colors.unknown;
  visual.indicator_hex = colors.unknown;
  return visual;
}

uint32_t stable_status_text_hex(const PrinterSnapshot& snapshot, const ArcColorScheme& colors) {
  // Filament
  if (snapshot.ui_status == "loading" || snapshot.ui_status == "unloading") {
    return colors.filament;
  }

  // Connection-level states
  if (snapshot.connection == PrinterConnectionState::kWaitingForCredentials) {
    return colors.setup;
  }
  if (snapshot.connection == PrinterConnectionState::kError || snapshot.has_error ||
      snapshot.lifecycle == PrintLifecycleState::kError) {
    return colors.error;
  }
  if (!snapshot.wifi_connected) {
    return colors.offline;
  }

  // Resolver-classified states
  if (snapshot.ui_status == "done" || snapshot.lifecycle == PrintLifecycleState::kFinished) {
    return colors.done;
  }
  if (snapshot.ui_status == "downloading") {
    return colors.preheat;
  }
  if (snapshot.ui_status == "preheating" || snapshot.ui_status == "preparing" ||
      snapshot.lifecycle == PrintLifecycleState::kPreparing) {
    return colors.preheat;
  }
  if (snapshot.ui_status == "clean nozzle") {
    return colors.clean;
  }
  if (snapshot.ui_status == "bed level") {
    return colors.level;
  }
  if (snapshot.ui_status == "cooling") {
    return colors.cool;
  }
  if (snapshot.ui_status == "offline") {
    return colors.offline;
  }

  // Lifecycle-based fallback
  switch (snapshot.lifecycle) {
    case PrintLifecycleState::kPrinting:
    case PrintLifecycleState::kPaused:
      return colors.printing;
    case PrintLifecycleState::kPreparing:
      return colors.preheat;
    case PrintLifecycleState::kFinished:
      return colors.done;
    case PrintLifecycleState::kIdle:
      return snapshot.print_active ? colors.idle_active : colors.idle;
    case PrintLifecycleState::kUnknown:
    default:
      break;
  }

  if (snapshot.connection == PrinterConnectionState::kBooting ||
      snapshot.connection == PrinterConnectionState::kConnecting ||
      snapshot.connection == PrinterConnectionState::kReadyForLanConnect) {
    return colors.setup;
  }

  return colors.unknown;
}

// ring_visual_is_animated() removed — animation state is now tracked
// via active_ring_anim_kind_ in apply_ring_visual_locked().

std::string lifecycle_label(const PrinterSnapshot& snapshot) {
  if (snapshot.connection == PrinterConnectionState::kWaitingForCredentials) {
    return "setup";
  }
  if (snapshot.connection == PrinterConnectionState::kConnecting && !snapshot.wifi_connected) {
    return "syncing";
  }
  // Let filament ui_status (loading/unloading) take priority over has_error,
  // because Bambu sends user-prompt error codes during filament changes.
  if (snapshot.ui_status == "loading" || snapshot.ui_status == "unloading") {
    return snapshot.ui_status;
  }
  if (snapshot.connection == PrinterConnectionState::kError || snapshot.has_error ||
      snapshot.lifecycle == PrintLifecycleState::kError) {
    return "failed";
  }
  if (snapshot.connection == PrinterConnectionState::kBooting) {
    return "booting";
  }
  if (snapshot.connection == PrinterConnectionState::kReadyForLanConnect) {
    return "ready";
  }
  if (!snapshot.ui_status.empty()) {
    return snapshot.ui_status;
  }

  switch (snapshot.lifecycle) {
    case PrintLifecycleState::kPreparing:
      return "preparing";
    case PrintLifecycleState::kPrinting:
      return "printing";
    case PrintLifecycleState::kPaused:
      return "paused";
    case PrintLifecycleState::kFinished:
      return "done";
    case PrintLifecycleState::kIdle:
      return "idle";
    case PrintLifecycleState::kUnknown:
    default:
      return snapshot.wifi_connected ? "waiting..." : "offline";
  }
}

std::string display_text_for_label(const std::string& raw, const char* semantic_fallback);
std::string short_portal_hint_for_display(const PrinterSnapshot& snapshot, bool include_pin);
bool looks_like_serial(const std::string& text);
std::string shortened_serial_for_display(const std::string& serial);
std::string printer_primary_title_for_display(const std::string& name, const std::string& model);
std::string printer_secondary_for_display(const std::string& name, const std::string& model);

std::string setup_access_text(const PrinterSnapshot& snapshot) {
  std::string text;
  if (!snapshot.setup_ap_ssid.empty()) {
    text = "AP: " + (kOnxLandscapeLayout
                         ? display_text_for_label(snapshot.setup_ap_ssid, "Setup SSID")
                         : snapshot.setup_ap_ssid);
  }
  if (!snapshot.setup_ap_password.empty()) {
    if (!text.empty()) {
      text += "\n";
    }
    text += "PW: " + snapshot.setup_ap_password;
  }

  const std::string ip = snapshot.setup_ap_ip.empty() ? "192.168.4.1" : snapshot.setup_ap_ip;
  if (!ip.empty()) {
    if (!text.empty()) {
      text += "\n";
    }
    text += "Open " + ip;
  }

  return text;
}

std::string station_portal_text(const PrinterSnapshot& snapshot) {
  if (!snapshot.wifi_ip.empty()) {
    return "Open " + snapshot.wifi_ip;
  }
  return "Open the portal on your Wi-Fi IP";
}

std::string detail_text(const PrinterSnapshot& snapshot) {
  if (snapshot.connection == PrinterConnectionState::kWaitingForCredentials) {
    if (snapshot.setup_ap_active) {
      return setup_access_text(snapshot);
    }
    if (snapshot.wifi_connected) {
      return station_portal_text(snapshot);
    }
    return {};
  }
  if (!snapshot.wifi_connected) {
    if (snapshot.setup_ap_active) {
      return setup_access_text(snapshot);
    }
    return {};
  }
  if (snapshot.has_error && !snapshot.detail.empty()) {
    return snapshot.detail;
  }
  if (!snapshot.job_name.empty() &&
      (snapshot.lifecycle == PrintLifecycleState::kPreparing ||
       snapshot.lifecycle == PrintLifecycleState::kPrinting ||
       snapshot.lifecycle == PrintLifecycleState::kPaused)) {
    return snapshot.job_name;
  }
  if (!snapshot.detail.empty() && snapshot.detail != snapshot.stage &&
      snapshot.detail != "Connected to local Bambu MQTT" &&
      snapshot.detail != "Printer version info received" &&
      snapshot.detail != "Status payload received") {
    return snapshot.detail;
  }
  return {};
}

std::string layer_text(const PrinterSnapshot& snapshot) {
  char buffer[32] = {};
  if (snapshot.total_layers > 0) {
    std::snprintf(buffer, sizeof(buffer), "Layer: %u / %u", snapshot.current_layer,
                  snapshot.total_layers);
  } else if (snapshot.current_layer > 0) {
    std::snprintf(buffer, sizeof(buffer), "Layer: %u / --", snapshot.current_layer);
  } else {
    std::snprintf(buffer, sizeof(buffer), "Layer: -- / --");
  }
  return buffer;
}

std::string compact_layer_text(const PrinterSnapshot& snapshot) {
  char buffer[24] = {};
  if (snapshot.total_layers > 0) {
    std::snprintf(buffer, sizeof(buffer), "%u/%u", snapshot.current_layer,
                  snapshot.total_layers);
  } else if (snapshot.current_layer > 0) {
    std::snprintf(buffer, sizeof(buffer), "%u/--", snapshot.current_layer);
  } else {
    std::snprintf(buffer, sizeof(buffer), "--/--");
  }
  return buffer;
}

std::string remaining_text(const PrinterSnapshot& snapshot) {
  if (snapshot.ui_status == "done" || snapshot.lifecycle == PrintLifecycleState::kFinished) {
    return "Done";
  }
  if (snapshot.remaining_seconds == 0) {
    return "--m";
  }

  const uint32_t minutes_total = snapshot.remaining_seconds / 60U;
  const uint32_t hours = minutes_total / 60U;
  const uint32_t minutes = minutes_total % 60U;
  char buffer[24] = {};
  if (hours > 0U) {
    std::snprintf(buffer, sizeof(buffer), "%uh %um", static_cast<unsigned int>(hours),
                  static_cast<unsigned int>(minutes));
  } else {
    std::snprintf(buffer, sizeof(buffer), "%um", static_cast<unsigned int>(minutes));
  }
  return buffer;
}

// Wall-clock predicted finish time as "HH:MM". Falls back to the regular
// remaining-duration text when SNTP has not synced yet (year < 2024) or
// when no remaining time is reported.
std::string eta_text(const PrinterSnapshot& snapshot) {
  if (snapshot.ui_status == "done" || snapshot.lifecycle == PrintLifecycleState::kFinished) {
    return "Done";
  }
  if (snapshot.remaining_seconds == 0) {
    return "--:--";
  }
  const std::time_t now = std::time(nullptr);
  if (now < 1700000000) {
    // Wall clock not yet synced — fall back to duration so the row stays useful.
    return remaining_text(snapshot);
  }
  const std::time_t finish = now + static_cast<std::time_t>(snapshot.remaining_seconds);
  std::tm local{};
  if (localtime_r(&finish, &local) == nullptr) {
    return remaining_text(snapshot);
  }
  char buffer[8] = {};
  std::snprintf(buffer, sizeof(buffer), "%02d:%02d", local.tm_hour, local.tm_min);
  return buffer;
}

bool is_remaining_duration_value(const std::string& value) {
  if (value == "--m") {
    return true;
  }
  bool has_digit = false;
  bool has_duration_unit = false;
  for (const char ch : value) {
    if (ch >= '0' && ch <= '9') {
      has_digit = true;
    } else if (ch == 'h' || ch == 'm') {
      has_duration_unit = true;
    }
  }
  return has_digit && has_duration_unit;
}

std::string display_text_for_label(const std::string& raw, const char* /*semantic_fallback*/) {
  return raw;
}

const lv_font_t* dynamic_text_font() {
  return kOnxLandscapeLayout ? &onx_cjk_16 : nullptr;
}

void set_dynamic_text_font(lv_obj_t* label) {
  const lv_font_t* font = dynamic_text_font();
  if (label != nullptr && font != nullptr) {
    lv_obj_set_style_text_font(label, font, 0);
  }
}

bool looks_like_serial(const std::string& text) {
  if (text.size() < 12U || text.size() > 32U) {
    return false;
  }
  bool has_digit = false;
  bool has_alpha = false;
  for (unsigned char c : text) {
    if (std::isdigit(c)) {
      has_digit = true;
    } else if (std::isalpha(c)) {
      has_alpha = true;
    } else if (c != '-' && c != '_') {
      return false;
    }
  }
  return has_digit && has_alpha;
}

std::string shortened_serial_for_display(const std::string& serial) {
  if (serial.size() <= 12U) {
    return serial;
  }
  return "SN " + serial.substr(0, 6) + "..." + serial.substr(serial.size() - 4);
}

bool model_is_displayable(const std::string& model) {
  return !model.empty() && model != "Unknown" && model != "unknown";
}

std::string printer_primary_title_for_display(const std::string& name, const std::string& model) {
  if (!name.empty() && !looks_like_serial(name)) {
    return display_text_for_label(name, "Printer name");
  }
  if (model_is_displayable(model)) {
    return display_text_for_label(model, "Printer name");
  }
  return looks_like_serial(name) ? "Cloud printer" : "Printer";
}

std::string printer_secondary_for_display(const std::string& name, const std::string& model) {
  if (looks_like_serial(name)) {
    if (model_is_displayable(model)) {
      return display_text_for_label(model, "Printer name") + "  " +
             name.substr(name.size() - std::min<size_t>(4U, name.size()));
    }
    return shortened_serial_for_display(name);
  }
  return model.empty() ? "Unknown" : display_text_for_label(model, "Printer name");
}

std::string short_portal_hint_for_display(const PrinterSnapshot& snapshot, bool include_pin) {
  std::string text;
  if (snapshot.setup_ap_active) {
    const std::string ip = snapshot.setup_ap_ip.empty() ? "192.168.4.1" : snapshot.setup_ap_ip;
    text = "Open " + ip;
  } else if (!snapshot.wifi_ip.empty()) {
    text = "Open " + snapshot.wifi_ip;
  } else if (snapshot.wifi_connected) {
    text = "Open portal on Wi-Fi IP";
  }
  if (include_pin) {
    if (!text.empty()) {
      text += " | ";
    }
    text += "Hold for PIN";
  }
  return text;
}

std::string portal_ip_for_display(const PrinterSnapshot& snapshot) {
  if (snapshot.setup_ap_active) {
    return snapshot.setup_ap_ip.empty() ? "192.168.4.1" : snapshot.setup_ap_ip;
  }
  return snapshot.wifi_ip;
}

std::string preview_note_text(const PrinterSnapshot& snapshot) {
  if (snapshot.preview_blob && !snapshot.preview_blob->empty()) {
    return {};  // note hidden when cover is loaded — title takes over
  }
  if (!snapshot.preview_url.empty()) {
    return "Loading cloud cover";
  }
  if (snapshot.connection == PrinterConnectionState::kWaitingForCredentials) {
    return "Set up printer";
  }
  if (!snapshot.wifi_connected) {
    return "Printer offline";
  }
  if (!snapshot.cloud_detail.empty() && !snapshot.cloud_connected) {
    return "Connecting to cloud";
  }

  switch (snapshot.lifecycle) {
    case PrintLifecycleState::kPreparing:
    case PrintLifecycleState::kPrinting:
    case PrintLifecycleState::kPaused:
      return "Preparing cover";
    case PrintLifecycleState::kFinished:
      return "Last print done";
    case PrintLifecycleState::kError:
      return "Cover unavailable";
    case PrintLifecycleState::kIdle:
    case PrintLifecycleState::kUnknown:
    default:
      return "No active print";
  }
}

std::string preview_subnote_text(const PrinterSnapshot& snapshot) {
  if (snapshot.print_active && !snapshot.job_name.empty()) {
    return snapshot.job_name;
  }
  if (!snapshot.preview_title.empty()) {
    return snapshot.preview_title;
  }
  if (!snapshot.job_name.empty()) {
    return snapshot.job_name;
  }
  if (!snapshot.cloud_detail.empty()) {
    return snapshot.cloud_detail;
  }
  return snapshot.preview_hint;
}

std::string camera_note_text(const PrinterSnapshot& snapshot) {
  if (snapshot.camera_blob && !snapshot.camera_blob->empty()) {
    // Image loaded: show printer status (downloading / clean nozzle /
    // printing / paused / failed / ...) above the JPEG.
    return lifecycle_label(snapshot);
  }
  if (snapshot.connection == PrinterConnectionState::kWaitingForCredentials) {
    return "Set up printer";
  }
  if (!snapshot.wifi_connected) {
    return "Camera offline";
  }
  if (!snapshot.camera_detail.empty()) {
    return snapshot.camera_detail;
  }
  return "Tap for new image";
}

std::string camera_subnote_text(const PrinterSnapshot& snapshot) {
  if (snapshot.camera_blob && !snapshot.camera_blob->empty()) {
    // Image is loaded: show layer progress below the snapshot so the cam page
    // carries the print-in-progress info that the dashboard usually owns.
    if (snapshot.total_layers > 0 || snapshot.current_layer > 0) {
      return layer_text(snapshot);
    }
    if (!snapshot.job_name.empty()) {
      return snapshot.job_name;
    }
    return {};
  }
  if (!snapshot.job_name.empty()) {
    return snapshot.job_name;
  }
  return "Auto-refresh every 2s";
}

const char* mdi_battery_symbol(const PrinterSnapshot& snapshot) {
  if (!snapshot.usb_present && snapshot.battery_percent == 0U && !snapshot.charging) {
    return kMdiBatteryAlert;
  }

  if (snapshot.charging) {
    if (snapshot.battery_percent >= 95U) return kMdiBatteryCharging100;
    if (snapshot.battery_percent >= 85U) return kMdiBatteryCharging90;
    if (snapshot.battery_percent >= 75U) return kMdiBatteryCharging80;
    if (snapshot.battery_percent >= 65U) return kMdiBatteryCharging70;
    if (snapshot.battery_percent >= 55U) return kMdiBatteryCharging60;
    if (snapshot.battery_percent >= 45U) return kMdiBatteryCharging50;
    if (snapshot.battery_percent >= 35U) return kMdiBatteryCharging40;
    if (snapshot.battery_percent >= 25U) return kMdiBatteryCharging30;
    if (snapshot.battery_percent >= 15U) return kMdiBatteryCharging20;
    if (snapshot.battery_percent >= 5U) return kMdiBatteryCharging10;
    return kMdiBatteryCharging0;
  }

  if (snapshot.battery_percent >= 95U) return kMdiBattery100;
  if (snapshot.battery_percent >= 85U) return kMdiBattery90;
  if (snapshot.battery_percent >= 75U) return kMdiBattery80;
  if (snapshot.battery_percent >= 65U) return kMdiBattery70;
  if (snapshot.battery_percent >= 55U) return kMdiBattery60;
  if (snapshot.battery_percent >= 45U) return kMdiBattery50;
  if (snapshot.battery_percent >= 35U) return kMdiBattery40;
  if (snapshot.battery_percent >= 25U) return kMdiBattery30;
  if (snapshot.battery_percent >= 15U) return kMdiBattery20;
  return kMdiBattery10;
}

std::string battery_icon_text(const PrinterSnapshot& snapshot) {
  return mdi_battery_symbol(snapshot);
}

std::string battery_pct_text(const PrinterSnapshot& snapshot) {
  if (!snapshot.usb_present && snapshot.battery_percent == 0U && !snapshot.charging) {
    return "--%";
  }

  char buffer[8] = {};
  std::snprintf(buffer, sizeof(buffer), "%u%%", snapshot.battery_percent);
  return buffer;
}

std::string short_duration_text(uint32_t total_seconds) {
  const uint32_t minutes = total_seconds / 60U;
  const uint32_t seconds = total_seconds % 60U;
  char buffer[24] = {};
  if (minutes > 0U) {
    std::snprintf(buffer, sizeof(buffer), "%um %us", static_cast<unsigned int>(minutes),
                  static_cast<unsigned int>(seconds));
  } else {
    std::snprintf(buffer, sizeof(buffer), "%us", static_cast<unsigned int>(seconds));
  }
  return buffer;
}

bool should_show_logo(const PrinterSnapshot& snapshot) {
  if (!snapshot.wifi_connected) {
    return false;
  }

  switch (snapshot.connection) {
    case PrinterConnectionState::kBooting:
    case PrinterConnectionState::kWaitingForCredentials:
    case PrinterConnectionState::kConnecting:
      return false;
    case PrinterConnectionState::kReadyForLanConnect:
    case PrinterConnectionState::kOnline:
    case PrinterConnectionState::kError:
    default:
      return true;
  }
}

void dot_pulse_exec_cb(void* obj, int32_t val) {
  lv_obj_set_style_bg_opa(static_cast<lv_obj_t*>(obj), static_cast<lv_opa_t>(val), 0);
}

void start_dot_pulse(lv_obj_t* dot) {
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, dot);
  lv_anim_set_exec_cb(&a, dot_pulse_exec_cb);
  lv_anim_set_values(&a, kDotPulseOpaMin, kDotPulseOpaMax);
  lv_anim_set_duration(&a, kDotPulseDurationMs);
  lv_anim_set_reverse_duration(&a, kDotPulseDurationMs);
  lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
  lv_anim_start(&a);
}

// Micro-interaction: uniform scale on card tap (256 = 100% in LVGL 9)
void card_scale_exec_cb(void* obj, int32_t val) {
  lv_obj_set_style_transform_scale(static_cast<lv_obj_t*>(obj), val, 0);
}

// Cascading reveal: vertical slide-in per card
void card_reveal_y_exec_cb(void* obj, int32_t val) {
  lv_obj_set_style_translate_y(static_cast<lv_obj_t*>(obj), val, 0);
}

// Cascading reveal: fade-in per card
void card_reveal_opa_exec_cb(void* obj, int32_t val) {
  lv_obj_set_style_opa(static_cast<lv_obj_t*>(obj), static_cast<lv_opa_t>(val), 0);
}

}  // namespace

void Ui::set_display_rotation(DisplayRotation rotation) {
  if (initialized_) {
    return;
  }
  display_rotation_ = rotation;
}

esp_err_t Ui::initialize() {
  if (initialized_) {
    return ESP_OK;
  }

  portal_hint_boot_ms_ = static_cast<uint64_t>(esp_timer_get_time() / 1000ULL);

  bsp_display_cfg_t display_cfg = {
      .lv_adapter_cfg = []() {
        esp_lv_adapter_config_t cfg = ESP_LV_ADAPTER_DEFAULT_CONFIG();
        // Pin LVGL render task to core 1 so WiFi/BT on core 0 can't interrupt rendering.
        cfg.task_core_id = 1;
        return cfg;
      }(),
      .rotation = ESP_LV_ADAPTER_ROTATE_0,
      // TE_SYNC is safe again: managed_components/espressif__esp_lvgl_adapter
      // is locally patched to use bounded timeouts in te_sync_wait_for_vsync()
      // and the SPI TX-done notify (lvgl_bridge_v9.c). Without TE on this
      // panel + adapter v0.4.2, single PSRAM buffer tears badly and the
      // unbounded LVGL flush rate steals CPU/PSRAM-bus from mbedTLS, which
      // causes printer-MQTT TLS handshakes to time out (select() timeout).
      .tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_TE_SYNC,
      .touch_flags = {
          .swap_xy = 0,
          .mirror_x = 1,
          .mirror_y = 1,
      },
  };
  apply_touch_rotation_flags(display_rotation_, &display_cfg);

  display_ = bsp_display_start_with_config(&display_cfg);
  if (display_ == nullptr) {
    ESP_LOGE(kTag, "bsp_display_start failed");
    return ESP_FAIL;
  }

  ESP_RETURN_ON_ERROR(bsp_display_rotation_set(bsp_rotation_for(display_rotation_)), kTag,
                      "apply display rotation failed");

  user_brightness_percent_ = -1;
  applied_brightness_percent_ = -1;
  screen_power_mode_ = ScreenPowerMode::kAwake;
  last_activity_tick_ms_.store(lv_tick_get());
  set_brightness_percent(kDefaultBrightnessPercent);
  ESP_RETURN_ON_ERROR(build_dashboard(), kTag, "build_dashboard failed");

  // With LV_SCROLL_SNAP_NONE the pager decelerates freely after finger release.
  // scroll_throw=90 shrinks velocity to ~10% per tick, reaching zero in ~3 ticks
  // (~30 ms). SCROLL_END fires almost immediately and our single set_active_page()
  // animation takes over cleanly — no competing LVGL snap animations.
  {
    lv_indev_t* indev = lv_indev_get_next(nullptr);
    while (indev != nullptr) {
      if (lv_indev_get_type(indev) == LV_INDEV_TYPE_POINTER) {
        lv_indev_set_scroll_throw(indev, 90);
        break;
      }
      indev = lv_indev_get_next(indev);
    }
  }

  // Ambient sweep timer removed — only pulse/filament animations remain (driven by lv_anim_t).

  initialized_ = true;
  ESP_LOGI(kTag, "UI ready with YAML-style pager layout (rotation=%s)",
           to_string(display_rotation_));
  return ESP_OK;
}

void Ui::set_arc_color_scheme(const ArcColorScheme& colors) {
  if (!initialized_) {
    arc_colors_ = colors;
    return;
  }

  LvglLockGuard lock(200, "set_arc_color");
  if (!lock.locked()) {
    return;
  }

  arc_colors_ = colors;
  if (last_snapshot_.ui_status.empty() && last_snapshot_.stage.empty() &&
      last_snapshot_.detail.empty()) {
    return;
  }
  apply_ring_visual_locked(last_snapshot_);
}

bool Ui::consume_camera_refresh_request() {
  std::lock_guard<std::mutex> lock(camera_refresh_mutex_);
  const bool requested = camera_refresh_requested_;
  camera_refresh_requested_ = false;
  return requested;
}

bool Ui::consume_chamber_light_toggle_request() {
  return chamber_light_toggle_requested_.exchange(false);
}

bool Ui::consume_portal_unlock_request() {
  return portal_unlock_requested_.exchange(false);
}

void Ui::set_portal_access_state(bool lock_enabled, bool request_authorized,
                                 bool session_active, bool pin_active,
                                 const std::string& pin_code, uint32_t pin_remaining_s,
                                 uint32_t session_remaining_s) {
  if (!initialized_) {
    return;
  }

  // Store portal data without LVGL lock.  The actual text computation and
  // visual update happen inside apply_snapshot_locked() which already holds
  // the lock.  This eliminates a separate lock acquisition per tick.
  portal_lock_enabled_ = lock_enabled;
  portal_request_authorized_ = request_authorized;
  portal_session_active_ = session_active;
  portal_pin_active_ = pin_active;
  portal_pin_code_ = pin_code;
  portal_pin_remaining_s_ = pin_remaining_s;
  portal_session_remaining_s_ = session_remaining_s;
}

void Ui::compute_portal_texts_locked() {
  const bool provisioning_context =
      last_snapshot_.setup_ap_active ||
      last_snapshot_.connection == PrinterConnectionState::kWaitingForCredentials;
  const bool station_portal_available =
      !last_snapshot_.setup_ap_active && last_snapshot_.wifi_connected && !last_snapshot_.wifi_ip.empty();

  if (portal_pin_active_) {
    portal_hint_text_ = portal_request_authorized_ ? "Web Config PIN active on the display"
                                                   : "Enter the PIN shown on the display";
    portal_overlay_title_text_ = "WEB CONFIG PIN";
    portal_overlay_value_text_ = portal_pin_code_;
    portal_overlay_detail_text_ =
        "Valid for " + short_duration_text(std::max<uint32_t>(portal_pin_remaining_s_, 1U)) +
        ". Enter it in the browser.";
  } else {
    portal_overlay_title_text_.clear();
    portal_overlay_value_text_.clear();
    portal_overlay_detail_text_.clear();
    if (portal_session_active_ && portal_request_authorized_) {
      portal_hint_text_ =
          "Web Config unlocked for " +
          short_duration_text(std::max<uint32_t>(portal_session_remaining_s_, 1U));
    } else if (provisioning_context) {
      portal_hint_text_ =
          kOnxLandscapeLayout ? short_portal_hint_for_display(last_snapshot_, portal_lock_enabled_) : "";
    } else if (!portal_lock_enabled_) {
      portal_hint_text_ =
          station_portal_available ? ("Open " + last_snapshot_.wifi_ip) : "Web Config open";
    } else if (kOnxLandscapeLayout && last_snapshot_.wifi_connected) {
      portal_hint_text_ = short_portal_hint_for_display(last_snapshot_, true);
    } else if (portal_hint_boot_ms_ != 0 &&
               static_cast<uint64_t>(esp_timer_get_time() / 1000ULL) <
                   portal_hint_boot_ms_ + kPortalHintIntroMs) {
      portal_hint_text_ = station_portal_available
                              ? ("Open " + last_snapshot_.wifi_ip + " | Hold for PIN")
                              : "Hold for PIN";
    } else {
      portal_hint_text_.clear();
    }
  }

  update_portal_access_visuals_locked();
}

// --- Page 0: printer cards ---

void Ui::printer_card_click_cb(lv_event_t* event) {
  auto* self = static_cast<Ui*>(lv_event_get_user_data(event));
  lv_obj_t* target = static_cast<lv_obj_t*>(lv_event_get_target(event));
  for (const auto& cw : self->page0_cards_) {
    if (cw.card == target) {
      self->pending_printer_switch_ = cw.profile_index;
      self->note_activity(true);

      // Micro-interaction: quick scale bounce (100% → 91% → 100%)
      lv_anim_t sa;
      lv_anim_init(&sa);
      lv_anim_set_var(&sa, target);
      lv_anim_set_exec_cb(&sa, card_scale_exec_cb);
      lv_anim_set_values(&sa, 256, 233);
      lv_anim_set_duration(&sa, 75);
      lv_anim_set_reverse_duration(&sa, 110);
      lv_anim_set_path_cb(&sa, lv_anim_path_ease_out);
      lv_anim_start(&sa);
      break;
    }
  }
}

int Ui::consume_printer_switch_request() {
  int val = pending_printer_switch_;
  pending_printer_switch_ = -1;
  return val;
}

void Ui::apply_page0_parallax(bool force) {
  if (page0_title_ == nullptr || page0_card_list_ == nullptr || pager_ == nullptr) {
    return;
  }

  if (kOnxUiLayout) {
    set_hidden(fixed_overlay_, true);
    set_hidden(status_arc_, true);
    return;
  }

  int scroll_x = lv_obj_get_scroll_x(pager_);
  if (scroll_x < 0) scroll_x = 0;

  const int page_w = board::kDisplayWidth;
  const int clamped = (scroll_x > page_w) ? page_w : scroll_x;
  if (!force && clamped == last_parallax_clamped_) {
    return;
  }
  last_parallax_clamped_ = clamped;

  // Page 0 content: fade out + shift up as we scroll away
  const int32_t title_y = static_cast<int32_t>(kParallaxTitleMaxY * clamped / page_w);
  const int32_t cards_y = static_cast<int32_t>(kParallaxCardsMaxY * clamped / page_w);
  const lv_opa_t page0_opa = static_cast<lv_opa_t>(255 - 255 * clamped / page_w);

  lv_obj_set_style_translate_y(page0_title_, title_y, 0);
  lv_obj_set_style_translate_y(page0_card_list_, cards_y, 0);
  // NOTE: lv_obj_set_style_opa() on large containers forces LVGL to render the entire
  // widget tree into a temporary offscreen layer before blending it at the given opacity.
  // With LV_DRAW_LAYER_SIMPLE_BUF_SIZE=24KB on a 466px-wide display that means ~14 layer
  // passes for the card list alone — triggered every LV_EVENT_SCROLL. This reduces scroll
  // fps to ~4. Use only translate_y for the parallax shift; no opa on large containers.
  if (page0_empty_note_ != nullptr) {
    // empty_note is a single small label — opa is cheap here
    lv_obj_set_style_opa(page0_empty_note_, page0_opa, 0);
  }

  // Overlay (arc, progress, battery): fade in as we leave page 0
  const lv_opa_t overlay_opa = static_cast<lv_opa_t>(255 * clamped / page_w);

  // fixed_overlay_ contains the full-screen arc (466×466 px).  Setting opa to any
  // fractional value forces LVGL to render the entire widget tree into a 24 KB
  // offscreen-layer buffer → ~18 layer-passes per scroll event → measurable stutter.
  // Instead we snap to binary visibility: hidden while page 0 dominates, fully
  // opaque once we cross the midpoint toward page 1.  Exactly 0 and LV_OPA_COVER
  // are both "fully decided" → LVGL skips the layer path entirely.
  if (clamped < page_w / 2) {
    set_hidden(fixed_overlay_, true);
  } else {
    set_hidden(fixed_overlay_, false);
    lv_obj_set_style_opa(fixed_overlay_, LV_OPA_COVER, 0);
  }

  // Small text labels: opa fade is cheap (entire label fits in the 24 KB layer buffer
  // in a single pass), so the visual cross-fade is worth keeping.
  lv_obj_set_style_opa(progress_label_, overlay_opa, 0);
  lv_obj_set_style_opa(battery_icon_label_, overlay_opa, 0);
  lv_obj_set_style_opa(battery_pct_label_, overlay_opa, 0);
}

void Ui::update_printer_cards(const std::vector<PrinterCardInfo>& cards) {
  if (!initialized_) {
    return;
  }

  // Only rebuild (and trigger the reveal animation) when the card data has
  // actually changed — the caller runs every main-loop tick, so without this
  // guard the animation restarts hundreds of times per second.
  const auto cards_equal = [](const std::vector<PrinterCardInfo>& a,
                               const std::vector<PrinterCardInfo>& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
      if (a[i].index != b[i].index ||
          a[i].active != b[i].active ||
          a[i].connected != b[i].connected ||
          a[i].name != b[i].name ||
          a[i].model != b[i].model ||
          a[i].host != b[i].host) {
        return false;
      }
    }
    return true;
  };
  if (cards_equal(cards, last_printer_cards_)) {
    return;
  }
  last_printer_cards_ = cards;

  LvglLockGuard lock(200, "update_cards");
  if (!lock.locked()) {
    return;
  }

  rebuild_printer_cards_locked(cards);
}

void Ui::rebuild_printer_cards_locked(const std::vector<PrinterCardInfo>& cards) {
  // Remove old card widgets
  for (auto& cw : page0_cards_) {
    if (cw.card != nullptr) {
      lv_obj_delete(cw.card);
    }
  }
  page0_cards_.clear();

  const bool empty = cards.empty();
  set_hidden(page0_card_list_, empty);
  set_hidden(page0_empty_note_, !empty);
  if (kOnxLandscapeLayout && page0_detail_panel_ != nullptr) {
    set_hidden(page0_detail_panel_, false);
    const std::string portal_ip = portal_ip_for_display(last_snapshot_);
    if (empty) {
      set_label_text_if_changed(page0_empty_note_,
                                portal_ip.empty()
                                    ? "No printers configured.\nUse the web portal to add printers."
                                    : ("No printers configured.\nOpen " + portal_ip));
      set_label_text_if_changed(page0_detail_title_, "No printer");
      set_label_text_if_changed(page0_detail_state_, "No profile");
      set_label_text_if_changed(page0_detail_host_, "--");
      set_label_text_if_changed(page0_detail_hint_, "Web Config");
      set_label_text_if_changed(page0_detail_ip_, portal_ip.empty() ? "No IP" : portal_ip);
      set_label_text_if_changed(page0_detail_pin_hint_, portal_pin_active_ ? "PIN active" : "Hold PIN");
      lv_obj_set_style_text_color(page0_detail_state_, lv_color_hex(kOnxColorMuted), 0);
    } else {
      const PrinterCardInfo* selected = &cards.front();
      for (const auto& info : cards) {
        if (info.active) {
          selected = &info;
          break;
        }
      }
      const std::string title =
          printer_primary_title_for_display(selected->name, selected->model);
      const std::string state = selected->connected
          ? "Online"
          : (selected->active ? "Waiting for LAN" : "Profile ready");
      std::string host = selected->host.empty() ? "No local IP" : selected->host;
      if (looks_like_serial(selected->name)) {
        host = shortened_serial_for_display(selected->name);
      }
      set_label_text_if_changed(page0_detail_title_, title);
      set_label_text_if_changed(page0_detail_state_, state);
      set_label_text_if_changed(page0_detail_host_, host);
      set_label_text_if_changed(page0_detail_hint_, "Web Config");
      set_label_text_if_changed(page0_detail_ip_, portal_ip.empty() ? "No IP" : portal_ip);
      set_label_text_if_changed(page0_detail_pin_hint_, portal_pin_active_ ? "PIN active" : "Hold PIN");
      lv_obj_set_style_text_color(page0_detail_state_,
                                  lv_color_hex(selected->connected ? 0x4ADE80 : kOnxColorMuted),
                                  0);
    }
  }
  if (empty) {
    return;
  }

  const lv_font_t* font_name = &dosis_20;
  const lv_font_t* font_detail = &lv_font_montserrat_14;

  int card_idx = 0;
  for (const auto& info : cards) {
    // Card container — glasmorphism-lite: semi-transparent bg + shadow elevation
    lv_obj_t* card = lv_obj_create(page0_card_list_);
    lv_obj_set_size(card, kOnxUiLayout ? kOnxPrinterCardW : 340,
                    kOnxLandscapeLayout ? kOnxPrinterCardH : LV_SIZE_CONTENT);
    lv_obj_set_style_min_height(card, kOnxUiLayout ? kOnxPrinterCardH : 72, 0);
    lv_obj_set_style_bg_color(card,
                              lv_color_hex(kOnxUiLayout ? kOnxColorPanel2 : 0x1E1E1E), 0);
    lv_obj_set_style_bg_opa(card, kOnxUiLayout ? LV_OPA_COVER : 195, 0);
    lv_obj_set_style_radius(card, kOnxUiLayout ? 8 : 16, 0);
    lv_obj_set_style_pad_all(card, kOnxLandscapeLayout ? 0 : (kOnxUiLayout ? 10 : 12), 0);
    lv_obj_set_style_pad_row(card, 2, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    // Shadow for 3D depth on OLED
    lv_obj_set_style_shadow_width(card, kOnxUiLayout ? 0 : 20, 0);
    lv_obj_set_style_shadow_color(card, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(card, LV_OPA_50, 0);
    lv_obj_set_style_shadow_offset_y(card, 6, 0);

    if (info.active) {
      lv_obj_set_style_border_color(card,
                                    lv_color_hex(kOnxUiLayout ? 0x4ADE80 : 0x00CC66), 0);
      lv_obj_set_style_border_width(card, 2, 0);
      lv_obj_set_style_border_opa(card, LV_OPA_COVER, 0);
    } else {
      // Subtle border to define card edges on dark background
      lv_obj_set_style_border_color(card,
                                    lv_color_hex(kOnxUiLayout ? kOnxColorLine : 0x303030), 0);
      lv_obj_set_style_border_width(card, 1, 0);
      lv_obj_set_style_border_opa(card, kOnxUiLayout ? LV_OPA_COVER : LV_OPA_60, 0);
    }

    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    enable_touch_bubble(card);
    lv_obj_add_event_cb(card, &Ui::printer_card_click_cb, LV_EVENT_CLICKED, this);

    if (kOnxLandscapeLayout) {
      lv_obj_t* stripe = lv_obj_create(card);
      lv_obj_set_pos(stripe, 0, 0);
      lv_obj_set_size(stripe, 5, kOnxPrinterCardH);
      lv_obj_set_style_bg_color(stripe, lv_color_hex(0x4ADE80), 0);
      lv_obj_set_style_bg_opa(stripe, info.active ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
      lv_obj_set_style_border_width(stripe, 0, 0);
      lv_obj_set_style_radius(stripe, 0, 0);
      lv_obj_clear_flag(stripe, LV_OBJ_FLAG_SCROLLABLE);
      lv_obj_clear_flag(stripe, LV_OBJ_FLAG_CLICKABLE);
    }

    // Status dot — small colored circle in top-right
    lv_obj_t* dot = lv_obj_create(card);
    lv_obj_set_size(dot, kOnxLandscapeLayout ? 8 : 10, kOnxLandscapeLayout ? 8 : 10);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(dot, info.connected
                                   ? lv_color_hex(kOnxUiLayout ? 0x4ADE80 : 0x00CC66)
                                   : lv_color_hex(0x666666), 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_align(dot, LV_ALIGN_TOP_RIGHT, kOnxLandscapeLayout ? -10 : 0,
                 kOnxLandscapeLayout ? 68 : 0);
    lv_obj_clear_flag(dot, LV_OBJ_FLAG_CLICKABLE);
    if (info.connected) {
      start_dot_pulse(dot);
    }

    // Printer name (bold, larger)
    lv_obj_t* name_lbl = lv_label_create(card);
    const std::string display_name = kOnxLandscapeLayout
        ? printer_primary_title_for_display(info.name, info.model)
        : (info.name.empty() ? info.model : info.name);
    set_label_text_if_changed(name_lbl, display_name);
    lv_obj_set_width(name_lbl, kOnxLandscapeLayout ? 208 : (kOnxUiLayout ? 250 : 300));
    lv_label_set_long_mode(name_lbl, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(name_lbl, font_name, 0);
    if (kOnxLandscapeLayout) {
      set_dynamic_text_font(name_lbl);
    }
    lv_obj_set_style_text_color(name_lbl,
                                lv_color_hex(kOnxUiLayout ? kOnxColorText : 0xFFFFFF), 0);
    if (kOnxLandscapeLayout) {
      lv_obj_set_pos(name_lbl, 12, 10);
      lv_obj_set_size(name_lbl, 208, 24);
    } else {
      lv_obj_align(name_lbl, LV_ALIGN_TOP_LEFT, 0, 0);
    }

    if (kOnxLandscapeLayout) {
      lv_obj_t* pill = lv_label_create(card);
      char pill_text[12] = {};
      if (info.active) {
        std::snprintf(pill_text, sizeof(pill_text), "act");
      } else {
        std::snprintf(pill_text, sizeof(pill_text), "%u", static_cast<unsigned>(info.index + 1));
      }
      set_label_text_if_changed(pill, pill_text);
      lv_obj_set_size(pill, 38, 22);
      lv_label_set_long_mode(pill, LV_LABEL_LONG_DOT);
      lv_obj_set_style_text_align(pill, LV_TEXT_ALIGN_CENTER, 0);
      lv_obj_set_style_text_font(pill, &lv_font_montserrat_14, 0);
      lv_obj_set_style_text_color(pill, lv_color_hex(kOnxColorSoft), 0);
      lv_obj_set_style_bg_color(pill, lv_color_hex(kOnxColorPanel), 0);
      lv_obj_set_style_bg_opa(pill, LV_OPA_COVER, 0);
      lv_obj_set_style_radius(pill, 11, 0);
      lv_obj_set_style_pad_top(pill, 2, 0);
      lv_obj_align(pill, LV_ALIGN_TOP_RIGHT, -10, 10);
      lv_obj_clear_flag(pill, LV_OBJ_FLAG_CLICKABLE);
    }

    // Model
    lv_obj_t* model_lbl = lv_label_create(card);
    const std::string model_text = kOnxLandscapeLayout
        ? printer_secondary_for_display(info.name, info.model)
        : (info.model.empty() ? "Unknown" : info.model);
    set_label_text_if_changed(model_lbl, model_text);
    lv_obj_set_width(model_lbl, kOnxLandscapeLayout ? 252 : (kOnxUiLayout ? 260 : 300));
    lv_label_set_long_mode(model_lbl, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(model_lbl, font_detail, 0);
    lv_obj_set_style_text_color(model_lbl,
                                lv_color_hex(kOnxUiLayout ? kOnxColorMuted : 0x888888), 0);
    if (kOnxLandscapeLayout) {
      lv_obj_set_pos(model_lbl, 12, 40);
      lv_obj_set_size(model_lbl, 252, 18);
    } else {
      lv_obj_align_to(model_lbl, name_lbl, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 2);
    }

    // Host IP
    lv_obj_t* host_lbl = lv_label_create(card);
    std::string host_text = info.host.empty() ? "No local IP" : info.host;
    if (kOnxLandscapeLayout) {
      const char* state = info.connected ? "Online" : (info.active ? "Waiting" : "Profile");
      if (looks_like_serial(info.name)) {
        host_text = shortened_serial_for_display(info.name);
      }
      host_text = std::string(state) + " | " + host_text;
    }
    set_label_text_if_changed(host_lbl, host_text);
    lv_obj_set_width(host_lbl, kOnxLandscapeLayout ? 252 : (kOnxUiLayout ? 260 : 300));
    lv_label_set_long_mode(host_lbl, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(host_lbl, font_detail, 0);
    lv_obj_set_style_text_color(host_lbl,
                                lv_color_hex(kOnxLandscapeLayout && info.connected
                                                 ? 0x4ADE80
                                                 : (kOnxUiLayout ? kOnxColorMuted : 0x666666)),
                                0);
    if (kOnxLandscapeLayout) {
      lv_obj_set_pos(host_lbl, 12, 64);
      lv_obj_set_size(host_lbl, 252, 18);
    } else {
      lv_obj_align_to(host_lbl, model_lbl, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 2);
    }

    PrinterCardWidgets cw;
    cw.card = card;
    cw.name_label = name_lbl;
    cw.model_label = model_lbl;
    cw.host_label = host_lbl;
    cw.status_dot = dot;
    cw.profile_index = info.index;
    page0_cards_.push_back(cw);

    // Cascading reveal: start hidden below, fade + slide in with staggered delay
    lv_obj_set_style_opa(card, LV_OPA_TRANSP, 0);
    lv_obj_set_style_translate_y(card, kCardRevealYStart, 0);
    const uint32_t reveal_delay = static_cast<uint32_t>(card_idx) * kCardRevealStaggerMs;

    lv_anim_t ry;
    lv_anim_init(&ry);
    lv_anim_set_var(&ry, card);
    lv_anim_set_exec_cb(&ry, card_reveal_y_exec_cb);
    lv_anim_set_values(&ry, kCardRevealYStart, 0);
    lv_anim_set_duration(&ry, kCardRevealDurationMs);
    lv_anim_set_delay(&ry, reveal_delay);
    LV_ANIM_SET_EASE_OUT_BACK(&ry);
    lv_anim_set_path_cb(&ry, lv_anim_path_custom_bezier3);
    lv_anim_start(&ry);

    lv_anim_t ro;
    lv_anim_init(&ro);
    lv_anim_set_var(&ro, card);
    lv_anim_set_exec_cb(&ro, card_reveal_opa_exec_cb);
    lv_anim_set_values(&ro, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_duration(&ro, kCardRevealDurationMs);
    lv_anim_set_delay(&ro, reveal_delay);
    lv_anim_set_path_cb(&ro, lv_anim_path_ease_out);
    lv_anim_start(&ro);

    ++card_idx;
  }
}

void Ui::replay_card_animations_locked() {
  int card_idx = 0;
  for (const PrinterCardWidgets& cw : page0_cards_) {
    lv_obj_set_style_opa(cw.card, LV_OPA_TRANSP, 0);
    lv_obj_set_style_translate_y(cw.card, kCardRevealYStart, 0);
    const uint32_t reveal_delay = static_cast<uint32_t>(card_idx) * kCardRevealStaggerMs;

    lv_anim_t ry;
    lv_anim_init(&ry);
    lv_anim_set_var(&ry, cw.card);
    lv_anim_set_exec_cb(&ry, card_reveal_y_exec_cb);
    lv_anim_set_values(&ry, kCardRevealYStart, 0);
    lv_anim_set_duration(&ry, kCardRevealDurationMs);
    lv_anim_set_delay(&ry, reveal_delay);
    LV_ANIM_SET_EASE_OUT_BACK(&ry);
    lv_anim_set_path_cb(&ry, lv_anim_path_custom_bezier3);
    lv_anim_start(&ry);

    lv_anim_t ro;
    lv_anim_init(&ro);
    lv_anim_set_var(&ro, cw.card);
    lv_anim_set_exec_cb(&ro, card_reveal_opa_exec_cb);
    lv_anim_set_values(&ro, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_duration(&ro, kCardRevealDurationMs);
    lv_anim_set_delay(&ro, reveal_delay);
    lv_anim_set_path_cb(&ro, lv_anim_path_ease_out);
    lv_anim_start(&ro);

    ++card_idx;
  }
}

void Ui::apply_snapshot(const PrinterSnapshot& snapshot) {
  if (!initialized_) {
    return;
  }
  if (scrolling_) {
    return;
  }

  // Pre-decode preview PNG outside LVGL lock, but only when the preview page is
  // actually active. The decoded cover is ~1 MB, lives in PSRAM, and is kept
  // cached across page changes so navigating away doesn't create a decode storm.
  std::shared_ptr<std::vector<uint8_t>> pre_decoded_raw;
  lv_image_dsc_t pre_decoded_dsc{};
  const bool preview_page_active = !scrolling_ && active_page_ == kPageIdxPreview;
  const bool preview_blob_changed =
      snapshot.preview_blob && !snapshot.preview_blob->empty() &&
      last_preview_blob_.get() != snapshot.preview_blob.get();
  // Also pre-decode when the blob exists but hasn't been decoded yet (e.g.
  // blob arrived while another page was active, then user scrolled to preview).
  const bool needs_first_decode =
      !preview_blob_changed && snapshot.preview_blob &&
      !snapshot.preview_blob->empty() &&
      (!last_preview_raw_ || last_preview_raw_->empty());
  if (preview_page_active && (preview_blob_changed || needs_first_decode)) {
    decode_preview_png(snapshot.preview_blob, &pre_decoded_raw, &pre_decoded_dsc);
  }

  LvglLockGuard lock(500, "apply_snapshot");
  if (!lock.locked()) {
    return;
  }

  last_snapshot_ = snapshot;
  if (scrolling_) {
    deferred_snapshot_ = snapshot;
    deferred_snapshot_pending_ = true;
    return;
  }

  apply_snapshot_locked(snapshot, false,
                        std::move(pre_decoded_raw), &pre_decoded_dsc);
}

void Ui::apply_ring_visual_locked(const PrinterSnapshot& snapshot) {
  const int progress = std::clamp(static_cast<int>(snapshot.progress_percent + 0.5f), 0, 100);
  const RingVisual ring = lifecycle_ring_visual(snapshot, arc_colors_);
  const uint32_t text_hex = stable_status_text_hex(snapshot, arc_colors_);

  // --- Manage lv_anim transitions ---
  const auto anim_kind_u8 = static_cast<uint8_t>(ring.anim_kind);
  if (anim_kind_u8 != active_ring_anim_kind_) {
    stop_ring_animations_locked();
    active_ring_anim_kind_ = anim_kind_u8;

    switch (ring.anim_kind) {
      case RingAnimKind::kFilamentLoad:
      case RingAnimKind::kFilamentUnload: {
        const bool loading = (ring.anim_kind == RingAnimKind::kFilamentLoad);
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, status_arc_);
        lv_anim_set_exec_cb(&a, [](void* obj, int32_t v) {
          lv_arc_set_value(static_cast<lv_obj_t*>(obj), v);
        });
        lv_anim_set_values(&a, loading ? 0 : 100, loading ? 100 : 0);
        lv_anim_set_duration(&a, 2000);
        lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_repeat_delay(&a, 300);
        lv_anim_set_path_cb(&a, lv_anim_path_linear);
        lv_anim_start(&a);
        break;
      }
      case RingAnimKind::kPulseBoth:
      case RingAnimKind::kPulseIndicator: {
        const uint16_t depth = static_cast<uint16_t>(
            std::min<uint32_t>(kRingPulseDepthPercent, 100U) * 255U / 100U);
        const int32_t min_scale = static_cast<int32_t>(255 > depth ? 255 - depth : 0);
        pulse_base_hex_ = ring.pulse_base_hex;
        pulse_both_parts_ = (ring.anim_kind == RingAnimKind::kPulseBoth);
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, this);
        lv_anim_set_exec_cb(&a, pulse_anim_exec_cb);
        lv_anim_set_values(&a, min_scale, 255);
        lv_anim_set_duration(&a, ring.pulse_period_ms / 2);
        lv_anim_set_reverse_duration(&a, ring.pulse_period_ms / 2);
        lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_path_cb(&a, lv_anim_path_linear);
        lv_anim_start(&a);
        break;
      }
      case RingAnimKind::kNone:
        break;
    }
  } else if (ring.anim_kind == RingAnimKind::kPulseBoth ||
             ring.anim_kind == RingAnimKind::kPulseIndicator) {
    pulse_base_hex_ = ring.pulse_base_hex;
    pulse_both_parts_ = (ring.anim_kind == RingAnimKind::kPulseBoth);
  }

  // --- Apply static properties ---
  if (ring.anim_kind != RingAnimKind::kFilamentLoad &&
      ring.anim_kind != RingAnimKind::kFilamentUnload) {
    const int displayed_value = (ring.value_override >= 0) ? ring.value_override : progress;
    if (lv_arc_get_value(status_arc_) != displayed_value) {
      lv_arc_set_value(status_arc_, displayed_value);
    }
  }

  if (ring.anim_kind != RingAnimKind::kPulseBoth &&
      ring.anim_kind != RingAnimKind::kPulseIndicator) {
    if (last_ring_main_hex_ != ring.main_hex) {
      lv_obj_set_style_arc_color(status_arc_, lv_color_hex(ring.main_hex), LV_PART_MAIN);
      last_ring_main_hex_ = ring.main_hex;
    }
    if (last_ring_indicator_hex_ != ring.indicator_hex) {
      lv_obj_set_style_arc_color(status_arc_, lv_color_hex(ring.indicator_hex), LV_PART_INDICATOR);
      if (onx_progress_bar_ != nullptr) {
        lv_obj_set_style_bg_color(onx_progress_bar_, lv_color_hex(ring.indicator_hex),
                                  LV_PART_INDICATOR);
      }
      last_ring_indicator_hex_ = ring.indicator_hex;
    }
  }

  if (last_ring_text_hex_ != text_hex) {
    const lv_color_t text_color = lv_color_hex(text_hex);
    lv_obj_set_style_text_color(progress_label_, text_color, 0);
    lv_obj_set_style_text_color(status_label_, text_color, 0);
    if (onx_top_status_label_ != nullptr) {
      lv_obj_set_style_text_color(onx_top_status_label_, text_color, 0);
    }
    // Camera page mirrors the main-page status line: same font/size and
    // the same lifecycle-driven color (green while printing, red on
    // failure, etc.) instead of a fixed white.
    if (page3_note_ != nullptr) {
      lv_obj_set_style_text_color(page3_note_, text_color, 0);
    }
    last_ring_text_hex_ = text_hex;
  }
}

bool Ui::ensure_preview_image_loaded_locked(
    bool force_reload,
    std::shared_ptr<std::vector<uint8_t>> pre_decoded_raw,
    const lv_image_dsc_t* pre_decoded_dsc) {
  if (force_reload) {
    release_preview_image_locked();
  }

  if (last_preview_raw_ && !last_preview_raw_->empty()) {
    lv_image_set_src(page2_image_, &preview_image_dsc_);
    return true;
  }

  // Use pre-decoded data when available (decoded outside LVGL lock).
  if (pre_decoded_raw && !pre_decoded_raw->empty() && pre_decoded_dsc != nullptr) {
    last_preview_raw_ = std::move(pre_decoded_raw);
    preview_image_dsc_ = *pre_decoded_dsc;
    lv_image_set_src(page2_image_, &preview_image_dsc_);
    log_blob_diag("ui preview set_src raw", last_preview_raw_);
    return true;
  }

  // No fallback decode under lock — pre-decode happens outside the LVGL
  // lock in apply_snapshot().  If we reach here without decoded data the
  // image will appear on the next snapshot tick (~500 ms).
  return false;
}

void Ui::release_preview_image_locked() {
  if (page2_image_ != nullptr) {
    lv_image_set_src(page2_image_, nullptr);
  }
  lv_image_cache_drop(&preview_image_dsc_);
  last_preview_raw_.reset();
  std::memset(&preview_image_dsc_, 0, sizeof(preview_image_dsc_));
}

void Ui::apply_snapshot_locked(const PrinterSnapshot& snapshot, bool force_ring_refresh,
                               std::shared_ptr<std::vector<uint8_t>> pre_decoded_raw,
                               const lv_image_dsc_t* pre_decoded_dsc) {
  deferred_snapshot_pending_ = false;
  update_page_availability_locked(snapshot);

  bool wake_due_to_state_change = false;
  if (!snapshot.ui_status.empty() && snapshot.ui_status != last_ui_status_) {
    wake_due_to_state_change = true;
    last_ui_status_ = snapshot.ui_status;
  }
  if (snapshot.print_active != last_print_active_) {
    wake_due_to_state_change = true;
    last_print_active_ = snapshot.print_active;
  }
  if (wake_due_to_state_change) {
    note_activity(true);
  }

  const int progress = std::clamp(static_cast<int>(snapshot.progress_percent + 0.5f), 0, 100);

  // Always apply ring visual — it manages lv_anim transitions internally
  // and only restarts animations when the kind actually changes.
  apply_ring_visual_locked(snapshot);

  char progress_buffer[8] = {};
  std::snprintf(progress_buffer, sizeof(progress_buffer), "%d%%", progress);
  set_label_text_if_changed(progress_label_, progress_buffer);
  const std::string status_text = lifecycle_label(snapshot);
  set_label_text_if_changed(status_label_, status_text);
  if (onx_progress_bar_ != nullptr) {
    lv_bar_set_value(onx_progress_bar_, progress, LV_ANIM_OFF);
  }
  if (onx_top_status_label_ != nullptr) {
    char top_status[64] = {};
    std::snprintf(top_status, sizeof(top_status), "%d%% %s", progress, status_text.c_str());
    set_label_text_if_changed(onx_top_status_label_, top_status);
  }
  if (onx_job_label_ != nullptr) {
    const std::string job_label =
        !snapshot.job_name.empty() ? snapshot.job_name :
        (!snapshot.gcode_file.empty() ? snapshot.gcode_file : "No active job");
    set_label_text_if_changed(onx_job_label_,
                              kOnxLandscapeLayout
                                  ? display_text_for_label(job_label, "Project name")
                                  : job_label);
  }

  const std::string raw_detail = detail_text(snapshot);
  const std::string detail = raw_detail.empty() || !kOnxLandscapeLayout
      ? raw_detail
      : display_text_for_label(raw_detail, "Project name");

  // [DIAG] Log what the display is actually showing — on change only.
  if (status_text != last_diag_status_ || detail != last_diag_detail_ ||
      snapshot.stage != last_diag_stage_ || progress != last_diag_progress_ ||
      snapshot.current_layer != last_diag_current_layer_ ||
      snapshot.total_layers != last_diag_total_layers_) {
    last_diag_status_ = status_text;
    last_diag_detail_ = detail;
    last_diag_stage_ = snapshot.stage;
    last_diag_progress_ = progress;
    last_diag_current_layer_ = snapshot.current_layer;
    last_diag_total_layers_ = snapshot.total_layers;
    ESP_LOGI(kTag,
             "[DIAG] display: status=%s stage=%s progress=%d layer=%u/%u "
             "detail=%.60s lifecycle=%s",
             status_text.c_str(), snapshot.stage.c_str(),
             progress, static_cast<unsigned>(snapshot.current_layer),
             static_cast<unsigned>(snapshot.total_layers),
             detail.empty() ? "(-)" : detail.c_str(),
             to_string(snapshot.lifecycle));
  }
  detail_visible_ = !detail.empty();
  if (detail_visible_) {
    const lv_label_long_mode_t desired_mode =
        kOnxLandscapeLayout ? LV_LABEL_LONG_WRAP :
        kOnxUiLayout ? LV_LABEL_LONG_DOT :
        (snapshot.has_error ? LV_LABEL_LONG_SCROLL_CIRCULAR : LV_LABEL_LONG_WRAP);
    if (lv_label_get_long_mode(detail_label_) != desired_mode) {
      lv_label_set_long_mode(detail_label_, desired_mode);
    }
    if (kOnxLandscapeLayout) {
      lv_obj_set_style_text_color(detail_label_,
                                  lv_color_hex(snapshot.has_error ? 0xEF4444 : kOnxColorSoft), 0);
      lv_obj_set_style_border_color(detail_label_,
                                    lv_color_hex(snapshot.has_error ? 0xEF4444 : kOnxColorLine), 0);
      lv_obj_set_style_bg_color(detail_label_,
                                lv_color_hex(snapshot.has_error ? 0x1A0B0B : kOnxColorPanel), 0);
    }
    set_label_text_if_changed(detail_label_, detail);
  }

  const std::string layer = layer_text(snapshot);
  set_label_text_if_changed(layer_label_,
                            kOnxLandscapeLayout ? compact_layer_text(snapshot) : layer);
  if (onx_layer_line_label_ != nullptr) {
    set_label_text_if_changed(onx_layer_line_label_, layer);
  }
  if (onx_metric_detail_label_ != nullptr) {
    const std::string metric_detail =
        snapshot.has_error ? "Error" :
        (!snapshot.stage.empty() ? snapshot.stage :
         (!detail.empty() ? detail : status_text));
    set_label_text_if_changed(onx_metric_detail_label_,
                              kOnxLandscapeLayout
                                  ? display_text_for_label(metric_detail, "Project name")
                                  : metric_detail);
  }

  const bool use_eta = !kOnxLandscapeLayout && show_eta_;
  const std::string remaining = use_eta ? eta_text(snapshot) : remaining_text(snapshot);
  // Landscape debug builds avoid wall-clock ETA because local time/SNTP can be
  // misleading on device; only the remaining duration gets the "Rem" prefix.
  const bool show_remaining_prefix = kOnxLandscapeLayout
      ? is_remaining_duration_value(remaining)
      : !use_eta;
  if (kOnxLandscapeLayout) {
    std::string remaining_display = remaining;
    if (is_remaining_duration_value(remaining)) {
      remaining_display = "Rem " + remaining;
    }
    set_label_text_if_changed(remaining_label_, remaining_display);
    lv_obj_set_pos(remaining_label_, 8, 10);
    lv_obj_set_size(remaining_label_, 156, 36);
    lv_obj_set_style_text_align(remaining_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(remaining_prefix_label_, 14, 17);
    lv_obj_set_size(remaining_prefix_label_, 42, 18);
  } else {
    set_label_text_if_changed(remaining_label_, remaining);
  }
  set_hidden(remaining_prefix_label_, kOnxLandscapeLayout || !show_remaining_prefix);

  char temp_buffer[24] = {};
  const bool is_dual_nozzle = snapshot.active_nozzle_index >= 0;
  const char* active_prefix = is_dual_nozzle ? (snapshot.active_nozzle_index == 1 ? "L " : "R ") : "";
  const char* secondary_prefix = is_dual_nozzle ? (snapshot.active_nozzle_index == 1 ? "R " : "L ") : "";
  if (snapshot.nozzle_temp_known || snapshot.nozzle_temp_c > 0.0f) {
    std::snprintf(temp_buffer, sizeof(temp_buffer), "%s%.0f%s", active_prefix, snapshot.nozzle_temp_c, kDegreeC);
  } else {
    std::snprintf(temp_buffer, sizeof(temp_buffer), "%s--%s", active_prefix, kDegreeC);
  }
  set_label_text_if_changed(nozzle_value_label_, temp_buffer);

  if (snapshot.bed_temp_known || snapshot.bed_temp_c > 0.0f) {
    std::snprintf(temp_buffer, sizeof(temp_buffer), "%.0f%s", snapshot.bed_temp_c, kDegreeC);
  } else {
    std::snprintf(temp_buffer, sizeof(temp_buffer), "--%s", kDegreeC);
  }
  set_label_text_if_changed(bed_value_label_, temp_buffer);

  char nozzle_aux_buf[40] = {};
  if (is_dual_nozzle &&
      (snapshot.secondary_nozzle_temp_known || snapshot.secondary_nozzle_temp_c > 0.0f)) {
    std::snprintf(nozzle_aux_buf, sizeof(nozzle_aux_buf), "%s%.0f%s",
                  secondary_prefix, snapshot.secondary_nozzle_temp_c, kDegreeC);
  } else if (!is_dual_nozzle) {
    const std::string tmp =
        optional_temperature_text("Other nozzle", snapshot.secondary_nozzle_temp_c,
                                  snapshot.secondary_nozzle_temp_known);
    std::snprintf(nozzle_aux_buf, sizeof(nozzle_aux_buf), "%s", tmp.c_str());
  }
  const std::string nozzle_aux(nozzle_aux_buf);
  nozzle_aux_visible_ = !nozzle_aux.empty();
  if (nozzle_aux_visible_) {
    set_label_text_if_changed(nozzle_aux_label_, nozzle_aux);
  } else if (kOnxLandscapeLayout) {
    set_label_text_if_changed(nozzle_aux_label_, "--");
  }

  const std::string bed_aux =
      optional_temperature_text("Chamber", snapshot.chamber_temp_c, snapshot.chamber_temp_known);
  bed_aux_visible_ = !bed_aux.empty();
  if (bed_aux_visible_) {
    set_label_text_if_changed(bed_aux_label_, bed_aux);
  } else if (kOnxLandscapeLayout) {
    set_label_text_if_changed(bed_aux_label_, "--");
  }

  const std::string battery_icon = battery_icon_text(snapshot);
  const std::string battery_pct = battery_pct_text(snapshot);
  const bool show_battery = snapshot.battery_present;
  set_label_text_if_changed(battery_icon_label_, show_battery ? battery_icon : "");
  set_label_text_if_changed(battery_pct_label_, show_battery ? battery_pct : "");

  // --- AMS page rendering ---
  const uint8_t ams_count = snapshot.ams ? snapshot.ams->count : 0;
  const uint32_t ams_signature = ams_ui_signature(snapshot);
  const bool ams_visible = scrolling_ ||
      (active_page_ >= kPageIdxAmsFirst && active_page_ <= kPageIdxAmsLast);
  const bool render_ams =
      ams_visible && ams_signature != last_rendered_ams_signature_;
  if (ams_count > 0 && render_ams) {
    last_rendered_ams_signature_ = ams_signature;
    compute_ams_tray_errors(snapshot);
    const bool show_unit_labels = ams_count > 1;
    for (int u = 0; u < kMaxAmsUnits; ++u) {
      if (u < ams_count) {
        render_ams_unit(u, snapshot, show_unit_labels);
      }
    }
  } else if (render_ams) {
    last_rendered_ams_signature_ = ams_signature;
    // No AMS — show "No AMS connected" note on the first AMS page only.
    for (int u = 0; u < kMaxAmsUnits; ++u) {
      if (ams_tray_row_[u] != nullptr) {
        set_hidden(ams_tray_row_[u], true);
      }
      if (ams_note_[u] != nullptr) {
        set_hidden(ams_note_[u], u != 0);
      }
      if (kOnxUiLayout) {
        set_hidden(ams_meta_row_[u], true);
        set_hidden(ams_humidity_label_[u], true);
        set_hidden(ams_temp_label_[u], true);
      }
      for (int s = 0; s < kMaxAmsTrays; ++s) {
        ams_tray_error_[u][s] = false;
      }
    }
    if (ams_ext_col_ != nullptr) {
      set_hidden(ams_ext_col_, true);
    }
    if (ams_ext_status_ != nullptr) {
      set_hidden(ams_ext_status_, true);
    }
    apply_ams_error_pulse_locked();
  }

  const std::string preview_note = preview_note_text(snapshot);
  const std::string preview_subnote = preview_subnote_text(snapshot);
  const std::string camera_note = camera_note_text(snapshot);
  const std::string camera_subnote = camera_subnote_text(snapshot);
  bool has_preview_image = false;
  if (snapshot.preview_blob && !snapshot.preview_blob->empty()) {
    const bool preview_blob_changed = last_preview_blob_.get() != snapshot.preview_blob.get();
    last_preview_blob_ = snapshot.preview_blob;
    if (active_page_ == kPageIdxPreview) {
      has_preview_image = ensure_preview_image_loaded_locked(
          preview_blob_changed, std::move(pre_decoded_raw), pre_decoded_dsc);
    } else if (preview_blob_changed) {
      release_preview_image_locked();
    }
  } else {
    if (last_preview_blob_ || last_preview_raw_) {
      release_preview_image_locked();
    }
    last_preview_blob_.reset();
  }
  const bool has_page2_image = has_preview_image;
  preview_image_visible_ = has_page2_image;

  if (preview_text_image_mode_ != has_page2_image) {
    if (kOnxUiLayout) {
      lv_obj_set_pos(page2_image_, 12, kOnxLandscapeLayout ? 48 : 60);
      if (kOnxLandscapeLayout) {
        lv_obj_set_pos(page2_note_, 24, 150);
        lv_obj_set_size(page2_note_, 216, 48);
        lv_obj_set_pos(page2_subnote_, 276, 60);
        lv_obj_set_size(page2_subnote_, 180, 72);
        if (page2_detail_label_ != nullptr) {
          lv_obj_set_pos(page2_detail_label_, 276, 142);
          lv_obj_set_size(page2_detail_label_, 180, 96);
        }
      } else {
        lv_obj_set_pos(page2_note_, 30, 194);
        lv_obj_set_pos(page2_subnote_, 12, 372);
        lv_obj_set_size(page2_subnote_, 296, 48);
      }
    } else if (has_page2_image) {
      // Note is hidden when cover is loaded; subnote (title) moves to former note position.
      lv_obj_align(page2_subnote_, LV_ALIGN_CENTER, 0, kPage2NoteWithImageY);
    } else {
      lv_obj_align(page2_note_, LV_ALIGN_CENTER, 0, -14);
      lv_obj_align(page2_subnote_, LV_ALIGN_CENTER, 0, 18);
    }
    preview_text_image_mode_ = has_page2_image;
  }
  set_hidden(page2_note_, preview_note.empty());
  if (!preview_note.empty()) {
    set_label_text_if_changed(page2_note_, preview_note);
  }
  set_hidden(page2_subnote_, preview_subnote.empty());
  if (!preview_subnote.empty()) {
    set_label_text_if_changed(page2_subnote_,
                              kOnxLandscapeLayout
                                  ? display_text_for_label(preview_subnote, "Project name")
                                  : preview_subnote);
  }
  if (kOnxUiLayout && page2_detail_label_ != nullptr) {
    std::string preview_detail;
    if (kOnxLandscapeLayout) {
      const std::string source =
          snapshot.preview_source == FieldSource::kNone
              ? std::string{}
              : ("Source: " + std::string(to_string(snapshot.preview_source)));
      const std::string short_detail =
          !snapshot.cloud_detail.empty() ? snapshot.cloud_detail : snapshot.preview_hint;
      if (has_page2_image) {
        preview_detail = source;
        if (!short_detail.empty()) {
          preview_detail = preview_detail.empty()
              ? display_text_for_label(short_detail, "Project name")
              : (preview_detail + "\n" + display_text_for_label(short_detail, "Project name"));
        }
      } else if (!snapshot.preview_url.empty()) {
        preview_detail = !source.empty() ? source : "Source: cloud";
        if (!short_detail.empty() && short_detail != "Loading cloud cover") {
          preview_detail += "\n" + display_text_for_label(short_detail, "Project name");
        }
      } else {
        preview_detail = !short_detail.empty()
            ? display_text_for_label(short_detail, "Project name")
            : (!preview_note.empty() ? preview_note : std::string{});
      }
    } else {
      preview_detail =
          !snapshot.cloud_detail.empty() ? snapshot.cloud_detail :
          (!snapshot.preview_hint.empty() ? snapshot.preview_hint :
           (!has_page2_image ? preview_note : std::string{}));
    }
    set_hidden(page2_detail_label_, preview_detail.empty());
    if (!preview_detail.empty()) {
      set_label_text_if_changed(page2_detail_label_, preview_detail);
    }
  }

  bool has_camera_image =
      camera_slot_initialized_ && camera_blobs_[active_camera_slot_] &&
      !camera_blobs_[active_camera_slot_]->empty();
  if (snapshot.camera_blob && !snapshot.camera_blob->empty() && snapshot.camera_width > 0U &&
      snapshot.camera_height > 0U) {
    const bool camera_blob_changed =
        !camera_slot_initialized_ || camera_blobs_[active_camera_slot_].get() != snapshot.camera_blob.get();
    if (camera_blob_changed ||
        last_camera_width_ != snapshot.camera_width || last_camera_height_ != snapshot.camera_height) {
      const uint8_t next_slot =
          camera_slot_initialized_ ? static_cast<uint8_t>((active_camera_slot_ + 1U) % 2U) : 0U;
      log_blob_diag("ui camera incoming rgb565", snapshot.camera_blob);
      lv_image_cache_drop(&camera_image_dscs_[next_slot]);
      log_heap_diag("ui camera after cache drop");
      lv_image_dsc_t next_dsc = {};
      if (configure_camera_rgb565(snapshot.camera_blob, snapshot.camera_width, snapshot.camera_height,
                                  &next_dsc)) {
        camera_blobs_[next_slot] = snapshot.camera_blob;
        camera_image_dscs_[next_slot] = next_dsc;
        active_camera_slot_ = next_slot;
        camera_slot_initialized_ = true;
        last_camera_width_ = snapshot.camera_width;
        last_camera_height_ = snapshot.camera_height;
        lv_image_set_src(page3_image_, &camera_image_dscs_[active_camera_slot_]);
        log_heap_diag("ui camera after lv_image_set_src");
      } else {
        camera_blobs_[next_slot].reset();
        std::memset(&camera_image_dscs_[next_slot], 0, sizeof(camera_image_dscs_[next_slot]));
        if (!camera_slot_initialized_) {
          active_camera_slot_ = 0;
        }
        last_camera_width_ = 0;
        last_camera_height_ = 0;
      }
    }
    has_camera_image =
        camera_slot_initialized_ && camera_blobs_[active_camera_slot_] &&
        !camera_blobs_[active_camera_slot_]->empty();
  } else {
    if (camera_slot_initialized_ && page3_image_ != nullptr) {
      lv_image_set_src(page3_image_, nullptr);
    }
    lv_image_cache_drop(&camera_image_dscs_[0]);
    lv_image_cache_drop(&camera_image_dscs_[1]);
    camera_blobs_[0].reset();
    camera_blobs_[1].reset();
    camera_slot_initialized_ = false;
    active_camera_slot_ = 0;
    last_camera_width_ = 0;
    last_camera_height_ = 0;
    std::memset(&camera_image_dscs_[0], 0, sizeof(camera_image_dscs_[0]));
    std::memset(&camera_image_dscs_[1], 0, sizeof(camera_image_dscs_[1]));
    has_camera_image = false;
  }
  camera_image_visible_ = has_camera_image;

  if (camera_text_image_mode_ != has_camera_image) {
    if (kOnxUiLayout) {
      if (has_camera_image) {
        lv_obj_set_pos(page3_note_, kOnxLandscapeLayout ? 336 : 12,
                       kOnxLandscapeLayout ? 60 : 48);
        lv_obj_set_size(page3_note_, kOnxLandscapeLayout ? 120 : 180,
                        kOnxLandscapeLayout ? 24 : 30);
        lv_obj_set_width(page3_note_, kOnxLandscapeLayout ? 120 : 180);
        lv_label_set_long_mode(page3_note_, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_align(page3_note_, LV_TEXT_ALIGN_LEFT, 0);
        lv_obj_set_style_bg_color(page3_note_, lv_color_hex(kOnxColorPanel), 0);
        lv_obj_set_style_bg_opa(page3_note_, kOnxLandscapeLayout ? LV_OPA_TRANSP : LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(page3_note_, lv_color_hex(kOnxColorLine), 0);
        lv_obj_set_style_border_opa(page3_note_, kOnxLandscapeLayout ? LV_OPA_TRANSP : LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(page3_note_, kOnxLandscapeLayout ? 0 : 1, 0);
        lv_obj_set_style_radius(page3_note_, kOnxLandscapeLayout ? 0 : 12, 0);
        lv_obj_set_style_pad_hor(page3_note_, kOnxLandscapeLayout ? 0 : 10, 0);
        lv_obj_set_style_pad_ver(page3_note_, kOnxLandscapeLayout ? 0 : 2, 0);
        if (kOnxLandscapeLayout) {
          lv_obj_set_style_text_color(page3_note_, lv_color_hex(kOnxColorText), 0);
        }
      } else {
        lv_obj_set_pos(page3_note_, kOnxLandscapeLayout ? 34 : 30,
                       kOnxLandscapeLayout ? 136 : 154);
        lv_obj_set_size(page3_note_, kOnxLandscapeLayout ? 256 : 260, 48);
        lv_obj_set_width(page3_note_, kOnxLandscapeLayout ? 256 : 260);
        lv_label_set_long_mode(page3_note_, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_align(page3_note_, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_bg_opa(page3_note_, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_opa(page3_note_, LV_OPA_TRANSP, 0);
        lv_obj_set_style_pad_hor(page3_note_, 0, 0);
        lv_obj_set_style_pad_ver(page3_note_, 0, 0);
        lv_obj_set_style_text_color(page3_note_, lv_color_hex(kOnxColorSoft), 0);
      }
      lv_obj_set_pos(page3_subnote_, kOnxLandscapeLayout ? 12 : 0,
                     kOnxLandscapeLayout ? 90 : 0);
      lv_obj_set_size(page3_subnote_, kOnxLandscapeLayout ? 120 : 276,
                      kOnxLandscapeLayout ? 24 : 36);
      lv_obj_set_width(page3_subnote_, kOnxLandscapeLayout ? 120 : 276);
      lv_label_set_long_mode(page3_subnote_,
                             kOnxLandscapeLayout ? LV_LABEL_LONG_DOT : LV_LABEL_LONG_WRAP);
    } else if (has_camera_image) {
      // Status note moves above the image; layer/subnote sits below.
      lv_obj_align(page3_note_, LV_ALIGN_CENTER, 0, kPage3StatusAboveImageY);
      lv_obj_align(page3_subnote_, LV_ALIGN_CENTER, 0, kPage3NoteWithImageY);
    } else {
      lv_obj_align(page3_note_, LV_ALIGN_CENTER, 0, 0);
      lv_obj_align(page3_subnote_, LV_ALIGN_CENTER, 0, 28);
    }
    camera_text_image_mode_ = has_camera_image;
  }
  set_hidden(page3_note_, camera_note.empty());
  if (!camera_note.empty()) {
    set_label_text_if_changed(page3_note_, camera_note);
    if (kOnxUiLayout && !has_camera_image) {
      lv_obj_set_style_text_color(page3_note_, lv_color_hex(kOnxColorSoft), 0);
    }
  }
  const bool show_camera_subnote =
      !camera_subnote.empty() &&
      !(kOnxLandscapeLayout && !has_camera_image && camera_subnote == "Auto-refresh every 2s");
  set_hidden(page3_subnote_, !show_camera_subnote);
  if (show_camera_subnote) {
    // Use the same large font as the main-page layer label when showing
    // "Layer: X / Y", fall back to the regular small font otherwise.
    const bool subnote_is_layer =
        has_camera_image && (snapshot.total_layers > 0 || snapshot.current_layer > 0);
    lv_obj_set_style_text_font(page3_subnote_,
                               (!kOnxUiLayout && subnote_is_layer) ? &dosis_32 :
                               (kOnxLandscapeLayout ? &lv_font_montserrat_14 :
                                &lv_font_montserrat_20), 0);
    set_label_text_if_changed(page3_subnote_, camera_subnote);
  }
  if (kOnxLandscapeLayout && page3_source_label_ != nullptr) {
    std::string source_text = "Source: ";
    source_text += snapshot.camera_source == FieldSource::kNone ? "waiting" : to_string(snapshot.camera_source);
    set_label_text_if_changed(page3_source_label_, source_text);
  }
  if (kOnxLandscapeLayout && page3_refresh_label_ != nullptr) {
    set_label_text_if_changed(page3_refresh_label_, "Auto 2s");
  }

  show_logo_ = should_show_logo(snapshot);
  const bool chamber_light_clickable = snapshot.chamber_light_supported;
  if (logo_clickable_ != chamber_light_clickable) {
    set_clickable(logo_badge_, chamber_light_clickable);
    logo_clickable_ = chamber_light_clickable;
  }

  const bool logo_recolor_enabled =
      snapshot.chamber_light_supported && snapshot.chamber_light_state_known &&
      !snapshot.chamber_light_on;
  const uint32_t logo_recolor_hex = logo_recolor_enabled ? 0x7A7A7A : 0U;
  if (logo_recolor_enabled != logo_recolor_enabled_ ||
      (logo_recolor_enabled && logo_recolor_hex != logo_recolor_hex_)) {
    if (logo_recolor_enabled) {
      lv_obj_set_style_image_recolor(logo_image_, lv_color_hex(logo_recolor_hex), 0);
      lv_obj_set_style_image_recolor_opa(logo_image_, LV_OPA_COVER, 0);
    } else {
      lv_obj_set_style_image_recolor_opa(logo_image_, LV_OPA_TRANSP, 0);
    }
    logo_recolor_enabled_ = logo_recolor_enabled;
    logo_recolor_hex_ = logo_recolor_hex;
  }
  compute_portal_texts_locked();
  apply_page_visibility();
}

void Ui::stop_ring_animations_locked() {
  lv_anim_delete(status_arc_, nullptr);
  lv_anim_delete(this, pulse_anim_exec_cb);
  active_ring_anim_kind_ = static_cast<uint8_t>(RingAnimKind::kNone);
}

void Ui::pulse_anim_exec_cb(void* var, int32_t scale) {
  auto* ui = static_cast<Ui*>(var);
  if (ui == nullptr || ui->status_arc_ == nullptr) return;
  const uint32_t hex = scale_color(ui->pulse_base_hex_, static_cast<uint16_t>(scale));
  if (ui->pulse_both_parts_) {
    if (ui->last_ring_main_hex_ != hex) {
      lv_obj_set_style_arc_color(ui->status_arc_, lv_color_hex(hex), LV_PART_MAIN);
      ui->last_ring_main_hex_ = hex;
    }
  }
  if (ui->last_ring_indicator_hex_ != hex) {
    lv_obj_set_style_arc_color(ui->status_arc_, lv_color_hex(hex), LV_PART_INDICATOR);
    ui->last_ring_indicator_hex_ = hex;
  }
}

// Ambient sweep timer (ring_timer_cb, handle_ring_timer) removed — the rotating
// arc during printing caused excessive LVGL redraws on single-buffered display.

// --- Multi-AMS helpers --------------------------------------------------------

void Ui::build_ams_page(int unit_idx) {
  if (unit_idx < 0 || unit_idx >= kMaxAmsUnits) return;
  lv_obj_t* page = ams_pages_[unit_idx];
  if (page == nullptr) return;

  // Unit header label ("AMS 1..4") — created always, hidden by default.
  ams_unit_label_[unit_idx] = lv_label_create(page);
  char unit_label_buf[16];
  std::snprintf(unit_label_buf, sizeof(unit_label_buf), "AMS %d", unit_idx + 1);
  set_label_text_if_changed(ams_unit_label_[unit_idx], unit_label_buf);
  lv_obj_set_style_text_font(ams_unit_label_[unit_idx], &dosis_32, 0);
  lv_obj_set_style_text_color(ams_unit_label_[unit_idx], lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_align(ams_unit_label_[unit_idx], LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(ams_unit_label_[unit_idx], LV_ALIGN_TOP_MID, 0, 60);
  lv_obj_add_flag(ams_unit_label_[unit_idx], LV_OBJ_FLAG_HIDDEN);

  // Gray shelf background (behind upper half of pills)
  ams_shelf_[unit_idx] = lv_obj_create(page);
  lv_obj_set_size(ams_shelf_[unit_idx], 359, 110);
  lv_obj_set_style_radius(ams_shelf_[unit_idx], 20, 0);
  lv_obj_set_style_bg_color(ams_shelf_[unit_idx], lv_color_hex(0x565656), 0);
  lv_obj_set_style_bg_opa(ams_shelf_[unit_idx], LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(ams_shelf_[unit_idx], 2, 0);
  lv_obj_set_style_border_color(ams_shelf_[unit_idx], lv_color_hex(0x888888), 0);
  lv_obj_set_style_border_opa(ams_shelf_[unit_idx], LV_OPA_COVER, 0);
  lv_obj_set_style_border_side(ams_shelf_[unit_idx],
      static_cast<lv_border_side_t>(LV_BORDER_SIDE_TOP | LV_BORDER_SIDE_LEFT | LV_BORDER_SIDE_RIGHT), 0);
  lv_obj_align(ams_shelf_[unit_idx], LV_ALIGN_CENTER, 0, -50);
  lv_obj_clear_flag(ams_shelf_[unit_idx], LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(ams_shelf_[unit_idx], LV_OBJ_FLAG_CLICKABLE);
  enable_touch_bubble(ams_shelf_[unit_idx]);

  // Dark base behind lower half of pills
  ams_base_[unit_idx] = lv_obj_create(page);
  lv_obj_set_size(ams_base_[unit_idx], 385, 103);
  lv_obj_set_style_radius(ams_base_[unit_idx], 0, 0);
  lv_obj_set_style_bg_color(ams_base_[unit_idx], lv_color_hex(0x1F1F1F), 0);
  lv_obj_set_style_bg_opa(ams_base_[unit_idx], LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(ams_base_[unit_idx], 2, 0);
  lv_obj_set_style_border_color(ams_base_[unit_idx], lv_color_hex(0x888888), 0);
  lv_obj_set_style_border_opa(ams_base_[unit_idx], LV_OPA_COVER, 0);
  lv_obj_set_style_border_side(ams_base_[unit_idx], LV_BORDER_SIDE_FULL, 0);
  lv_obj_align(ams_base_[unit_idx], LV_ALIGN_CENTER, 0, 35);
  lv_obj_clear_flag(ams_base_[unit_idx], LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(ams_base_[unit_idx], LV_OBJ_FLAG_CLICKABLE);
  enable_touch_bubble(ams_base_[unit_idx]);

  ams_tray_row_[unit_idx] = lv_obj_create(page);
  lv_obj_set_size(ams_tray_row_[unit_idx], 420, LV_SIZE_CONTENT);
  make_transparent(ams_tray_row_[unit_idx]);
  lv_obj_set_flex_flow(ams_tray_row_[unit_idx], LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(ams_tray_row_[unit_idx], LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(ams_tray_row_[unit_idx], 6, 0);
  lv_obj_align(ams_tray_row_[unit_idx], LV_ALIGN_CENTER, 0, -13);
  lv_obj_clear_flag(ams_tray_row_[unit_idx], LV_OBJ_FLAG_SCROLLABLE);
  enable_touch_bubble(ams_tray_row_[unit_idx]);

  for (int i = 0; i < kMaxAmsTrays; ++i) {
    ams_tray_col_[unit_idx][i] = lv_obj_create(ams_tray_row_[unit_idx]);
    lv_obj_set_size(ams_tray_col_[unit_idx][i], 76, LV_SIZE_CONTENT);
    make_transparent(ams_tray_col_[unit_idx][i]);
    lv_obj_set_flex_flow(ams_tray_col_[unit_idx][i], LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(ams_tray_col_[unit_idx][i], LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(ams_tray_col_[unit_idx][i], 4, 0);
    lv_obj_set_style_pad_all(ams_tray_col_[unit_idx][i], 0, 0);
    lv_obj_clear_flag(ams_tray_col_[unit_idx][i], LV_OBJ_FLAG_SCROLLABLE);

    ams_tray_rect_[unit_idx][i] = lv_obj_create(ams_tray_col_[unit_idx][i]);
    lv_obj_set_size(ams_tray_rect_[unit_idx][i], 72, 140);
    lv_obj_set_style_radius(ams_tray_rect_[unit_idx][i], 40, 0);
    lv_obj_set_style_bg_color(ams_tray_rect_[unit_idx][i], lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_opa(ams_tray_rect_[unit_idx][i], LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(ams_tray_rect_[unit_idx][i], 1, 0);
    lv_obj_set_style_border_color(ams_tray_rect_[unit_idx][i], lv_color_hex(0x555555), 0);
    lv_obj_set_style_border_opa(ams_tray_rect_[unit_idx][i], LV_OPA_COVER, 0);
    lv_obj_set_style_outline_width(ams_tray_rect_[unit_idx][i], 0, 0);
    lv_obj_set_style_pad_all(ams_tray_rect_[unit_idx][i], 0, 0);
    lv_obj_set_style_clip_corner(ams_tray_rect_[unit_idx][i], true, 0);
    lv_obj_clear_flag(ams_tray_rect_[unit_idx][i], LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(ams_tray_rect_[unit_idx][i], LV_OBJ_FLAG_CLICKABLE);
    // Diamond/rhombus pattern overlay drawn when this slot has an HMS error.
    lv_obj_add_event_cb(ams_tray_rect_[unit_idx][i], ams_pill_error_overlay_cb,
                        LV_EVENT_DRAW_POST, &ams_tray_error_[unit_idx][i]);

    ams_tray_type_[unit_idx][i] = lv_label_create(ams_tray_rect_[unit_idx][i]);
    set_label_text_if_changed(ams_tray_type_[unit_idx][i], "--");
    lv_obj_set_width(ams_tray_type_[unit_idx][i], 68);
    lv_label_set_long_mode(ams_tray_type_[unit_idx][i], LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(ams_tray_type_[unit_idx][i], LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(ams_tray_type_[unit_idx][i], &dosis_20, 0);
    lv_obj_set_style_text_color(ams_tray_type_[unit_idx][i], lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(ams_tray_type_[unit_idx][i], LV_ALIGN_TOP_MID, 0, 10);

    ams_tray_fill_[unit_idx][i] = lv_obj_create(ams_tray_rect_[unit_idx][i]);
    lv_obj_set_size(ams_tray_fill_[unit_idx][i], 72, 0);
    lv_obj_set_style_radius(ams_tray_fill_[unit_idx][i], 0, 0);
    lv_obj_set_style_bg_color(ams_tray_fill_[unit_idx][i], lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(ams_tray_fill_[unit_idx][i], LV_OPA_40, 0);
    lv_obj_set_style_border_width(ams_tray_fill_[unit_idx][i], 0, 0);
    lv_obj_align(ams_tray_fill_[unit_idx][i], LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_clear_flag(ams_tray_fill_[unit_idx][i], LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(ams_tray_fill_[unit_idx][i], LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(ams_tray_fill_[unit_idx][i], LV_OBJ_FLAG_HIDDEN);

    if (kOnxUiLayout) {
      ams_tray_name_[unit_idx][i] = lv_label_create(ams_tray_rect_[unit_idx][i]);
      set_label_text_if_changed(ams_tray_name_[unit_idx][i], "--");
      lv_obj_set_width(ams_tray_name_[unit_idx][i], 116);
      lv_label_set_long_mode(ams_tray_name_[unit_idx][i], LV_LABEL_LONG_DOT);
      lv_obj_set_style_text_align(ams_tray_name_[unit_idx][i], LV_TEXT_ALIGN_LEFT, 0);
      lv_obj_set_style_text_font(ams_tray_name_[unit_idx][i], &lv_font_montserrat_14, 0);
      lv_obj_set_style_text_color(ams_tray_name_[unit_idx][i], lv_color_hex(kOnxColorMuted), 0);

      ams_tray_flag_[unit_idx][i] = lv_label_create(ams_tray_rect_[unit_idx][i]);
      set_label_text_if_changed(ams_tray_flag_[unit_idx][i], "");
      lv_obj_set_width(ams_tray_flag_[unit_idx][i], 48);
      lv_label_set_long_mode(ams_tray_flag_[unit_idx][i], LV_LABEL_LONG_DOT);
      lv_obj_set_style_text_align(ams_tray_flag_[unit_idx][i], LV_TEXT_ALIGN_CENTER, 0);
      lv_obj_set_style_text_font(ams_tray_flag_[unit_idx][i], &lv_font_montserrat_14, 0);
      lv_obj_set_style_text_color(ams_tray_flag_[unit_idx][i], lv_color_hex(0x4ADE80), 0);
      lv_obj_add_flag(ams_tray_flag_[unit_idx][i], LV_OBJ_FLAG_HIDDEN);
    }

    ams_tray_arrow_[unit_idx][i] = lv_obj_create(ams_tray_col_[unit_idx][i]);
    lv_obj_set_size(ams_tray_arrow_[unit_idx][i], 40, 25);
    make_transparent(ams_tray_arrow_[unit_idx][i]);
    // bg_color carries the triangle color; bg_opa stays 0 so the OBJ background
    // is invisible and only the triangle drawn in ams_arrow_draw_cb is visible.
    lv_obj_set_style_bg_color(ams_tray_arrow_[unit_idx][i], lv_color_hex(0x1F1F1F), 0);
    lv_obj_set_style_pad_all(ams_tray_arrow_[unit_idx][i], 0, 0);
    lv_obj_clear_flag(ams_tray_arrow_[unit_idx][i], LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(ams_tray_arrow_[unit_idx][i], LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(ams_tray_arrow_[unit_idx][i], ams_arrow_draw_cb, LV_EVENT_DRAW_MAIN, nullptr);
  }

  // Floating percentage labels above pills (ignore layout)
  for (int i = 0; i < kMaxAmsTrays; ++i) {
    ams_tray_pct_[unit_idx][i] = lv_label_create(page);
    set_label_text_if_changed(ams_tray_pct_[unit_idx][i], "");
    lv_obj_set_style_text_font(ams_tray_pct_[unit_idx][i], &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(ams_tray_pct_[unit_idx][i], lv_color_hex(0xFFFFFF), 0);
    lv_obj_add_flag(ams_tray_pct_[unit_idx][i], LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ams_tray_pct_[unit_idx][i], LV_OBJ_FLAG_IGNORE_LAYOUT);
  }

  // Humidity pill
  ams_meta_row_[unit_idx] = lv_obj_create(page);
  lv_obj_t* hum_pill = ams_meta_row_[unit_idx];
  lv_obj_set_size(hum_pill, 139, 50);
  lv_obj_set_style_radius(hum_pill, 25, 0);
  lv_obj_set_style_bg_opa(hum_pill, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_color(hum_pill, lv_color_hex(0x9B9B9B), 0);
  lv_obj_set_style_border_opa(hum_pill, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(hum_pill, 1, 0);
  lv_obj_set_style_pad_all(hum_pill, 0, 0);
  lv_obj_align(hum_pill, LV_ALIGN_CENTER, 0, 135);
  lv_obj_clear_flag(hum_pill, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_flex_flow(hum_pill, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(hum_pill, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(hum_pill, 8, 0);

  ams_humidity_drop_[unit_idx] = lv_obj_create(hum_pill);
  lv_obj_set_size(ams_humidity_drop_[unit_idx], 14, 14);
  lv_obj_set_style_radius(ams_humidity_drop_[unit_idx], LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(ams_humidity_drop_[unit_idx], lv_color_hex(0x4A90D9), 0);
  lv_obj_set_style_bg_opa(ams_humidity_drop_[unit_idx], LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(ams_humidity_drop_[unit_idx], 0, 0);
  lv_obj_clear_flag(ams_humidity_drop_[unit_idx], LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(ams_humidity_drop_[unit_idx], LV_OBJ_FLAG_CLICKABLE);

  ams_humidity_label_[unit_idx] = lv_label_create(hum_pill);
  set_label_text_if_changed(ams_humidity_label_[unit_idx], "--");
  lv_obj_set_style_text_font(ams_humidity_label_[unit_idx], &dosis_20, 0);
  lv_obj_set_style_text_color(ams_humidity_label_[unit_idx], lv_color_hex(0x94A3B8), 0);
  lv_obj_set_style_text_align(ams_humidity_label_[unit_idx], LV_TEXT_ALIGN_CENTER, 0);

  ams_temp_label_[unit_idx] = lv_label_create(hum_pill);
  set_label_text_if_changed(ams_temp_label_[unit_idx], "--\xC2\xB0""C");
  lv_obj_set_style_text_font(ams_temp_label_[unit_idx], &dosis_20, 0);
  lv_obj_set_style_text_color(ams_temp_label_[unit_idx], lv_color_hex(0x94A3B8), 0);
  lv_obj_set_style_text_align(ams_temp_label_[unit_idx], LV_TEXT_ALIGN_CENTER, 0);

  // External-spool widgets only on the first AMS page.
  if (unit_idx == 0) {
    ams_ext_col_ = lv_obj_create(page);
    lv_obj_set_size(ams_ext_col_, 56, LV_SIZE_CONTENT);
    make_transparent(ams_ext_col_);
    lv_obj_set_flex_flow(ams_ext_col_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(ams_ext_col_, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(ams_ext_col_, 4, 0);
    lv_obj_set_style_pad_all(ams_ext_col_, 0, 0);
    lv_obj_add_flag(ams_ext_col_, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_clear_flag(ams_ext_col_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ams_ext_col_, LV_OBJ_FLAG_HIDDEN);

    ams_ext_rect_ = lv_obj_create(ams_ext_col_);
    lv_obj_set_size(ams_ext_rect_, 52, 108);
    lv_obj_set_style_radius(ams_ext_rect_, 32, 0);
    lv_obj_set_style_bg_color(ams_ext_rect_, lv_color_hex(0x444444), 0);
    lv_obj_set_style_bg_opa(ams_ext_rect_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(ams_ext_rect_, 1, 0);
    lv_obj_set_style_border_color(ams_ext_rect_, lv_color_hex(0x555555), 0);
    lv_obj_set_style_border_opa(ams_ext_rect_, LV_OPA_COVER, 0);
    lv_obj_set_style_outline_width(ams_ext_rect_, 0, 0);
    lv_obj_set_style_pad_all(ams_ext_rect_, 0, 0);
    lv_obj_set_style_clip_corner(ams_ext_rect_, true, 0);
    lv_obj_clear_flag(ams_ext_rect_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(ams_ext_rect_, LV_OBJ_FLAG_CLICKABLE);

    ams_ext_type_ = lv_label_create(ams_ext_rect_);
    set_label_text_if_changed(ams_ext_type_, "EXT");
    lv_obj_set_width(ams_ext_type_, 48);
    lv_label_set_long_mode(ams_ext_type_, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(ams_ext_type_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(ams_ext_type_, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(ams_ext_type_, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(ams_ext_type_, LV_ALIGN_CENTER, 0, 8);

    ams_ext_mat_ = lv_label_create(ams_ext_rect_);
    set_label_text_if_changed(ams_ext_mat_, "");
    lv_obj_set_width(ams_ext_mat_, 48);
    lv_label_set_long_mode(ams_ext_mat_, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(ams_ext_mat_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(ams_ext_mat_, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(ams_ext_mat_, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(ams_ext_mat_, LV_ALIGN_TOP_MID, 0, 12);

    if (kOnxUiLayout) {
      ams_ext_status_ = lv_label_create(ams_ext_col_);
      set_label_text_if_changed(ams_ext_status_, "active");
      lv_obj_set_width(ams_ext_status_, 64);
      lv_label_set_long_mode(ams_ext_status_, LV_LABEL_LONG_DOT);
      lv_obj_set_style_text_align(ams_ext_status_, LV_TEXT_ALIGN_RIGHT, 0);
      lv_obj_set_style_text_font(ams_ext_status_, &lv_font_montserrat_14, 0);
      lv_obj_set_style_text_color(ams_ext_status_, lv_color_hex(0x4ADE80), 0);
      lv_obj_add_flag(ams_ext_status_, LV_OBJ_FLAG_HIDDEN);
    }

    ams_ext_arrow_ = lv_obj_create(ams_ext_col_);
    lv_obj_set_size(ams_ext_arrow_, 35, 23);
    make_transparent(ams_ext_arrow_);
    // bg_color carries the triangle color; bg_opa stays 0 (see ams_arrow_draw_cb).
    lv_obj_set_style_bg_color(ams_ext_arrow_, lv_color_hex(0x1F1F1F), 0);
    lv_obj_set_style_pad_all(ams_ext_arrow_, 0, 0);
    lv_obj_clear_flag(ams_ext_arrow_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(ams_ext_arrow_, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(ams_ext_arrow_, ams_arrow_draw_cb, LV_EVENT_DRAW_MAIN, nullptr);
  }

  if (kOnxUiLayout) {
    set_hidden(ams_unit_label_[unit_idx], true);
    set_hidden(ams_shelf_[unit_idx], true);
    set_hidden(ams_base_[unit_idx], true);

    lv_obj_set_pos(ams_tray_row_[unit_idx], 12, kOnxLandscapeLayout ? 52 : 90);
    lv_obj_set_size(ams_tray_row_[unit_idx], kOnxLandscapeLayout ? 456 : 296,
                    kOnxLandscapeLayout ? 132 : 214);
    lv_obj_set_layout(ams_tray_row_[unit_idx], LV_LAYOUT_NONE);
    lv_obj_clear_flag(ams_tray_row_[unit_idx], LV_OBJ_FLAG_CLICKABLE);

    for (int i = 0; i < kMaxAmsTrays; ++i) {
      lv_obj_t* col = ams_tray_col_[unit_idx][i];
      lv_obj_t* rect = ams_tray_rect_[unit_idx][i];
      lv_obj_t* pct = ams_tray_pct_[unit_idx][i];
      lv_obj_t* arrow = ams_tray_arrow_[unit_idx][i];

      if (kOnxLandscapeLayout) {
        constexpr int kAmsLandscapeTrayX0 = 6;
        constexpr int kAmsLandscapeTrayW = 105;
        constexpr int kAmsLandscapeTrayGap = 8;
        constexpr int kAmsLandscapeTrayH = 128;
        lv_obj_set_pos(col, kAmsLandscapeTrayX0 + i * (kAmsLandscapeTrayW + kAmsLandscapeTrayGap),
                       0);
        lv_obj_set_size(col, kAmsLandscapeTrayW, kAmsLandscapeTrayH);
      } else {
        lv_obj_set_pos(col, (i % 2) * 153, (i / 2) * 112);
        lv_obj_set_size(col, 143, 102);
      }
      lv_obj_set_layout(col, LV_LAYOUT_NONE);
      lv_obj_clear_flag(col, LV_OBJ_FLAG_CLICKABLE);

      lv_obj_set_pos(rect, 0, 0);
      lv_obj_set_size(rect, kOnxLandscapeLayout ? 105 : 143,
                      kOnxLandscapeLayout ? 128 : 102);
      lv_obj_set_style_radius(rect, 8, 0);
      lv_obj_set_style_bg_color(rect, lv_color_hex(kOnxColorPanel2), 0);
      lv_obj_set_style_border_width(rect, 2, 0);
      lv_obj_set_style_border_color(rect, lv_color_hex(kOnxColorLine), 0);
      lv_obj_set_style_pad_all(rect, 0, 0);
      lv_obj_set_style_clip_corner(rect, true, 0);

      lv_obj_set_pos(ams_tray_type_[unit_idx][i], kOnxLandscapeLayout ? 8 : 10,
                     kOnxLandscapeLayout ? 10 : 8);
      lv_obj_set_width(ams_tray_type_[unit_idx][i], kOnxLandscapeLayout ? 92 : 123);
      lv_obj_set_style_text_align(ams_tray_type_[unit_idx][i], LV_TEXT_ALIGN_LEFT, 0);
      lv_obj_set_style_text_font(ams_tray_type_[unit_idx][i], &dosis_20, 0);
      lv_obj_set_style_text_color(ams_tray_type_[unit_idx][i], lv_color_hex(kOnxColorText), 0);

      lv_obj_set_pos(ams_tray_name_[unit_idx][i], kOnxLandscapeLayout ? 8 : 10,
                     kOnxLandscapeLayout ? 38 : 36);
      lv_obj_set_width(ams_tray_name_[unit_idx][i], kOnxLandscapeLayout ? 92 : 116);
      lv_obj_set_pos(ams_tray_fill_[unit_idx][i], kOnxLandscapeLayout ? 8 : 10,
                     kOnxLandscapeLayout ? 72 : 72);
      lv_obj_set_size(ams_tray_fill_[unit_idx][i], kOnxLandscapeLayout ? 56 : 52, 12);
      lv_obj_set_style_radius(ams_tray_fill_[unit_idx][i], 6, 0);
      lv_obj_set_style_bg_opa(ams_tray_fill_[unit_idx][i], LV_OPA_COVER, 0);

      lv_obj_set_parent(pct, rect);
      lv_obj_clear_flag(pct, LV_OBJ_FLAG_IGNORE_LAYOUT);
      lv_obj_set_pos(pct, kOnxLandscapeLayout ? 8 : 72,
                     kOnxLandscapeLayout ? 96 : 66);
      lv_obj_set_width(pct, kOnxLandscapeLayout ? 56 : 52);
      lv_label_set_long_mode(pct, LV_LABEL_LONG_DOT);
      lv_obj_set_style_text_font(pct, &lv_font_montserrat_20, 0);
      lv_obj_set_style_text_align(pct, kOnxLandscapeLayout ? LV_TEXT_ALIGN_LEFT : LV_TEXT_ALIGN_RIGHT, 0);

      lv_obj_set_pos(ams_tray_flag_[unit_idx][i], kOnxLandscapeLayout ? 66 : 86,
                     kOnxLandscapeLayout ? 98 : 78);
      lv_obj_set_size(ams_tray_flag_[unit_idx][i], kOnxLandscapeLayout ? 34 : 48,
                      kOnxLandscapeLayout ? 22 : LV_SIZE_CONTENT);
      lv_obj_set_style_radius(ams_tray_flag_[unit_idx][i], 11, 0);
      lv_obj_set_style_pad_top(ams_tray_flag_[unit_idx][i], kOnxLandscapeLayout ? 2 : 0, 0);
      set_hidden(arrow, true);
    }

    lv_obj_set_pos(ams_meta_row_[unit_idx], kOnxLandscapeLayout ? 180 : 12,
                   kOnxLandscapeLayout ? 10 : 48);
    lv_obj_set_size(ams_meta_row_[unit_idx], kOnxLandscapeLayout ? 288 : 296,
                    kOnxLandscapeLayout ? 28 : 22);
    make_transparent(ams_meta_row_[unit_idx]);
    lv_obj_set_layout(ams_meta_row_[unit_idx], LV_LAYOUT_NONE);
    lv_obj_clear_flag(ams_meta_row_[unit_idx], LV_OBJ_FLAG_CLICKABLE);
    set_hidden(ams_humidity_drop_[unit_idx], true);

    lv_obj_set_parent(ams_humidity_label_[unit_idx], page);
    lv_obj_set_pos(ams_humidity_label_[unit_idx], kOnxLandscapeLayout ? 180 : 12,
                   kOnxLandscapeLayout ? 14 : 48);
    lv_obj_set_width(ams_humidity_label_[unit_idx], kOnxLandscapeLayout ? 124 : 138);
    lv_obj_set_style_text_font(ams_humidity_label_[unit_idx], &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_align(ams_humidity_label_[unit_idx], LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_color(ams_humidity_label_[unit_idx], lv_color_hex(kOnxColorMuted), 0);

    lv_obj_set_parent(ams_temp_label_[unit_idx], page);
    lv_obj_set_pos(ams_temp_label_[unit_idx], kOnxLandscapeLayout ? 304 : 170,
                   kOnxLandscapeLayout ? 14 : 48);
    lv_obj_set_width(ams_temp_label_[unit_idx], kOnxLandscapeLayout ? 96 : 138);
    lv_obj_set_style_text_font(ams_temp_label_[unit_idx], &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_align(ams_temp_label_[unit_idx],
                                kOnxLandscapeLayout ? LV_TEXT_ALIGN_LEFT : LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_color(ams_temp_label_[unit_idx], lv_color_hex(kOnxColorMuted), 0);

    if (unit_idx == 0 && ams_ext_col_ != nullptr) {
      lv_obj_set_parent(ams_ext_col_, page);
      lv_obj_set_pos(ams_ext_col_, 12, kOnxLandscapeLayout ? 196 : 320);
      lv_obj_set_size(ams_ext_col_, kOnxLandscapeLayout ? 456 : 296, 44);
      lv_obj_set_layout(ams_ext_col_, LV_LAYOUT_NONE);
      lv_obj_set_style_radius(ams_ext_col_, 8, 0);
      lv_obj_set_style_bg_color(ams_ext_col_, lv_color_hex(kOnxColorPanel), 0);
      lv_obj_set_style_bg_opa(ams_ext_col_, LV_OPA_COVER, 0);
      lv_obj_set_style_border_width(ams_ext_col_, 1, 0);
      lv_obj_set_style_border_color(ams_ext_col_, lv_color_hex(kOnxColorLine), 0);
      lv_obj_set_style_border_opa(ams_ext_col_, LV_OPA_COVER, 0);
      lv_obj_set_style_pad_all(ams_ext_col_, 0, 0);
      lv_obj_clear_flag(ams_ext_col_, LV_OBJ_FLAG_CLICKABLE);

      lv_obj_set_parent(ams_ext_type_, ams_ext_col_);
      lv_obj_set_pos(ams_ext_type_, 10, 12);
      lv_obj_set_width(ams_ext_type_, kOnxLandscapeLayout ? 340 : 206);
      lv_obj_set_style_text_font(ams_ext_type_, &lv_font_montserrat_14, 0);
      lv_obj_set_style_text_align(ams_ext_type_, LV_TEXT_ALIGN_LEFT, 0);
      lv_obj_set_style_text_color(ams_ext_type_, lv_color_hex(kOnxColorSoft), 0);

      set_hidden(ams_ext_rect_, true);
      set_hidden(ams_ext_mat_, true);
      set_hidden(ams_ext_arrow_, true);
      if (ams_ext_status_ != nullptr) {
        lv_obj_set_pos(ams_ext_status_, kOnxLandscapeLayout ? 376 : 222, 12);
        lv_obj_set_width(ams_ext_status_, kOnxLandscapeLayout ? 70 : 64);
      }
    }
  }

  ams_note_[unit_idx] = lv_label_create(page);
  set_label_text_if_changed(ams_note_[unit_idx], "No AMS connected");
  lv_obj_set_width(ams_note_[unit_idx], 280);
  lv_label_set_long_mode(ams_note_[unit_idx], LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(ams_note_[unit_idx], LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(ams_note_[unit_idx], &dosis_20, 0);
  lv_obj_set_style_text_color(ams_note_[unit_idx], lv_color_hex(0x666666), 0);
  lv_obj_align(ams_note_[unit_idx], LV_ALIGN_CENTER, 0, 0);
  if (kOnxUiLayout) {
    lv_obj_set_pos(ams_note_[unit_idx], 12, kOnxLandscapeLayout ? 248 : 180);
    lv_obj_set_size(ams_note_[unit_idx], kOnxLandscapeLayout ? 456 : 296,
                    kOnxLandscapeLayout ? 36 : LV_SIZE_CONTENT);
    lv_obj_set_style_text_font(ams_note_[unit_idx],
                               kOnxLandscapeLayout ? &lv_font_montserrat_14 : &dosis_20, 0);
    lv_obj_set_style_text_color(ams_note_[unit_idx], lv_color_hex(kOnxColorSoft), 0);
    lv_obj_set_style_text_align(ams_note_[unit_idx],
                                kOnxLandscapeLayout ? LV_TEXT_ALIGN_LEFT : LV_TEXT_ALIGN_CENTER, 0);
  }
  lv_obj_add_flag(ams_note_[unit_idx], LV_OBJ_FLAG_HIDDEN);
}

void Ui::compute_ams_tray_errors(const PrinterSnapshot& snapshot) {
  for (int u = 0; u < kMaxAmsUnits; ++u) {
    for (int s = 0; s < kMaxAmsTrays; ++s) {
      ams_tray_error_[u][s] = false;
    }
  }
  for (uint64_t code : snapshot.hms_codes) {
    const uint32_t attr = static_cast<uint32_t>(code >> 32);
    const uint8_t module_id = (attr >> 24) & 0xFFU;
    if (module_id != 0x07U) continue;  // not an AMS-class HMS code
    const uint8_t module_num = (attr >> 16) & 0xFFU;
    int unit_idx = (module_num >= 128) ? (module_num - 128) : module_num;
    if (unit_idx < 0 || unit_idx >= kMaxAmsUnits) continue;
    const uint8_t part_id = (attr >> 8) & 0xFFU;
    // Slot mapping (BambuStudio convention): part_id values 0x20..0x23 map to
    // slots 0..3. Other part_id values describe unit-level errors and are
    // ignored for per-slot overlay (a unit-level indicator could be added
    // later via a shared marker on ams_unit_label_).
    if (part_id >= 0x20U && part_id <= 0x20U + (kMaxAmsTrays - 1)) {
      const int slot_idx = part_id - 0x20U;
      ams_tray_error_[unit_idx][slot_idx] = true;
    }
  }
  apply_ams_error_pulse_locked();
}

void Ui::ams_error_pulse_timer_cb(lv_timer_t* timer) {
  auto* ui = static_cast<Ui*>(lv_timer_get_user_data(timer));
  if (ui == nullptr) return;
  ui->ams_error_pulse_phase_ = (ui->ams_error_pulse_phase_ + 1U) & 0x3FU;
  // Triangle wave 0..32 → blend factor between dark and red. The arrow stays
  // fully opaque (LV_OPA_COVER); only the color changes, so it never bleeds
  // through the pill background like opacity-based pulsing did.
  const int32_t phase = static_cast<int32_t>(ui->ams_error_pulse_phase_);
  const int32_t tri = phase < 32 ? phase : (64 - phase);  // 0..32
  // Blend factor 0..255 (with a small floor so the dim end stays visible).
  const int32_t mix = 60 + tri * 6;  // 60..252
  // Lerp 0x1F1F1F → 0xEF4444
  const int32_t r = (0x1F * (255 - mix) + 0xEF * mix) / 255;
  const int32_t g = (0x1F * (255 - mix) + 0x44 * mix) / 255;
  const int32_t b = (0x1F * (255 - mix) + 0x44 * mix) / 255;
  const lv_color_t col = lv_color_hex(
      (static_cast<uint32_t>(r) << 16) |
      (static_cast<uint32_t>(g) << 8) |
      static_cast<uint32_t>(b));
  for (int u = 0; u < kMaxAmsUnits; ++u) {
    for (int s = 0; s < kMaxAmsTrays; ++s) {
      if (!ui->ams_tray_error_[u][s]) continue;
      lv_obj_t* arrow = ui->ams_tray_arrow_[u][s];
      if (arrow == nullptr) continue;
      lv_obj_set_style_bg_color(arrow, col, 0);
      lv_obj_invalidate(arrow);
    }
  }
}

void Ui::apply_ams_error_pulse_locked() {
  bool any_error = false;
  for (int u = 0; u < kMaxAmsUnits && !any_error; ++u) {
    for (int s = 0; s < kMaxAmsTrays && !any_error; ++s) {
      if (ams_tray_error_[u][s]) any_error = true;
    }
  }

  // Force pill redraw so the rhombus overlay event-cb re-runs with the new
  // flag value. Arrow color/opacity is owned by render_ams_unit() (non-error)
  // and the pulse timer (error) — we must not override it here, otherwise we
  // would clobber the green "active" indicator on non-error slots.
  for (int u = 0; u < kMaxAmsUnits; ++u) {
    for (int s = 0; s < kMaxAmsTrays; ++s) {
      if (ams_tray_rect_[u][s] != nullptr) {
        lv_obj_invalidate(ams_tray_rect_[u][s]);
      }
    }
  }

  if (any_error && ams_error_pulse_timer_ == nullptr) {
    ams_error_pulse_timer_ = lv_timer_create(&Ui::ams_error_pulse_timer_cb, 60, this);
  } else if (!any_error && ams_error_pulse_timer_ != nullptr) {
    lv_timer_delete(ams_error_pulse_timer_);
    ams_error_pulse_timer_ = nullptr;
    ams_error_pulse_phase_ = 0;
  }
}

void Ui::render_ams_unit(int unit_idx, const PrinterSnapshot& snapshot, bool show_unit_label) {
  if (unit_idx < 0 || unit_idx >= kMaxAmsUnits) return;
  if (snapshot.ams == nullptr) return;
  const AmsUnitInfo& unit = snapshot.ams->units[unit_idx];

  // Unit header label
  if (ams_unit_label_[unit_idx] != nullptr) {
    set_hidden(ams_unit_label_[unit_idx], !show_unit_label);
  }

  const bool ext_spool_active =
      unit_idx == 0 && (snapshot.tray_now == 254 || snapshot.tray_tar == 254);

  if (kOnxUiLayout) {
    set_hidden(ams_unit_label_[unit_idx], true);
    set_hidden(ams_tray_row_[unit_idx], false);
    set_hidden(ams_note_[unit_idx], true);
    set_hidden(ams_meta_row_[unit_idx], false);
    set_hidden(ams_humidity_label_[unit_idx], false);
    set_hidden(ams_temp_label_[unit_idx], false);

    if (kOnxLandscapeLayout) {
      bool any_slot_error = false;
      int first_error_slot = -1;

      for (int i = 0; i < kMaxAmsTrays; ++i) {
        const AmsTrayInfo& tray = unit.trays[i];
        lv_obj_t* rect = ams_tray_rect_[unit_idx][i];
        const bool has_error = ams_tray_error_[unit_idx][i];
        if (has_error && first_error_slot < 0) {
          first_error_slot = i;
          any_slot_error = true;
        }

        if (tray.present) {
          const uint32_t rgb = tray.color_rgba != 0
              ? ((tray.color_rgba >> 8) & 0x00FFFFFF)
              : kOnxColorPanel2;
          const bool dark = tray.color_rgba == 0 || color_is_dark(rgb);
          const lv_color_t text_color = lv_color_hex(dark ? 0xFFFFFF : 0x101418);
          const lv_color_t muted_color = lv_color_hex(dark ? 0xE5E7EB : 0x27313A);

          lv_obj_set_style_bg_color(rect, lv_color_hex(rgb), 0);
          lv_obj_set_style_bg_opa(rect, LV_OPA_COVER, 0);
          lv_obj_set_style_border_width(rect, 2, 0);
          lv_obj_set_style_border_color(rect,
              lv_color_hex(has_error ? 0xEF4444 : (tray.active ? 0x4ADE80 : kOnxColorLine)), 0);
          lv_obj_set_style_outline_width(rect, 0, 0);

          set_label_text_if_changed(ams_tray_type_[unit_idx][i],
                                    tray.material_type.empty() ? "Material" : tray.material_type);
          lv_obj_set_style_text_color(ams_tray_type_[unit_idx][i], text_color, 0);

          set_label_text_if_changed(ams_tray_name_[unit_idx][i],
                                    tray.material_name.empty() ? "--" : tray.material_name);
          lv_obj_set_style_text_color(ams_tray_name_[unit_idx][i], muted_color, 0);

          lv_obj_add_flag(ams_tray_fill_[unit_idx][i], LV_OBJ_FLAG_HIDDEN);

          char pct_buf[16] = {};
          if (tray.remain_pct >= 0) {
            std::snprintf(pct_buf, sizeof(pct_buf), "%d%%", tray.remain_pct);
          } else {
            std::snprintf(pct_buf, sizeof(pct_buf), "--");
          }
          set_label_text_if_changed(ams_tray_pct_[unit_idx][i], pct_buf);
          lv_obj_set_style_text_color(ams_tray_pct_[unit_idx][i], text_color, 0);
          lv_obj_clear_flag(ams_tray_pct_[unit_idx][i], LV_OBJ_FLAG_HIDDEN);

          if (has_error) {
            set_label_text_if_changed(ams_tray_flag_[unit_idx][i], "ERR");
            lv_obj_set_style_text_color(ams_tray_flag_[unit_idx][i], lv_color_hex(0xFFFFFF), 0);
            lv_obj_set_style_bg_color(ams_tray_flag_[unit_idx][i], lv_color_hex(0xEF4444), 0);
            lv_obj_set_style_bg_opa(ams_tray_flag_[unit_idx][i], LV_OPA_COVER, 0);
            lv_obj_clear_flag(ams_tray_flag_[unit_idx][i], LV_OBJ_FLAG_HIDDEN);
          } else if (tray.active) {
            set_label_text_if_changed(ams_tray_flag_[unit_idx][i], ">");
            lv_obj_set_style_text_color(ams_tray_flag_[unit_idx][i], lv_color_hex(0x052E16), 0);
            lv_obj_set_style_bg_color(ams_tray_flag_[unit_idx][i], lv_color_hex(0x4ADE80), 0);
            lv_obj_set_style_bg_opa(ams_tray_flag_[unit_idx][i], LV_OPA_COVER, 0);
            lv_obj_clear_flag(ams_tray_flag_[unit_idx][i], LV_OBJ_FLAG_HIDDEN);
          } else {
            lv_obj_add_flag(ams_tray_flag_[unit_idx][i], LV_OBJ_FLAG_HIDDEN);
          }
        } else {
          lv_obj_set_style_bg_color(rect, lv_color_hex(kOnxColorPanel2), 0);
          lv_obj_set_style_bg_opa(rect, LV_OPA_COVER, 0);
          lv_obj_set_style_border_width(rect, 2, 0);
          lv_obj_set_style_border_color(rect, lv_color_hex(has_error ? 0xEF4444 : kOnxColorLine), 0);
          lv_obj_set_style_outline_width(rect, 0, 0);
          set_label_text_if_changed(ams_tray_type_[unit_idx][i], "Empty");
          lv_obj_set_style_text_color(ams_tray_type_[unit_idx][i], lv_color_hex(kOnxColorMuted), 0);
          char slot_buf[16] = {};
          std::snprintf(slot_buf, sizeof(slot_buf), "Slot %d", i + 1);
          set_label_text_if_changed(ams_tray_name_[unit_idx][i], slot_buf);
          lv_obj_set_style_text_color(ams_tray_name_[unit_idx][i], lv_color_hex(kOnxColorMuted), 0);
          lv_obj_add_flag(ams_tray_fill_[unit_idx][i], LV_OBJ_FLAG_HIDDEN);
          set_label_text_if_changed(ams_tray_pct_[unit_idx][i], "--");
          lv_obj_set_style_text_color(ams_tray_pct_[unit_idx][i], lv_color_hex(kOnxColorMuted), 0);
          lv_obj_clear_flag(ams_tray_pct_[unit_idx][i], LV_OBJ_FLAG_HIDDEN);
          if (has_error) {
            set_label_text_if_changed(ams_tray_flag_[unit_idx][i], "ERR");
            lv_obj_set_style_text_color(ams_tray_flag_[unit_idx][i], lv_color_hex(0xFFFFFF), 0);
            lv_obj_set_style_bg_color(ams_tray_flag_[unit_idx][i], lv_color_hex(0xEF4444), 0);
            lv_obj_set_style_bg_opa(ams_tray_flag_[unit_idx][i], LV_OPA_COVER, 0);
            lv_obj_clear_flag(ams_tray_flag_[unit_idx][i], LV_OBJ_FLAG_HIDDEN);
          } else {
            lv_obj_add_flag(ams_tray_flag_[unit_idx][i], LV_OBJ_FLAG_HIDDEN);
          }
        }
      }

      char hum_buf[24] = {};
      if (unit.humidity_pct >= 0) {
        std::snprintf(hum_buf, sizeof(hum_buf), "Humidity %d%%", unit.humidity_pct);
      } else {
        std::snprintf(hum_buf, sizeof(hum_buf), "Humidity --");
      }
      set_label_text_if_changed(ams_humidity_label_[unit_idx], hum_buf);

      char temp_buf[24] = {};
      if (unit.temperature_c > 0.0f) {
        std::snprintf(temp_buf, sizeof(temp_buf), "Temp %.0f%s", unit.temperature_c, kDegreeC);
      } else {
        std::snprintf(temp_buf, sizeof(temp_buf), "Temp --%s", kDegreeC);
      }
      set_label_text_if_changed(ams_temp_label_[unit_idx], temp_buf);

      if (unit_idx == 0 && ams_ext_col_ != nullptr) {
        set_hidden(ams_ext_col_, !ext_spool_active);
        set_hidden(ams_ext_status_, !ext_spool_active);
        if (ext_spool_active) {
          const AmsTrayInfo& ext = snapshot.ams->external_spool;
          std::string ext_label = "EXT - ";
          ext_label += ext.material_type.empty() ? "Material" : ext.material_type;
          if (!ext.material_name.empty()) {
            ext_label += " ";
            ext_label += ext.material_name;
          }
          set_label_text_if_changed(ams_ext_type_, ext_label);
          set_label_text_if_changed(ams_ext_status_, "active");
        }
      } else if (unit_idx != 0 && ams_ext_col_ != nullptr) {
        set_hidden(ams_ext_col_, true);
        set_hidden(ams_ext_status_, true);
      }

      if (any_slot_error && first_error_slot >= 0) {
        char note_buf[48] = {};
        std::snprintf(note_buf, sizeof(note_buf), "Slot %d HMS error visible", first_error_slot + 1);
        set_label_text_if_changed(ams_note_[unit_idx], note_buf);
        lv_obj_set_style_text_color(ams_note_[unit_idx], lv_color_hex(0xEF4444), 0);
      } else {
        set_label_text_if_changed(ams_note_[unit_idx], "AMS trays are display-only");
        lv_obj_set_style_text_color(ams_note_[unit_idx], lv_color_hex(kOnxColorMuted), 0);
      }
      set_hidden(ams_note_[unit_idx], false);

      return;
    }

    for (int i = 0; i < kMaxAmsTrays; ++i) {
      const AmsTrayInfo& tray = unit.trays[i];
      lv_obj_t* rect = ams_tray_rect_[unit_idx][i];
      const bool has_error = ams_tray_error_[unit_idx][i];

      if (tray.present) {
        const uint32_t rgb = tray.color_rgba != 0
            ? ((tray.color_rgba >> 8) & 0x00FFFFFF)
            : kOnxColorPanel2;
        lv_obj_set_style_bg_color(rect, lv_color_hex(kOnxColorPanel2), 0);
        lv_obj_set_style_bg_opa(rect, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(rect, 2, 0);
        lv_obj_set_style_border_color(rect,
            lv_color_hex(has_error ? 0xEF4444 : (tray.active ? 0x4ADE80 : kOnxColorLine)), 0);
        lv_obj_set_style_outline_width(rect, 0, 0);

        const char* type_label = tray.material_type.empty() ? "Material" : tray.material_type.c_str();
        set_label_text_if_changed(ams_tray_type_[unit_idx][i], type_label);
        lv_obj_set_style_text_color(ams_tray_type_[unit_idx][i], lv_color_hex(kOnxColorText), 0);

        const char* name_label = !tray.material_name.empty() ? tray.material_name.c_str() : "--";
        set_label_text_if_changed(ams_tray_name_[unit_idx][i], name_label);
        lv_obj_set_style_text_color(ams_tray_name_[unit_idx][i], lv_color_hex(kOnxColorMuted), 0);

        lv_obj_set_style_bg_color(ams_tray_fill_[unit_idx][i], lv_color_hex(rgb), 0);
        lv_obj_clear_flag(ams_tray_fill_[unit_idx][i], LV_OBJ_FLAG_HIDDEN);

        char pct_buf[16] = {};
        if (tray.remain_pct >= 0) {
          std::snprintf(pct_buf, sizeof(pct_buf), "%d%%", tray.remain_pct);
        } else {
          std::snprintf(pct_buf, sizeof(pct_buf), "--");
        }
        set_label_text_if_changed(ams_tray_pct_[unit_idx][i], pct_buf);
        lv_obj_set_style_text_color(ams_tray_pct_[unit_idx][i], lv_color_hex(kOnxColorSoft), 0);
        lv_obj_clear_flag(ams_tray_pct_[unit_idx][i], LV_OBJ_FLAG_HIDDEN);

        if (has_error) {
          set_label_text_if_changed(ams_tray_flag_[unit_idx][i], "ERR");
          lv_obj_set_style_text_color(ams_tray_flag_[unit_idx][i], lv_color_hex(0xEF4444), 0);
          lv_obj_clear_flag(ams_tray_flag_[unit_idx][i], LV_OBJ_FLAG_HIDDEN);
        } else if (tray.active) {
          set_label_text_if_changed(ams_tray_flag_[unit_idx][i], "active");
          lv_obj_set_style_text_color(ams_tray_flag_[unit_idx][i], lv_color_hex(0x4ADE80), 0);
          lv_obj_clear_flag(ams_tray_flag_[unit_idx][i], LV_OBJ_FLAG_HIDDEN);
        } else {
          lv_obj_add_flag(ams_tray_flag_[unit_idx][i], LV_OBJ_FLAG_HIDDEN);
        }
      } else {
        lv_obj_set_style_bg_color(rect, lv_color_hex(kOnxColorPanel2), 0);
        lv_obj_set_style_bg_opa(rect, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(rect, 2, 0);
        lv_obj_set_style_border_color(rect, lv_color_hex(has_error ? 0xEF4444 : kOnxColorLine), 0);
        lv_obj_set_style_outline_width(rect, 0, 0);
        set_label_text_if_changed(ams_tray_type_[unit_idx][i], "Empty");
        lv_obj_set_style_text_color(ams_tray_type_[unit_idx][i], lv_color_hex(kOnxColorMuted), 0);
        char slot_buf[16] = {};
        std::snprintf(slot_buf, sizeof(slot_buf), "Slot %d", i + 1);
        set_label_text_if_changed(ams_tray_name_[unit_idx][i], slot_buf);
        lv_obj_set_style_text_color(ams_tray_name_[unit_idx][i], lv_color_hex(kOnxColorMuted), 0);
        lv_obj_add_flag(ams_tray_fill_[unit_idx][i], LV_OBJ_FLAG_HIDDEN);
        set_label_text_if_changed(ams_tray_pct_[unit_idx][i], "--");
        lv_obj_set_style_text_color(ams_tray_pct_[unit_idx][i], lv_color_hex(kOnxColorMuted), 0);
        lv_obj_clear_flag(ams_tray_pct_[unit_idx][i], LV_OBJ_FLAG_HIDDEN);
        if (has_error) {
          set_label_text_if_changed(ams_tray_flag_[unit_idx][i], "ERR");
          lv_obj_set_style_text_color(ams_tray_flag_[unit_idx][i], lv_color_hex(0xEF4444), 0);
          lv_obj_clear_flag(ams_tray_flag_[unit_idx][i], LV_OBJ_FLAG_HIDDEN);
        } else {
          lv_obj_add_flag(ams_tray_flag_[unit_idx][i], LV_OBJ_FLAG_HIDDEN);
        }
      }
    }

    char hum_buf[24] = {};
    if (unit.humidity_pct >= 0) {
      std::snprintf(hum_buf, sizeof(hum_buf), "Humidity %d%%", unit.humidity_pct);
    } else {
      std::snprintf(hum_buf, sizeof(hum_buf), "Humidity --");
    }
    set_label_text_if_changed(ams_humidity_label_[unit_idx], hum_buf);

    char temp_buf[24] = {};
    if (unit.temperature_c > 0.0f) {
      std::snprintf(temp_buf, sizeof(temp_buf), "Temp %.0f%s", unit.temperature_c, kDegreeC);
    } else {
      std::snprintf(temp_buf, sizeof(temp_buf), "Temp --%s", kDegreeC);
    }
    set_label_text_if_changed(ams_temp_label_[unit_idx], temp_buf);

    if (unit_idx == 0 && ams_ext_col_ != nullptr) {
      set_hidden(ams_ext_col_, !ext_spool_active);
      set_hidden(ams_ext_status_, !ext_spool_active);
      if (ext_spool_active) {
        const AmsTrayInfo& ext = snapshot.ams->external_spool;
        std::string ext_label = "EXT - ";
        if (!ext.material_type.empty()) {
          ext_label += ext.material_type;
        } else {
          ext_label += "Material";
        }
        if (!ext.material_name.empty()) {
          ext_label += " ";
          ext_label += ext.material_name;
        }
        set_label_text_if_changed(ams_ext_type_, ext_label);
        set_label_text_if_changed(ams_ext_status_, "active");
      }
    } else if (unit_idx != 0 && ams_ext_col_ != nullptr) {
      set_hidden(ams_ext_col_, true);
      set_hidden(ams_ext_status_, true);
    }

    return;
  }

  // Dynamic ext-spool layout shrink — only on AMS page 0.
  if (unit_idx == 0 && ext_spool_active != ams_ext_spool_shown_) {
    ams_ext_spool_shown_ = ext_spool_active;
    if (ext_spool_active) {
      for (int i = 0; i < kMaxAmsTrays; ++i) {
        lv_obj_set_size(ams_tray_col_[0][i], 58, LV_SIZE_CONTENT);
        lv_obj_set_size(ams_tray_rect_[0][i], 54, 108);
        lv_obj_set_style_radius(ams_tray_rect_[0][i], 30, 0);
        lv_obj_set_width(ams_tray_type_[0][i], 50);
        lv_obj_set_size(ams_tray_fill_[0][i], 54, 0);
      }
      lv_obj_set_size(ams_shelf_[0], 275, 85);
      lv_obj_align(ams_shelf_[0], LV_ALIGN_CENTER, 38, -55);
      lv_obj_set_size(ams_base_[0], 300, 80);
      lv_obj_align(ams_base_[0], LV_ALIGN_CENTER, 38, 19);
      lv_obj_set_size(ams_tray_row_[0], 310, LV_SIZE_CONTENT);
      lv_obj_align(ams_tray_row_[0], LV_ALIGN_CENTER, 38, -30);
      lv_obj_align(ams_ext_col_, LV_ALIGN_CENTER, -155, -30);
      lv_obj_clear_flag(ams_ext_col_, LV_OBJ_FLAG_HIDDEN);
    } else {
      for (int i = 0; i < kMaxAmsTrays; ++i) {
        lv_obj_set_size(ams_tray_col_[0][i], 76, LV_SIZE_CONTENT);
        lv_obj_set_size(ams_tray_rect_[0][i], 72, 140);
        lv_obj_set_style_radius(ams_tray_rect_[0][i], 40, 0);
        lv_obj_set_width(ams_tray_type_[0][i], 68);
        lv_obj_set_size(ams_tray_fill_[0][i], 72, 0);
      }
      lv_obj_set_size(ams_shelf_[0], 359, 110);
      lv_obj_align(ams_shelf_[0], LV_ALIGN_CENTER, 0, -50);
      lv_obj_set_size(ams_base_[0], 385, 103);
      lv_obj_align(ams_base_[0], LV_ALIGN_CENTER, 0, 35);
      lv_obj_set_size(ams_tray_row_[0], 420, LV_SIZE_CONTENT);
      lv_obj_align(ams_tray_row_[0], LV_ALIGN_CENTER, 0, -13);
      lv_obj_add_flag(ams_ext_col_, LV_OBJ_FLAG_HIDDEN);
    }
  }

  // External spool styling (only on unit 0 when active).
  if (unit_idx == 0 && ext_spool_active) {
    const AmsTrayInfo& ext = snapshot.ams->external_spool;
    if (ext.color_rgba != 0) {
      const uint32_t ext_rgb = (ext.color_rgba >> 8) & 0x00FFFFFF;
      lv_obj_set_style_bg_color(ams_ext_rect_, lv_color_hex(ext_rgb), 0);
      const bool ext_dark = ((ext_rgb >> 16) & 0xFF) * 299 +
                            ((ext_rgb >> 8) & 0xFF) * 587 +
                            (ext_rgb & 0xFF) * 114 < 128000;
      const lv_color_t txt_col = lv_color_hex(ext_dark ? 0xFFFFFF : 0x000000);
      lv_obj_set_style_text_color(ams_ext_type_, txt_col, 0);
      lv_obj_set_style_text_color(ams_ext_mat_, txt_col, 0);
    } else {
      lv_obj_set_style_bg_color(ams_ext_rect_, lv_color_hex(0x444444), 0);
      lv_obj_set_style_text_color(ams_ext_type_, lv_color_hex(0xFFFFFF), 0);
      lv_obj_set_style_text_color(ams_ext_mat_, lv_color_hex(0xFFFFFF), 0);
    }
    lv_obj_set_style_bg_opa(ams_ext_rect_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(ams_ext_rect_, 1, 0);
    lv_obj_set_style_border_color(ams_ext_rect_, lv_color_hex(0x555555), 0);
    lv_obj_set_style_outline_width(ams_ext_rect_, 0, 0);
    lv_obj_set_style_bg_color(ams_ext_arrow_, lv_color_hex(0x4ADE80), 0);
    const char* mat_label = !ext.material_type.empty() ? ext.material_type.c_str() : "";
    set_label_text_if_changed(ams_ext_mat_, mat_label);
  }

  const int pill_h = (unit_idx == 0 && ext_spool_active) ? 108 : 140;

  for (int i = 0; i < kMaxAmsTrays; ++i) {
    const AmsTrayInfo& tray = unit.trays[i];
    lv_obj_t* rect = ams_tray_rect_[unit_idx][i];
    lv_obj_t* arrow = ams_tray_arrow_[unit_idx][i];
    const bool has_error = ams_tray_error_[unit_idx][i];
    if (tray.present) {
      const uint32_t rgba = tray.color_rgba;
      const uint32_t rgb = (rgba >> 8) & 0x00FFFFFF;
      lv_obj_set_style_bg_color(rect, lv_color_hex(rgb), 0);
      lv_obj_set_style_bg_opa(rect, LV_OPA_COVER, 0);
      lv_obj_set_style_border_width(rect, 1, 0);
      lv_obj_set_style_border_color(rect, lv_color_hex(0x555555), 0);
      lv_obj_set_style_outline_width(rect, 0, 0);
      if (has_error) {
        // Red triangle on error (color pulses via timer).
        lv_obj_set_style_bg_color(arrow, lv_color_hex(0xEF4444), 0);
      } else if (tray.active) {
        lv_obj_set_style_bg_color(arrow, lv_color_hex(0x4ADE80), 0);
      } else {
        lv_obj_set_style_bg_color(arrow, lv_color_hex(0x1F1F1F), 0);
      }
      set_label_text_if_changed(ams_tray_type_[unit_idx][i],
                                tray.material_type.empty() ? "--" : tray.material_type);
      const bool is_dark = ((rgb >> 16) & 0xFF) * 299 +
                           ((rgb >> 8) & 0xFF) * 587 +
                           (rgb & 0xFF) * 114 < 128000;
      lv_obj_set_style_text_color(ams_tray_type_[unit_idx][i],
          lv_color_hex(is_dark ? 0xFFFFFF : 0x000000), 0);

      if (tray.remain_pct >= 0) {
        const int empty_h = pill_h - (pill_h * tray.remain_pct / 100);
        lv_obj_set_height(ams_tray_fill_[unit_idx][i], empty_h);
        lv_obj_align(ams_tray_fill_[unit_idx][i], LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_clear_flag(ams_tray_fill_[unit_idx][i], LV_OBJ_FLAG_HIDDEN);
        char pct_buf[8];
        std::snprintf(pct_buf, sizeof(pct_buf), "%d%%", tray.remain_pct);
        set_label_text_if_changed(ams_tray_pct_[unit_idx][i], pct_buf);
        lv_obj_set_style_text_color(ams_tray_pct_[unit_idx][i],
            lv_color_hex(is_dark ? 0xFFFFFF : 0x000000), 0);
        lv_obj_clear_flag(ams_tray_pct_[unit_idx][i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_align_to(ams_tray_pct_[unit_idx][i], rect,
                        LV_ALIGN_BOTTOM_MID, 0, -10);
      } else {
        lv_obj_add_flag(ams_tray_fill_[unit_idx][i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ams_tray_pct_[unit_idx][i], LV_OBJ_FLAG_HIDDEN);
      }
    } else {
      lv_obj_set_style_bg_color(rect, lv_color_hex(0x333333), 0);
      lv_obj_set_style_bg_opa(rect, LV_OPA_COVER, 0);
      lv_obj_set_style_border_width(rect, 1, 0);
      lv_obj_set_style_border_color(rect, lv_color_hex(0x555555), 0);
      lv_obj_set_style_outline_width(rect, 0, 0);
      set_label_text_if_changed(ams_tray_type_[unit_idx][i], "Empty");
      lv_obj_add_flag(ams_tray_fill_[unit_idx][i], LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(ams_tray_pct_[unit_idx][i], LV_OBJ_FLAG_HIDDEN);
      if (has_error) {
        lv_obj_set_style_bg_color(arrow, lv_color_hex(0xEF4444), 0);
      } else {
        lv_obj_set_style_bg_color(arrow, lv_color_hex(0x1F1F1F), 0);
      }
    }
  }

  // Humidity + temperature
  char hum_buf[16] = {};
  if (unit.humidity_pct >= 0) {
    std::snprintf(hum_buf, sizeof(hum_buf), "%d%%", unit.humidity_pct);
  } else {
    std::snprintf(hum_buf, sizeof(hum_buf), "--%% ");
  }
  set_label_text_if_changed(ams_humidity_label_[unit_idx], hum_buf);

  char temp_buf[24] = {};
  if (unit.temperature_c > 0.0f) {
    std::snprintf(temp_buf, sizeof(temp_buf), "%.0f%s", unit.temperature_c, kDegreeC);
  } else {
    std::snprintf(temp_buf, sizeof(temp_buf), "--%s", kDegreeC);
  }
  set_label_text_if_changed(ams_temp_label_[unit_idx], temp_buf);

  set_hidden(ams_tray_row_[unit_idx], false);
  set_hidden(ams_note_[unit_idx], true);
}

esp_err_t Ui::build_dashboard() {
  LvglLockGuard lock(3000, "build_dashboard");
  if (!lock.locked()) {
    return ESP_ERR_TIMEOUT;
  }

  screen_ = lv_screen_active();
  lv_obj_set_style_bg_color(screen_, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(screen_, LV_OPA_COVER, 0);

  const lv_font_t* dosis20 = &dosis_20;
  const lv_font_t* dosis32 = &dosis_32;
  const lv_font_t* dosis40 = &dosis_40;
  const lv_font_t* info20 = &lv_font_montserrat_20;
  const lv_font_t* mdi30 = &mdi_30;
  const lv_font_t* mdi40 = &mdi_40;

  pager_ = lv_obj_create(screen_);
  lv_obj_set_size(pager_, board::kDisplayWidth, board::kDisplayHeight);
  lv_obj_center(pager_);
  make_transparent(pager_);
  apply_display_rotation_visual_offset(pager_, display_rotation_);
  lv_obj_set_style_pad_column(pager_, 0, 0);
  lv_obj_set_style_pad_row(pager_, 0, 0);
  enable_touch_bubble(pager_);
  lv_obj_set_flex_flow(pager_, LV_FLEX_FLOW_ROW);
  lv_obj_set_scroll_dir(pager_, kOnxUiLayout ? LV_DIR_NONE : LV_DIR_HOR);
  // LV_SCROLL_SNAP_NONE: we handle page snapping ourselves in handle_pager_event.
  // LV_SCROLL_SNAP_CENTER would make LVGL launch its own snap animation that conflicts
  // with our set_active_page() call, causing double-animation jitter and page-skip bugs.
  lv_obj_set_scroll_snap_x(pager_, LV_SCROLL_SNAP_NONE);
  lv_obj_set_scrollbar_mode(pager_, LV_SCROLLBAR_MODE_OFF);
  lv_obj_add_event_cb(pager_, &Ui::pager_event_cb, LV_EVENT_ALL, this);

  auto create_page = [](lv_obj_t* parent) {
    lv_obj_t* page = lv_obj_create(parent);
    lv_obj_set_size(page, board::kDisplayWidth, board::kDisplayHeight);
    apply_page_root_style(page);
    lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);
    return page;
  };

  page0_ = create_page(pager_);
  for (int u = 0; u < kMaxAmsUnits; ++u) {
    ams_pages_[u] = create_page(pager_);
    enable_touch_bubble(ams_pages_[u]);
  }
  page1_ = create_page(pager_);
  page2_ = create_page(pager_);
  page3_ = create_page(pager_);
  enable_touch_bubble(page0_);
  enable_touch_bubble(page1_);
  enable_touch_bubble(page2_);
  enable_touch_bubble(page3_);

  add_onx_page_chrome(page0_, "Printers",
                      kOnxLandscapeLayout ? "Web Config" : "1/8",
                      kOnxLandscapeLayout ? "" : "Hold: Web Config PIN",
                      dosis20, info20);
  for (int u = 0; u < kMaxAmsUnits; ++u) {
    char ams_title[16] = {};
    char ams_meta[8] = {};
    std::snprintf(ams_title, sizeof(ams_title), "AMS %d", u + 1);
    std::snprintf(ams_meta, sizeof(ams_meta), "%d/8", u + 2);
    add_onx_page_chrome(ams_pages_[u], ams_title, ams_meta, "AMS trays are display-only",
                        dosis20, info20);
  }
  add_onx_page_chrome(page2_, "Cover", "7/8", "Cover image is display-only", dosis20, info20);
  add_onx_page_chrome(page3_, "Camera", "8/8",
                      kOnxLandscapeLayout ? "" : "Tap image area to refresh",
                      dosis20, info20);

  // --- Page 0: printer selection ---
  page0_title_ = lv_label_create(page0_);
  set_label_text_if_changed(page0_title_, "Printers");
  lv_obj_set_width(page0_title_, 320);
  lv_label_set_long_mode(page0_title_, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(page0_title_, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(page0_title_, dosis32, 0);
  lv_obj_set_style_text_color(page0_title_, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(page0_title_, LV_ALIGN_TOP_MID, 0, 60);
  if (kOnxUiLayout) {
    lv_obj_add_flag(page0_title_, LV_OBJ_FLAG_HIDDEN);
  }

  page0_card_list_ = lv_obj_create(page0_);
  if (kOnxUiLayout) {
    lv_obj_set_pos(page0_card_list_, kOnxPrinterListX, kOnxPrinterListY);
    lv_obj_set_size(page0_card_list_, kOnxPrinterListW, kOnxPrinterListH);
  } else {
    lv_obj_set_size(page0_card_list_, 380, 300);
    lv_obj_align(page0_card_list_, LV_ALIGN_CENTER, 0, 20);
  }
  make_transparent(page0_card_list_);
  lv_obj_set_flex_flow(page0_card_list_, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(page0_card_list_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(page0_card_list_, 10, 0);
  lv_obj_set_style_pad_all(page0_card_list_, 0, 0);
  lv_obj_set_scroll_dir(page0_card_list_, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(page0_card_list_, LV_SCROLLBAR_MODE_OFF);
  enable_touch_bubble(page0_card_list_);

  page0_empty_note_ = lv_label_create(page0_);
  set_label_text_if_changed(page0_empty_note_, "No printers configured.\nUse the web portal to add printers.");
  lv_obj_set_width(page0_empty_note_, kOnxUiLayout ? 296 : 320);
  lv_label_set_long_mode(page0_empty_note_, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(page0_empty_note_, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(page0_empty_note_, info20, 0);
  lv_obj_set_style_text_color(page0_empty_note_,
                              lv_color_hex(kOnxUiLayout ? kOnxColorMuted : 0x666666), 0);
  if (kOnxUiLayout) {
    lv_obj_set_pos(page0_empty_note_, 12, kOnxLandscapeLayout ? 116 : 180);
    lv_obj_set_size(page0_empty_note_, kOnxLandscapeLayout ? 300 : 296,
                    kOnxLandscapeLayout ? 72 : 100);
  } else {
    lv_obj_align(page0_empty_note_, LV_ALIGN_CENTER, 0, 20);
  }
  lv_obj_add_flag(page0_empty_note_, LV_OBJ_FLAG_HIDDEN);

  if (kOnxLandscapeLayout) {
    page0_detail_panel_ = create_onx_panel(page0_, 324, 48, 144, 228, kOnxColorPanel);
    enable_touch_bubble(page0_detail_panel_);

    page0_detail_title_ = lv_label_create(page0_detail_panel_);
    set_label_text_if_changed(page0_detail_title_, "No printer");
    lv_obj_set_pos(page0_detail_title_, 12, 12);
    lv_obj_set_size(page0_detail_title_, 120, 40);
    lv_label_set_long_mode(page0_detail_title_, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(page0_detail_title_, dosis20, 0);
    set_dynamic_text_font(page0_detail_title_);
    lv_obj_set_style_text_color(page0_detail_title_, lv_color_hex(kOnxColorText), 0);

    page0_detail_state_ = lv_label_create(page0_detail_panel_);
    set_label_text_if_changed(page0_detail_state_, "No profile");
    lv_obj_set_pos(page0_detail_state_, 12, 60);
    lv_obj_set_size(page0_detail_state_, 120, 20);
    lv_label_set_long_mode(page0_detail_state_, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(page0_detail_state_, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(page0_detail_state_, lv_color_hex(kOnxColorMuted), 0);

    page0_detail_host_ = lv_label_create(page0_detail_panel_);
    set_label_text_if_changed(page0_detail_host_, "--");
    lv_obj_set_pos(page0_detail_host_, 12, 88);
    lv_obj_set_size(page0_detail_host_, 120, 20);
    lv_label_set_long_mode(page0_detail_host_, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(page0_detail_host_, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(page0_detail_host_, lv_color_hex(kOnxColorMuted), 0);

    page0_detail_hint_ = lv_label_create(page0_detail_panel_);
    set_label_text_if_changed(page0_detail_hint_, "Web Config");
    lv_obj_set_pos(page0_detail_hint_, 12, 120);
    lv_obj_set_size(page0_detail_hint_, 120, 16);
    lv_label_set_long_mode(page0_detail_hint_, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(page0_detail_hint_, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(page0_detail_hint_, lv_color_hex(kOnxColorSoft), 0);

    page0_detail_ip_ = lv_label_create(page0_detail_panel_);
    set_label_text_if_changed(page0_detail_ip_, "No IP");
    lv_obj_set_pos(page0_detail_ip_, 12, 138);
    lv_obj_set_size(page0_detail_ip_, 120, 20);
    lv_label_set_long_mode(page0_detail_ip_, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(page0_detail_ip_, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(page0_detail_ip_, lv_color_hex(kOnxColorMuted), 0);

    page0_detail_pin_hint_ = lv_label_create(page0_detail_panel_);
    set_label_text_if_changed(page0_detail_pin_hint_, "Hold PIN");
    lv_obj_set_pos(page0_detail_pin_hint_, 12, 166);
    lv_obj_set_size(page0_detail_pin_hint_, 120, 18);
    lv_label_set_long_mode(page0_detail_pin_hint_, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(page0_detail_pin_hint_, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(page0_detail_pin_hint_, lv_color_hex(kOnxColorSoft), 0);
  }

  // --- AMS pages (one per AMS unit, indices kPageIdxAmsFirst..kPageIdxAmsLast) ---
  // Build all kMaxAmsUnits AMS pages up front; each page is hidden until the
  // backend reports a corresponding unit. The first AMS page additionally hosts
  // the external-spool widgets (which dynamically shrink that page's pills).
  for (int u = 0; u < kMaxAmsUnits; ++u) {
    build_ams_page(u);
    lv_obj_add_flag(ams_pages_[u], LV_OBJ_FLAG_HIDDEN);
    ams_unit_present_[u] = false;
  }
  ams_ext_spool_shown_ = false;


  fixed_overlay_ = lv_obj_create(screen_);
  lv_obj_set_size(fixed_overlay_, board::kDisplayWidth, board::kDisplayHeight);
  lv_obj_center(fixed_overlay_);
  make_transparent(fixed_overlay_);
  apply_display_rotation_visual_offset(fixed_overlay_, display_rotation_);
  lv_obj_clear_flag(fixed_overlay_, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(fixed_overlay_, LV_OBJ_FLAG_SCROLLABLE);
  enable_touch_bubble(fixed_overlay_);
  lv_obj_move_foreground(fixed_overlay_);

  status_arc_ = lv_arc_create(fixed_overlay_);
  lv_obj_set_size(status_arc_, board::kDisplayWidth, board::kDisplayHeight);
  lv_obj_remove_flag(status_arc_, LV_OBJ_FLAG_CLICKABLE);
  lv_arc_set_rotation(status_arc_, 270);
  lv_arc_set_bg_angles(status_arc_, 0, 360);
  lv_arc_set_range(status_arc_, 0, 100);
  lv_arc_set_value(status_arc_, 0);
  lv_obj_center(status_arc_);
  lv_obj_set_style_arc_width(status_arc_, kRingStrokeWidth, LV_PART_MAIN);
  lv_obj_set_style_arc_width(status_arc_, kRingStrokeWidth, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(status_arc_, lv_color_hex(0x101010), LV_PART_MAIN);
  lv_obj_set_style_arc_color(status_arc_, lv_color_hex(arc_colors_.printing), LV_PART_INDICATOR);
  lv_obj_set_style_arc_rounded(status_arc_, true, LV_PART_MAIN);
  lv_obj_set_style_arc_rounded(status_arc_, true, LV_PART_INDICATOR);
  lv_obj_set_style_opa(status_arc_, LV_OPA_TRANSP, LV_PART_KNOB);
  lv_obj_clear_flag(status_arc_, LV_OBJ_FLAG_SCROLLABLE);
  enable_touch_bubble(status_arc_);

  progress_label_ = lv_label_create(lv_layer_top());
  set_label_text_if_changed(progress_label_, "--%");
  lv_obj_set_style_text_font(progress_label_, dosis40, 0);
  lv_obj_set_style_text_color(progress_label_, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(progress_label_, LV_ALIGN_CENTER, 0, -178);
  apply_display_rotation_visual_offset(progress_label_, display_rotation_);
  lv_obj_move_foreground(progress_label_);

  battery_icon_label_ = lv_label_create(lv_layer_top());
  set_label_text_if_changed(battery_icon_label_, kMdiBattery100);
  lv_obj_set_style_text_font(battery_icon_label_, mdi30, 0);
  lv_obj_set_style_text_color(battery_icon_label_, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(battery_icon_label_, LV_ALIGN_CENTER, -20, -140);
  apply_display_rotation_visual_offset(battery_icon_label_, display_rotation_);
  lv_obj_move_foreground(battery_icon_label_);

  battery_pct_label_ = lv_label_create(lv_layer_top());
  set_label_text_if_changed(battery_pct_label_, "--%");
  lv_obj_set_style_text_font(battery_pct_label_, dosis20, 0);
  lv_obj_set_style_text_color(battery_pct_label_, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(battery_pct_label_, LV_ALIGN_CENTER, 20, -140);
  apply_display_rotation_visual_offset(battery_pct_label_, display_rotation_);
  lv_obj_move_foreground(battery_pct_label_);

  badge_slot_ = lv_obj_create(page1_);
  make_transparent(badge_slot_);
  lv_obj_set_size(badge_slot_, 86, 86);
  lv_obj_align(badge_slot_, LV_ALIGN_CENTER, 0, -7);
  lv_obj_clear_flag(badge_slot_, LV_OBJ_FLAG_SCROLLABLE);

  logo_badge_ = lv_obj_create(badge_slot_);
  lv_obj_set_size(logo_badge_, 120, 120);
  lv_obj_set_style_radius(logo_badge_, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_opa(logo_badge_, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_opa(logo_badge_, LV_OPA_TRANSP, 0);
  lv_obj_set_style_pad_all(logo_badge_, 0, 0);
  lv_obj_center(logo_badge_);
  lv_obj_clear_flag(logo_badge_, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(logo_badge_, LV_OBJ_FLAG_SCROLLABLE);
  enable_touch_bubble(logo_badge_);
  lv_obj_add_event_cb(logo_badge_, &Ui::logo_event_cb, LV_EVENT_CLICKED, this);

  logo_image_ = lv_image_create(logo_badge_);
  lv_image_set_src(logo_image_, &bambuicon_small);
  lv_image_set_scale(logo_image_, 183);
  lv_image_set_antialias(logo_image_, true);
  lv_obj_set_style_image_recolor_opa(logo_image_, LV_OPA_TRANSP, 0);
  lv_obj_center(logo_image_);
  lv_obj_clear_flag(logo_image_, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(logo_image_, LV_OBJ_FLAG_SCROLLABLE);

  status_label_ = lv_label_create(page1_);
  set_label_text_if_changed(status_label_, "waiting...");
  lv_obj_set_style_text_font(status_label_, dosis32, 0);
  lv_obj_set_style_text_color(status_label_, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(status_label_, LV_ALIGN_CENTER, 0, -86);

  detail_label_ = lv_label_create(page1_);
  set_label_text_if_changed(detail_label_, "Waiting for printer data");
  lv_label_set_long_mode(detail_label_, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(detail_label_, 320);
  lv_obj_set_style_text_align(detail_label_, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(detail_label_, info20, 0);
  lv_obj_set_style_text_color(detail_label_, lv_color_hex(0x94A3B8), 0);
  lv_obj_align(detail_label_, LV_ALIGN_CENTER, 0, 114);

  layer_label_ = lv_label_create(page1_);
  set_label_text_if_changed(layer_label_, "Layer: -- / --");
  lv_obj_set_style_text_font(layer_label_, dosis32, 0);
  lv_obj_set_style_text_color(layer_label_, lv_color_hex(0xDDDDDD), 0);
  lv_obj_align(layer_label_, LV_ALIGN_CENTER, 0, 70);

  nozzle_prefix_label_ = lv_label_create(page1_);
  set_label_text_if_changed(nozzle_prefix_label_, kMdiNozzle);
  lv_obj_set_style_text_font(nozzle_prefix_label_, mdi40, 0);
  lv_obj_set_style_text_color(nozzle_prefix_label_, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(nozzle_prefix_label_, LV_ALIGN_CENTER, -182, -10);

  nozzle_value_label_ = lv_label_create(page1_);
  set_label_text_if_changed(nozzle_value_label_, "--°C");
  lv_obj_set_style_text_font(nozzle_value_label_, dosis32, 0);
  lv_obj_set_style_text_color(nozzle_value_label_, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(nozzle_value_label_, LV_ALIGN_CENTER, -132, -10);
  set_label_text_if_changed(nozzle_value_label_, std::string("--") + kDegreeC);

  nozzle_aux_label_ = lv_label_create(page1_);
  set_label_text_if_changed(nozzle_aux_label_, "");
  lv_obj_set_width(nozzle_aux_label_, 170);
  lv_label_set_long_mode(nozzle_aux_label_, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(nozzle_aux_label_, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(nozzle_aux_label_, dosis20, 0);
  lv_obj_set_style_text_color(nozzle_aux_label_, lv_color_hex(0x94A3B8), 0);
  lv_obj_align(nozzle_aux_label_, LV_ALIGN_CENTER, -132, kAuxTempRowY);
  lv_obj_add_flag(nozzle_aux_label_, LV_OBJ_FLAG_HIDDEN);

  bed_prefix_label_ = lv_label_create(page1_);
  set_label_text_if_changed(bed_prefix_label_, kMdiBed);
  lv_obj_set_style_text_font(bed_prefix_label_, mdi40, 0);
  lv_obj_set_style_text_color(bed_prefix_label_, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(bed_prefix_label_, LV_ALIGN_CENTER, 182, -10);

  bed_value_label_ = lv_label_create(page1_);
  set_label_text_if_changed(bed_value_label_, "--°C");
  lv_obj_set_style_text_font(bed_value_label_, dosis32, 0);
  lv_obj_set_style_text_color(bed_value_label_, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_width(bed_value_label_, 96);
  lv_obj_set_style_text_align(bed_value_label_, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_align(bed_value_label_, LV_ALIGN_CENTER, 108, -10);
  set_label_text_if_changed(bed_value_label_, std::string("--") + kDegreeC);

  bed_aux_label_ = lv_label_create(page1_);
  set_label_text_if_changed(bed_aux_label_, "");
  lv_obj_set_width(bed_aux_label_, 170);
  lv_label_set_long_mode(bed_aux_label_, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(bed_aux_label_, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(bed_aux_label_, dosis20, 0);
  lv_obj_set_style_text_color(bed_aux_label_, lv_color_hex(0x94A3B8), 0);
  lv_obj_align(bed_aux_label_, LV_ALIGN_CENTER, 132, kAuxTempRowY);
  lv_obj_add_flag(bed_aux_label_, LV_OBJ_FLAG_HIDDEN);

  remaining_row_ = lv_obj_create(page1_);
  make_transparent(remaining_row_);
  lv_obj_set_size(remaining_row_, 280, LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(remaining_row_, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(remaining_row_, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_align(remaining_row_, LV_ALIGN_CENTER, 0, kRemainingRowY);
  lv_obj_clear_flag(remaining_row_, LV_OBJ_FLAG_SCROLLABLE);
  // Tap to toggle between remaining duration and predicted finish time.
  lv_obj_add_flag(remaining_row_, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(remaining_row_, &Ui::remaining_row_event_cb, LV_EVENT_CLICKED, this);

  remaining_prefix_label_ = lv_label_create(remaining_row_);
  set_label_text_if_changed(remaining_prefix_label_, kOnxLandscapeLayout ? "Rem" : kMdiClock);
  lv_obj_set_style_text_font(remaining_prefix_label_, mdi40, 0);
  lv_obj_set_style_text_color(remaining_prefix_label_, lv_color_hex(0x87CEEB), 0);
  lv_obj_set_style_pad_right(remaining_prefix_label_, 8, 0);

  remaining_label_ = lv_label_create(remaining_row_);
  set_label_text_if_changed(remaining_label_, "--m");
  lv_obj_set_style_text_font(remaining_label_, dosis40, 0);
  lv_obj_set_style_text_color(remaining_label_, lv_color_hex(0x87CEEB), 0);

  portal_hint_label_ = lv_label_create(page1_);
  set_label_text_if_changed(portal_hint_label_, "");
  lv_obj_set_width(portal_hint_label_, 320);
  lv_label_set_long_mode(portal_hint_label_, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(portal_hint_label_, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(portal_hint_label_, info20, 0);
  lv_obj_set_style_text_color(portal_hint_label_, lv_color_hex(0x64748B), 0);
  lv_obj_align(portal_hint_label_, LV_ALIGN_CENTER, 0, 114);
  lv_obj_add_flag(portal_hint_label_, LV_OBJ_FLAG_HIDDEN);

  if (kOnxUiLayout) {
    set_hidden(fixed_overlay_, true);
    set_hidden(status_arc_, true);

    onx_main_topbar_ = lv_obj_create(page1_);
    lv_obj_set_pos(onx_main_topbar_, kOnxPagePad, kOnxTopbarY);
    lv_obj_set_size(onx_main_topbar_, kOnxContentWidth, kOnxTopbarHeight);
    make_transparent(onx_main_topbar_);
    lv_obj_clear_flag(onx_main_topbar_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(onx_main_topbar_, LV_OBJ_FLAG_CLICKABLE);
    enable_touch_bubble(onx_main_topbar_);

    onx_top_status_label_ = lv_label_create(onx_main_topbar_);
    set_label_text_if_changed(onx_top_status_label_, "--% Waiting");
    lv_obj_set_width(onx_top_status_label_, kOnxLandscapeLayout ? 260 : 202);
    lv_label_set_long_mode(onx_top_status_label_, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(onx_top_status_label_, dosis20, 0);
    lv_obj_set_style_text_color(onx_top_status_label_, lv_color_hex(kOnxColorText), 0);
    lv_obj_align(onx_top_status_label_, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_set_parent(battery_icon_label_, onx_main_topbar_);
    lv_obj_set_parent(battery_pct_label_, onx_main_topbar_);
    lv_obj_set_style_translate_x(battery_icon_label_, 0, 0);
    lv_obj_set_style_translate_y(battery_icon_label_, 0, 0);
    lv_obj_set_style_translate_x(battery_pct_label_, 0, 0);
    lv_obj_set_style_translate_y(battery_pct_label_, 0, 0);
    lv_obj_set_style_text_font(battery_icon_label_, mdi30, 0);
    lv_obj_set_style_text_color(battery_icon_label_, lv_color_hex(kOnxColorMuted), 0);
    lv_obj_align(battery_icon_label_, LV_ALIGN_RIGHT_MID, kOnxLandscapeLayout ? -72 : -48, 0);
    lv_obj_set_width(battery_pct_label_, kOnxLandscapeLayout ? 68 : 44);
    lv_obj_set_style_text_align(battery_pct_label_, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_font(battery_pct_label_, dosis20, 0);
    lv_obj_set_style_text_color(battery_pct_label_, lv_color_hex(kOnxColorMuted), 0);
    lv_obj_align(battery_pct_label_, LV_ALIGN_RIGHT_MID, 0, 0);

    lv_obj_set_size(badge_slot_, 60, 60);
    lv_obj_align(badge_slot_, LV_ALIGN_TOP_LEFT,
                 kOnxLandscapeLayout ? 21 : 12,
                 kOnxLandscapeLayout ? 57 : 57);
    lv_obj_set_size(logo_badge_, 60, 60);
    lv_obj_set_style_radius(logo_badge_, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(logo_badge_, lv_color_hex(kOnxColorPanel), 0);
    lv_obj_set_style_bg_opa(logo_badge_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(logo_badge_, lv_color_hex(kOnxColorLine), 0);
    lv_obj_set_style_border_opa(logo_badge_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(logo_badge_, 1, 0);
    lv_obj_center(logo_badge_);
    lv_image_set_scale(logo_image_, kOnxLandscapeLayout ? 102 : 112);
    lv_obj_center(logo_image_);

    onx_job_label_ = lv_label_create(page1_);
    set_label_text_if_changed(onx_job_label_, "Waiting for printer data");
    lv_obj_set_pos(onx_job_label_, kOnxLandscapeLayout ? 220 : 90,
                   kOnxLandscapeLayout ? 48 : 54);
    lv_obj_set_size(onx_job_label_, kOnxLandscapeLayout ? 248 : 218,
                    kOnxLandscapeLayout ? 38 : 42);
    lv_label_set_long_mode(onx_job_label_, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(onx_job_label_, dosis20, 0);
    set_dynamic_text_font(onx_job_label_);
    lv_obj_set_style_text_color(onx_job_label_, lv_color_hex(kOnxColorText), 0);

    if (kOnxLandscapeLayout) {
      onx_layer_line_label_ = lv_label_create(page1_);
      set_label_text_if_changed(onx_layer_line_label_, "Layer: -- / --");
      lv_obj_set_pos(onx_layer_line_label_, 220, 90);
      lv_obj_set_size(onx_layer_line_label_, 248, 18);
      lv_label_set_long_mode(onx_layer_line_label_, LV_LABEL_LONG_DOT);
      lv_obj_set_style_text_align(onx_layer_line_label_, LV_TEXT_ALIGN_LEFT, 0);
      lv_obj_set_style_text_font(onx_layer_line_label_, info20, 0);
      lv_obj_set_style_text_color(onx_layer_line_label_, lv_color_hex(kOnxColorSoft), 0);

      lv_obj_set_pos(detail_label_, 220, 232);
      lv_obj_set_size(detail_label_, 248, 44);
      lv_label_set_long_mode(detail_label_, LV_LABEL_LONG_WRAP);
      lv_obj_set_style_text_align(detail_label_, LV_TEXT_ALIGN_LEFT, 0);
      lv_obj_set_style_text_font(detail_label_, &lv_font_montserrat_14, 0);
      set_dynamic_text_font(detail_label_);
      lv_obj_set_style_text_color(detail_label_, lv_color_hex(kOnxColorSoft), 0);
      lv_obj_set_style_bg_color(detail_label_, lv_color_hex(kOnxColorPanel), 0);
      lv_obj_set_style_bg_opa(detail_label_, LV_OPA_COVER, 0);
      lv_obj_set_style_border_color(detail_label_, lv_color_hex(kOnxColorLine), 0);
      lv_obj_set_style_border_opa(detail_label_, LV_OPA_COVER, 0);
      lv_obj_set_style_border_width(detail_label_, 1, 0);
      lv_obj_set_style_radius(detail_label_, 8, 0);
      lv_obj_set_style_pad_hor(detail_label_, 10, 0);
      lv_obj_set_style_pad_ver(detail_label_, 6, 0);
    } else {
      lv_obj_set_size(detail_label_, 218, 18);
      lv_obj_align(detail_label_, LV_ALIGN_TOP_LEFT, 90, 96);
      lv_label_set_long_mode(detail_label_, LV_LABEL_LONG_DOT);
      lv_obj_set_style_text_align(detail_label_, LV_TEXT_ALIGN_LEFT, 0);
      lv_obj_set_style_text_font(detail_label_, &lv_font_montserrat_14, 0);
      lv_obj_set_style_text_color(detail_label_, lv_color_hex(kOnxColorMuted), 0);
    }

    onx_progress_panel_ = create_onx_panel(page1_, kOnxMainLeftPanelX, kOnxMainLeftPanelY,
                                           kOnxMainLeftPanelW, kOnxMainLeftPanelH,
                                           kOnxColorPanel);
    lv_obj_set_parent(progress_label_, onx_progress_panel_);
    lv_obj_set_style_translate_x(progress_label_, 0, 0);
    lv_obj_set_style_translate_y(progress_label_, 0, 0);
    lv_obj_align(progress_label_, LV_ALIGN_TOP_LEFT,
                 kOnxLandscapeLayout ? 80 : 14,
                 kOnxLandscapeLayout ? 6 : 8);
    lv_obj_set_width(progress_label_, kOnxLandscapeLayout ? 104 : 110);
    lv_obj_set_style_text_font(progress_label_, kOnxLandscapeLayout ? dosis40 : dosis32, 0);
    lv_obj_set_style_text_align(progress_label_, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_parent(status_label_, onx_progress_panel_);
    lv_obj_align(status_label_, LV_ALIGN_TOP_LEFT,
                 kOnxLandscapeLayout ? 80 : 130,
                 kOnxLandscapeLayout ? 52 : 18);
    lv_obj_set_width(status_label_, kOnxLandscapeLayout ? 104 : 152);
    lv_obj_set_style_text_align(status_label_, kOnxLandscapeLayout ? LV_TEXT_ALIGN_LEFT : LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_font(status_label_, dosis20, 0);
    onx_progress_bar_ = lv_bar_create(onx_progress_panel_);
    lv_obj_set_pos(onx_progress_bar_, kOnxLandscapeLayout ? 12 : 12,
                   kOnxLandscapeLayout ? 94 : 84);
    lv_obj_set_size(onx_progress_bar_, kOnxLandscapeLayout ? 172 : 272,
                    kOnxLandscapeLayout ? 12 : 10);
    lv_bar_set_range(onx_progress_bar_, 0, 100);
    lv_bar_set_value(onx_progress_bar_, 0, LV_ANIM_OFF);
    lv_obj_set_style_radius(onx_progress_bar_, 6, LV_PART_MAIN);
    lv_obj_set_style_radius(onx_progress_bar_, 6, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(onx_progress_bar_, lv_color_hex(0x20252B), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(onx_progress_bar_, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(onx_progress_bar_, lv_color_hex(arc_colors_.printing), LV_PART_INDICATOR);

    onx_nozzle_card_ = create_onx_panel(page1_,
                                        kOnxLandscapeLayout ? 220 : 12,
                                        kOnxLandscapeLayout ? 112 : 258,
                                        kOnxLandscapeLayout ? 120 : 143,
                                        kOnxLandscapeLayout ? 52 : 66,
                                        kOnxColorPanel2);
    onx_bed_card_ = create_onx_panel(page1_,
                                     kOnxLandscapeLayout ? 348 : 165,
                                     kOnxLandscapeLayout ? 112 : 258,
                                     kOnxLandscapeLayout ? 120 : 143,
                                     kOnxLandscapeLayout ? 52 : 66,
                                     kOnxColorPanel2);
    onx_layer_card_ = create_onx_panel(page1_,
                                       kOnxLandscapeLayout ? 220 : 12,
                                       kOnxLandscapeLayout ? 172 : 334,
                                       kOnxLandscapeLayout ? 150 : 143,
                                       kOnxLandscapeLayout ? 52 : 66,
                                       kOnxColorPanel2);
    onx_detail_card_ = create_onx_panel(page1_,
                                        kOnxLandscapeLayout ? 378 : 165,
                                        kOnxLandscapeLayout ? 172 : 334,
                                        kOnxLandscapeLayout ? 90 : 143,
                                        kOnxLandscapeLayout ? 52 : 66,
                                        kOnxColorPanel2);
    create_onx_caption(onx_nozzle_card_, "Nozzle", 8, kOnxLandscapeLayout ? 5 : 8,
                       kOnxLandscapeLayout ? 108 : 123, &lv_font_montserrat_14);
    create_onx_caption(onx_bed_card_, "Bed", 8, kOnxLandscapeLayout ? 5 : 8,
                       kOnxLandscapeLayout ? 108 : 123, &lv_font_montserrat_14);
    create_onx_caption(onx_layer_card_, "Layer", 8, kOnxLandscapeLayout ? 5 : 8,
                       kOnxLandscapeLayout ? 138 : 123, &lv_font_montserrat_14);
    create_onx_caption(onx_detail_card_, "Detail", 8, kOnxLandscapeLayout ? 5 : 8,
                       kOnxLandscapeLayout ? 78 : 123, &lv_font_montserrat_14);

    lv_obj_set_parent(nozzle_value_label_, onx_nozzle_card_);
    lv_obj_align(nozzle_value_label_, LV_ALIGN_TOP_LEFT, 8, kOnxLandscapeLayout ? 23 : 32);
    lv_obj_set_size(nozzle_value_label_, kOnxLandscapeLayout ? 108 : 123,
                    kOnxLandscapeLayout ? 20 : 24);
    lv_label_set_long_mode(nozzle_value_label_, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(nozzle_value_label_, dosis20, 0);
    lv_obj_set_style_text_align(nozzle_value_label_, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_parent(nozzle_aux_label_, onx_nozzle_card_);
    lv_obj_align(nozzle_aux_label_, LV_ALIGN_TOP_LEFT,
                 kOnxLandscapeLayout ? 66 : 74,
                 kOnxLandscapeLayout ? 23 : 11);
    lv_obj_set_width(nozzle_aux_label_, kOnxLandscapeLayout ? 46 : 58);
    lv_label_set_long_mode(nozzle_aux_label_, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(nozzle_aux_label_, LV_TEXT_ALIGN_RIGHT, 0);
    set_hidden(nozzle_prefix_label_, true);

    lv_obj_set_parent(bed_value_label_, onx_bed_card_);
    lv_obj_align(bed_value_label_, LV_ALIGN_TOP_LEFT, 8, kOnxLandscapeLayout ? 23 : 32);
    lv_obj_set_size(bed_value_label_, kOnxLandscapeLayout ? 108 : 123,
                    kOnxLandscapeLayout ? 20 : 24);
    lv_label_set_long_mode(bed_value_label_, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(bed_value_label_, dosis20, 0);
    lv_obj_set_style_text_align(bed_value_label_, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_parent(bed_aux_label_, onx_bed_card_);
    lv_obj_align(bed_aux_label_, LV_ALIGN_TOP_LEFT,
                 kOnxLandscapeLayout ? 66 : 46,
                 kOnxLandscapeLayout ? 23 : 11);
    lv_obj_set_width(bed_aux_label_, kOnxLandscapeLayout ? 46 : 88);
    lv_label_set_long_mode(bed_aux_label_, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(bed_aux_label_, LV_TEXT_ALIGN_RIGHT, 0);
    set_hidden(bed_prefix_label_, true);

    lv_obj_set_parent(layer_label_, onx_layer_card_);
    lv_obj_align(layer_label_, LV_ALIGN_TOP_LEFT, 8, kOnxLandscapeLayout ? 23 : 32);
    lv_obj_set_size(layer_label_, kOnxLandscapeLayout ? 138 : 123,
                    kOnxLandscapeLayout ? 20 : 24);
    lv_label_set_long_mode(layer_label_, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(layer_label_, dosis20, 0);
    lv_obj_set_style_text_align(layer_label_, LV_TEXT_ALIGN_LEFT, 0);

    onx_metric_detail_label_ = lv_label_create(onx_detail_card_);
    set_label_text_if_changed(onx_metric_detail_label_, "Waiting");
    lv_obj_align(onx_metric_detail_label_, LV_ALIGN_TOP_LEFT, 8, kOnxLandscapeLayout ? 23 : 32);
    lv_obj_set_size(onx_metric_detail_label_, kOnxLandscapeLayout ? 78 : 123,
                    kOnxLandscapeLayout ? 20 : 24);
    lv_label_set_long_mode(onx_metric_detail_label_, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(onx_metric_detail_label_, dosis20, 0);
    set_dynamic_text_font(onx_metric_detail_label_);
    lv_obj_set_style_text_color(onx_metric_detail_label_, lv_color_hex(kOnxColorSoft), 0);

    if (kOnxLandscapeLayout) {
      lv_obj_set_parent(remaining_row_, onx_progress_panel_);
    }
    lv_obj_set_size(remaining_row_, kOnxLandscapeLayout ? 172 : 296,
                    kOnxLandscapeLayout ? 56 : 48);
    lv_obj_align(remaining_row_, LV_ALIGN_TOP_LEFT,
                 kOnxLandscapeLayout ? 12 : 12,
                 kOnxLandscapeLayout ? 126 : 412);
    lv_obj_set_style_bg_color(remaining_row_,
                              lv_color_hex(kOnxLandscapeLayout ? kOnxColorPanel2 : kOnxColorPanel),
                              0);
    lv_obj_set_style_bg_opa(remaining_row_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(remaining_row_, lv_color_hex(kOnxColorLine), 0);
    lv_obj_set_style_border_opa(remaining_row_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(remaining_row_, 1, 0);
    lv_obj_set_style_radius(remaining_row_, 8, 0);
    lv_obj_set_style_pad_all(remaining_row_, 0, 0);
    lv_obj_set_style_pad_column(remaining_row_, kOnxLandscapeLayout ? 4 : 8, 0);
    lv_obj_set_style_text_color(remaining_label_, lv_color_hex(kOnxColorCyan), 0);
    if (kOnxLandscapeLayout) {
      lv_obj_set_layout(remaining_row_, LV_LAYOUT_NONE);
      lv_obj_set_pos(remaining_label_, 8, 10);
      lv_obj_set_size(remaining_label_, 156, 36);
      lv_label_set_long_mode(remaining_label_, LV_LABEL_LONG_DOT);
      lv_obj_set_style_text_align(remaining_label_, LV_TEXT_ALIGN_CENTER, 0);
      lv_obj_set_style_text_font(remaining_label_, dosis32, 0);
      lv_obj_set_pos(remaining_prefix_label_, 14, 17);
      lv_obj_set_size(remaining_prefix_label_, 42, 18);
      lv_label_set_long_mode(remaining_prefix_label_, LV_LABEL_LONG_DOT);
      lv_obj_set_style_text_align(remaining_prefix_label_, LV_TEXT_ALIGN_LEFT, 0);
      lv_obj_set_style_text_font(remaining_prefix_label_, dosis20, 0);
      lv_obj_set_style_pad_right(remaining_prefix_label_, 0, 0);
      lv_obj_add_flag(remaining_prefix_label_, LV_OBJ_FLAG_HIDDEN);
      lv_obj_move_foreground(badge_slot_);
      lv_obj_move_foreground(remaining_row_);
    }

    lv_obj_set_size(portal_hint_label_, kOnxLandscapeLayout ? 456 : 296,
                    kOnxLandscapeLayout ? 20 : 16);
    lv_label_set_long_mode(portal_hint_label_, LV_LABEL_LONG_DOT);
    lv_obj_align(portal_hint_label_, LV_ALIGN_TOP_LEFT,
                 12, kOnxLandscapeLayout ? 292 : 462);
    lv_obj_set_style_text_align(portal_hint_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(portal_hint_label_, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(portal_hint_label_, lv_color_hex(kOnxColorMuted), 0);
  }

  brightness_overlay_ = lv_label_create(lv_layer_top());
  set_label_text_if_changed(brightness_overlay_, "80%");
  lv_obj_set_style_text_font(brightness_overlay_, dosis40, 0);
  lv_obj_set_style_text_color(brightness_overlay_, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_bg_color(brightness_overlay_, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(brightness_overlay_, LV_OPA_60, 0);
  lv_obj_set_style_pad_hor(brightness_overlay_, 20, 0);
  lv_obj_set_style_pad_ver(brightness_overlay_, 10, 0);
  lv_obj_set_style_radius(brightness_overlay_, 16, 0);
  lv_obj_align(brightness_overlay_, LV_ALIGN_CENTER, 0, 0);
  apply_display_rotation_visual_offset(brightness_overlay_, display_rotation_);
  lv_obj_add_flag(brightness_overlay_, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(brightness_overlay_);

  portal_overlay_card_ = lv_obj_create(lv_layer_top());
  lv_obj_set_size(portal_overlay_card_, 280, LV_SIZE_CONTENT);
  lv_obj_set_style_radius(portal_overlay_card_, 22, 0);
  lv_obj_set_style_bg_color(portal_overlay_card_, lv_color_hex(0x071018), 0);
  lv_obj_set_style_bg_opa(portal_overlay_card_, LV_OPA_90, 0);
  lv_obj_set_style_border_color(portal_overlay_card_, lv_color_hex(0xF0A64B), 0);
  lv_obj_set_style_border_width(portal_overlay_card_, 2, 0);
  lv_obj_set_style_pad_hor(portal_overlay_card_, 22, 0);
  lv_obj_set_style_pad_ver(portal_overlay_card_, 18, 0);
  lv_obj_set_style_pad_row(portal_overlay_card_, 8, 0);
  lv_obj_set_layout(portal_overlay_card_, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(portal_overlay_card_, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(portal_overlay_card_, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_center(portal_overlay_card_);
  apply_display_rotation_visual_offset(portal_overlay_card_, display_rotation_);
  lv_obj_clear_flag(portal_overlay_card_, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(portal_overlay_card_, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(portal_overlay_card_, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(portal_overlay_card_);

  portal_overlay_title_ = lv_label_create(portal_overlay_card_);
  set_label_text_if_changed(portal_overlay_title_, "WEB CONFIG PIN");
  lv_obj_set_style_text_font(portal_overlay_title_, dosis20, 0);
  lv_obj_set_style_text_color(portal_overlay_title_, lv_color_hex(0xF8FAFC), 0);

  portal_overlay_value_ = lv_label_create(portal_overlay_card_);
  set_label_text_if_changed(portal_overlay_value_, "000000");
  lv_obj_set_style_text_font(portal_overlay_value_, dosis40, 0);
  lv_obj_set_style_text_color(portal_overlay_value_, lv_color_hex(0xF0A64B), 0);

  portal_overlay_detail_ = lv_label_create(portal_overlay_card_);
  set_label_text_if_changed(portal_overlay_detail_, "");
  lv_obj_set_width(portal_overlay_detail_, 236);
  lv_label_set_long_mode(portal_overlay_detail_, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(portal_overlay_detail_, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(portal_overlay_detail_, dosis20, 0);
  lv_obj_set_style_text_color(portal_overlay_detail_, lv_color_hex(0xCBD5E1), 0);

  if (kOnxUiLayout) {
    page2_shell_ = create_onx_panel(page2_, 12, kOnxLandscapeLayout ? 48 : 60,
                                    kOnxPreviewImageSize, kOnxPreviewImageSize, kOnxColorPanel);
    if (kOnxLandscapeLayout) {
      page2_info_panel_ = create_onx_panel(page2_, 264, 48, 204, 240, kOnxColorPanel);
    } else {
      page2_info_panel_ = nullptr;
    }
  } else {
    page2_shell_ = nullptr;
    page2_info_panel_ = nullptr;
  }

  page2_image_ = lv_image_create(page2_);
  lv_obj_set_size(page2_image_, kOnxUiLayout ? kOnxPreviewImageSize : kPage2PreviewSize,
                  kOnxUiLayout ? kOnxPreviewImageSize : kPage2PreviewSize);
  lv_image_set_inner_align(page2_image_, LV_IMAGE_ALIGN_CONTAIN);
  if (kOnxUiLayout) {
    lv_obj_set_pos(page2_image_, 12, kOnxLandscapeLayout ? 48 : 60);
  } else {
    lv_obj_align(page2_image_, LV_ALIGN_CENTER, 0, kPage2PreviewYOffset);
  }
  lv_obj_add_flag(page2_image_, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(page2_image_, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(page2_image_, LV_OBJ_FLAG_SCROLLABLE);
  enable_touch_bubble(page2_image_);

  page2_note_ = lv_label_create(page2_);
  set_label_text_if_changed(page2_note_, "No cover image yet");
  lv_obj_set_width(page2_note_, kOnxLandscapeLayout ? 216 : (kOnxUiLayout ? 260 : 280));
  lv_label_set_long_mode(page2_note_, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(page2_note_, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(page2_note_, kOnxUiLayout ? info20 : dosis20, 0);
  lv_obj_set_style_text_color(page2_note_,
                              lv_color_hex(kOnxUiLayout ? kOnxColorSoft : 0x888888), 0);
  if (kOnxUiLayout) {
    lv_obj_set_pos(page2_note_, kOnxLandscapeLayout ? 24 : 30,
                   kOnxLandscapeLayout ? 150 : 194);
    if (kOnxLandscapeLayout) {
      lv_obj_set_size(page2_note_, 216, 48);
    }
  } else {
    lv_obj_align(page2_note_, LV_ALIGN_CENTER, 0, -14);
  }
  enable_touch_bubble(page2_note_);

  page2_subnote_ = lv_label_create(page2_);
  set_label_text_if_changed(page2_subnote_, "");
  lv_obj_set_width(page2_subnote_, kOnxLandscapeLayout ? 180 : (kOnxUiLayout ? 296 : 320));
  lv_label_set_long_mode(page2_subnote_,
                         kOnxUiLayout ? LV_LABEL_LONG_WRAP : LV_LABEL_LONG_SCROLL_CIRCULAR);
  lv_obj_set_style_text_align(page2_subnote_,
                              kOnxUiLayout ? LV_TEXT_ALIGN_LEFT : LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(page2_subnote_, info20, 0);
  set_dynamic_text_font(page2_subnote_);
  lv_obj_set_style_text_color(page2_subnote_,
                              lv_color_hex(kOnxUiLayout ? kOnxColorText : 0x888888), 0);
  if (kOnxUiLayout) {
    lv_obj_set_pos(page2_subnote_, kOnxLandscapeLayout ? 276 : 12,
                   kOnxLandscapeLayout ? 60 : 372);
    lv_obj_set_size(page2_subnote_, kOnxLandscapeLayout ? 180 : 296,
                    kOnxLandscapeLayout ? 72 : 48);
  } else {
    lv_obj_align(page2_subnote_, LV_ALIGN_CENTER, 0, 18);
  }
  lv_obj_add_flag(page2_subnote_, LV_OBJ_FLAG_HIDDEN);
  enable_touch_bubble(page2_subnote_);

  if (kOnxUiLayout) {
    page2_detail_label_ = lv_label_create(page2_);
    set_label_text_if_changed(page2_detail_label_, "");
    lv_obj_set_pos(page2_detail_label_, kOnxLandscapeLayout ? 276 : 12,
                   kOnxLandscapeLayout ? 142 : 428);
    lv_obj_set_size(page2_detail_label_, kOnxLandscapeLayout ? 180 : 296,
                    kOnxLandscapeLayout ? 96 : 20);
    lv_label_set_long_mode(page2_detail_label_,
                           kOnxLandscapeLayout ? LV_LABEL_LONG_WRAP : LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(page2_detail_label_, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(page2_detail_label_, &lv_font_montserrat_14, 0);
    set_dynamic_text_font(page2_detail_label_);
    lv_obj_set_style_text_color(page2_detail_label_, lv_color_hex(kOnxColorMuted), 0);
    lv_obj_add_flag(page2_detail_label_, LV_OBJ_FLAG_HIDDEN);
    enable_touch_bubble(page2_detail_label_);
  } else {
    page2_detail_label_ = nullptr;
  }

  if (kOnxUiLayout) {
    page3_shell_ = create_onx_panel(page3_, 12, kOnxLandscapeLayout ? 48 : 86,
                                    kOnxCameraImageWidth, kOnxCameraImageHeight,
                                    kOnxColorPanel);
    page3_info_panel_ =
        kOnxLandscapeLayout ? create_onx_panel(page3_, 324, 48, 144, 224, kOnxColorPanel) :
        lv_obj_create(page3_);
    if (!kOnxLandscapeLayout) {
      lv_obj_set_pos(page3_info_panel_, 12, 270);
      lv_obj_set_size(page3_info_panel_, 296, 120);
      make_transparent(page3_info_panel_);
    }
    lv_obj_clear_flag(page3_info_panel_, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(page3_info_panel_, LV_OBJ_FLAG_SCROLLABLE);
    enable_touch_bubble(page3_info_panel_);
  } else {
    page3_shell_ = nullptr;
    page3_info_panel_ = nullptr;
  }

  page3_image_ = lv_image_create(page3_);
  lv_obj_set_size(page3_image_, kOnxUiLayout ? kOnxCameraImageWidth : board::kDisplayWidth,
                  kOnxUiLayout ? kOnxCameraImageHeight : kPage3CameraHeight);
  lv_image_set_inner_align(page3_image_,
                           kOnxUiLayout ? LV_IMAGE_ALIGN_CONTAIN : LV_IMAGE_ALIGN_CENTER);
  if (kOnxUiLayout) {
    lv_obj_set_pos(page3_image_, 12, kOnxLandscapeLayout ? 48 : 86);
  } else {
    lv_obj_align(page3_image_, LV_ALIGN_CENTER, 0, kPage3CameraYOffset);
  }
  lv_obj_add_flag(page3_image_, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(page3_image_, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(page3_image_, LV_OBJ_FLAG_SCROLLABLE);
  enable_touch_bubble(page3_image_);

  page3_note_ = lv_label_create(page3_);
  set_label_text_if_changed(page3_note_, "Tap for new image");
  lv_obj_set_width(page3_note_, kOnxLandscapeLayout ? 256 : (kOnxUiLayout ? 180 : 320));
  lv_label_set_long_mode(page3_note_, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(page3_note_, kOnxUiLayout ? LV_TEXT_ALIGN_LEFT : LV_TEXT_ALIGN_CENTER,
                              0);
  // Match the main-page status label (dosis32 / white) so the lifecycle status
  // line on the camera page reads consistently across pages.
  lv_obj_set_style_text_font(page3_note_, kOnxUiLayout ? info20 : dosis32, 0);
  lv_obj_set_style_text_color(page3_note_,
                              lv_color_hex(kOnxUiLayout ? kOnxColorSoft : 0xFFFFFF), 0);
  if (kOnxUiLayout) {
    lv_obj_set_pos(page3_note_, kOnxLandscapeLayout ? 34 : 30,
                   kOnxLandscapeLayout ? 136 : 154);
    lv_obj_set_size(page3_note_, kOnxLandscapeLayout ? 256 : 260, 48);
    lv_obj_set_width(page3_note_, kOnxLandscapeLayout ? 256 : 260);
    lv_obj_set_style_text_align(page3_note_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_bg_opa(page3_note_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(page3_note_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_hor(page3_note_, 0, 0);
    lv_obj_set_style_pad_ver(page3_note_, 0, 0);
  } else {
    lv_obj_align(page3_note_, LV_ALIGN_CENTER, 0, 0);
  }
  enable_touch_bubble(page3_note_);

  page3_subnote_ = lv_label_create(kOnxUiLayout ? page3_info_panel_ : page3_);
  set_label_text_if_changed(page3_subnote_, "Auto-refresh every 2s");
  lv_obj_set_width(page3_subnote_, kOnxLandscapeLayout ? 120 : (kOnxUiLayout ? 276 : 320));
  lv_label_set_long_mode(page3_subnote_,
                         kOnxLandscapeLayout ? LV_LABEL_LONG_DOT : LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(page3_subnote_,
                              kOnxUiLayout ? LV_TEXT_ALIGN_LEFT : LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(page3_subnote_,
                             kOnxLandscapeLayout ? &lv_font_montserrat_14 : info20, 0);
  lv_obj_set_style_text_color(page3_subnote_,
                              lv_color_hex(kOnxUiLayout ? kOnxColorSoft : 0x888888), 0);
  if (kOnxUiLayout) {
    lv_obj_set_pos(page3_subnote_, kOnxLandscapeLayout ? 12 : 0,
                   kOnxLandscapeLayout ? 90 : 0);
    if (kOnxLandscapeLayout) {
      lv_obj_set_size(page3_subnote_, 120, 28);
    }
  } else {
    lv_obj_align(page3_subnote_, LV_ALIGN_CENTER, 0, 28);
  }
  lv_obj_add_flag(page3_subnote_, LV_OBJ_FLAG_HIDDEN);
  enable_touch_bubble(page3_subnote_);

  if (kOnxUiLayout) {
    page3_info_hint_ = lv_label_create(page3_info_panel_);
    set_label_text_if_changed(page3_info_hint_,
                              kOnxLandscapeLayout ? "Tap image" :
                              "Auto-refresh while open");
    lv_obj_set_pos(page3_info_hint_, kOnxLandscapeLayout ? 12 : 0,
                   kOnxLandscapeLayout ? 198 : 42);
    lv_obj_set_size(page3_info_hint_, kOnxLandscapeLayout ? 120 : 296,
                    kOnxLandscapeLayout ? 20 : LV_SIZE_CONTENT);
    lv_obj_set_width(page3_info_hint_, kOnxLandscapeLayout ? 120 : 296);
    lv_label_set_long_mode(page3_info_hint_, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(page3_info_hint_,
                               kOnxLandscapeLayout ? &lv_font_montserrat_14 : info20, 0);
    lv_obj_set_style_text_color(page3_info_hint_,
                                lv_color_hex(kOnxLandscapeLayout ? kOnxColorCyan :
                                             kOnxColorMuted), 0);

    page3_refresh_label_ = lv_label_create(page3_info_panel_);
    set_label_text_if_changed(page3_refresh_label_,
                              kOnxLandscapeLayout ? "Auto 2s" :
                              "Auto-refresh while open");
    lv_obj_set_pos(page3_refresh_label_, kOnxLandscapeLayout ? 12 : 0,
                   kOnxLandscapeLayout ? 156 : 42);
    lv_obj_set_size(page3_refresh_label_, kOnxLandscapeLayout ? 120 : 296,
                    kOnxLandscapeLayout ? 20 : LV_SIZE_CONTENT);
    lv_label_set_long_mode(page3_refresh_label_, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(page3_refresh_label_,
                               kOnxLandscapeLayout ? &lv_font_montserrat_14 : info20, 0);
    lv_obj_set_style_text_color(page3_refresh_label_, lv_color_hex(kOnxColorMuted), 0);
    if (!kOnxLandscapeLayout) {
      lv_obj_add_flag(page3_refresh_label_, LV_OBJ_FLAG_HIDDEN);
    }
    enable_touch_bubble(page3_refresh_label_);

    page3_source_label_ = lv_label_create(page3_info_panel_);
    set_label_text_if_changed(page3_source_label_,
                              kOnxLandscapeLayout ? "Source: waiting" :
                              "Source: local JPEG snapshot");
    lv_obj_set_pos(page3_source_label_, kOnxLandscapeLayout ? 12 : 0,
                   kOnxLandscapeLayout ? 124 : 78);
    lv_obj_set_size(page3_source_label_, kOnxLandscapeLayout ? 120 : 296,
                    kOnxLandscapeLayout ? 20 : LV_SIZE_CONTENT);
    lv_obj_set_width(page3_source_label_, kOnxLandscapeLayout ? 120 : 296);
    lv_label_set_long_mode(page3_source_label_,
                           LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(page3_source_label_,
                               kOnxLandscapeLayout ? &lv_font_montserrat_14 : info20, 0);
    lv_obj_set_style_text_color(page3_source_label_, lv_color_hex(kOnxColorMuted), 0);
  }

  lv_obj_add_event_cb(screen_, &Ui::screen_event_cb, LV_EVENT_ALL, this);
  lv_obj_update_layout(pager_);
  lv_obj_scroll_to_view(page1_, LV_ANIM_OFF);

  active_page_ = kPageIdxMain;
  scrolling_ = false;
  deferred_snapshot_pending_ = false;
  detail_visible_ = true;
  show_logo_ = false;
  preview_text_image_mode_ = false;
  camera_text_image_mode_ = false;
  apply_page_visibility();
  update_portal_access_visuals_locked();

  return ESP_OK;
}

void Ui::apply_page_visibility() {
  // Page content labels are children of their respective page objects which live
  // inside the horizontally-scrolling pager.  LVGL clips children that are
  // outside the visible viewport, so we do NOT need to hide page-1/2/3 content
  // during a scroll gesture.  Keeping them visible produces a smooth gallery-
  // like swipe transition where the departing and arriving pages render side by
  // side while the user's finger drags.
  const bool on_page1 = !scrolling_ ? (active_page_ == kPageIdxMain) : true;
  const bool on_page2 = !scrolling_ ? (active_page_ == kPageIdxPreview) : true;
  const bool on_page3 = !scrolling_ ? (active_page_ == kPageIdxCamera) : true;
  const bool settled_page1 = !scrolling_ && active_page_ == kPageIdxMain;
  const bool portal_hint_has_priority = portal_pin_active_ || portal_session_active_;
  const bool show_portal_hint =
      settled_page1 && !portal_hint_text_.empty() &&
      (kOnxLandscapeLayout || portal_hint_has_priority || !detail_visible_);

  for (int u = 0; u < kMaxAmsUnits; ++u) {
    if (ams_pages_[u] != nullptr) {
      set_hidden(ams_pages_[u], !ams_unit_present_[u]);
    }
  }
  set_hidden(page2_, !preview_page_available_);
  set_hidden(page3_, !camera_page_available_);
  set_hidden(status_label_, !on_page1);
  set_hidden(detail_label_, !on_page1 || !detail_visible_ ||
                            (show_portal_hint && !kOnxLandscapeLayout));
  set_hidden(layer_label_, !on_page1);
  set_hidden(nozzle_prefix_label_, !on_page1);
  set_hidden(nozzle_value_label_, !on_page1);
  set_hidden(nozzle_aux_label_, !on_page1 || !nozzle_aux_visible_ || kOnxLandscapeLayout);
  set_hidden(bed_prefix_label_, !on_page1);
  set_hidden(bed_value_label_, !on_page1);
  set_hidden(bed_aux_label_, !on_page1 || !bed_aux_visible_ || kOnxLandscapeLayout);
  set_hidden(remaining_row_, !on_page1);
  set_hidden(badge_slot_, !on_page1);
  set_hidden(portal_hint_label_, !show_portal_hint);
  set_hidden(page2_image_, !on_page2 || !preview_image_visible_);
  set_hidden(page3_image_, !on_page3 || !camera_image_visible_);

  if (kOnxUiLayout) {
    set_hidden(fixed_overlay_, true);
    set_hidden(status_arc_, true);
    set_hidden(progress_label_, !on_page1);
    set_hidden(onx_main_topbar_, !on_page1);
    set_hidden(onx_top_status_label_, !on_page1);
    set_hidden(onx_job_label_, !on_page1);
    set_hidden(onx_progress_panel_, !on_page1);
    set_hidden(onx_progress_bar_, !on_page1);
    set_hidden(onx_nozzle_card_, !on_page1);
    set_hidden(onx_bed_card_, !on_page1);
    set_hidden(onx_layer_card_, !on_page1);
    set_hidden(onx_detail_card_, !on_page1);
    set_hidden(onx_layer_line_label_, !on_page1);
    set_hidden(onx_metric_detail_label_, !on_page1);
    set_hidden(nozzle_prefix_label_, true);
    set_hidden(bed_prefix_label_, true);
  }

  // Overlay + page0 opacity are driven by apply_page0_parallax.
  // When not scrolling, snap to final state.
  if (!scrolling_) {
    apply_page0_parallax(true);
  }

  apply_logo_visibility();
  update_portal_access_visuals_locked();
}

void Ui::apply_logo_visibility() {
  // badge_slot_ is a child of page1_ so it clips naturally during scroll.
  const bool show_badge_slot = scrolling_ || active_page_ == kPageIdxMain;
  if (!show_badge_slot) {
    set_hidden(logo_badge_, true);
    return;
  }

  set_hidden(logo_badge_, !show_logo_);
}

void Ui::update_page_availability_locked(const PrinterSnapshot& snapshot) {
  const bool preview_available = snapshot.preview_page_available;
  const bool camera_available = snapshot.camera_page_available;
  const uint8_t ams_count = snapshot.ams ? snapshot.ams->count : 0;
  bool ams_changed = false;
  for (int u = 0; u < kMaxAmsUnits; ++u) {
    const bool present = u < ams_count;
    if (ams_unit_present_[u] != present) {
      ams_unit_present_[u] = present;
      ams_changed = true;
    }
  }
  const bool availability_changed =
      preview_page_available_ != preview_available || camera_page_available_ != camera_available ||
      ams_changed;

  preview_page_available_ = preview_available;
  camera_page_available_ = camera_available;

  if (!availability_changed) {
    return;
  }

  for (int u = 0; u < kMaxAmsUnits; ++u) {
    if (ams_pages_[u] != nullptr) {
      set_hidden(ams_pages_[u], !ams_unit_present_[u]);
    }
  }
  set_hidden(page2_, !preview_page_available_);
  set_hidden(page3_, !camera_page_available_);

  if (!preview_page_available_) {
    release_preview_image_locked();
    preview_image_visible_ = false;
  }

  if (!camera_page_available_) {
    if (camera_slot_initialized_ && page3_image_ != nullptr) {
      lv_image_set_src(page3_image_, nullptr);
    }
    lv_image_cache_drop(&camera_image_dscs_[0]);
    lv_image_cache_drop(&camera_image_dscs_[1]);
    camera_blobs_[0].reset();
    camera_blobs_[1].reset();
    camera_slot_initialized_ = false;
    active_camera_slot_ = 0;
    last_camera_width_ = 0;
    last_camera_height_ = 0;
    std::memset(&camera_image_dscs_[0], 0, sizeof(camera_image_dscs_[0]));
    std::memset(&camera_image_dscs_[1], 0, sizeof(camera_image_dscs_[1]));
    camera_image_visible_ = false;
  }

  active_page_ = clamp_enabled_page(active_page_);
  lv_obj_update_layout(pager_);
  if (lv_obj_t* target_page = page_object(active_page_); target_page != nullptr) {
    lv_obj_scroll_to_view(target_page, LV_ANIM_OFF);
  }
  scrolling_ = false;
  apply_page0_parallax(true);
  // Ring timer resume is handled by apply_ring_visual_locked via apply_snapshot.
}

bool Ui::page_enabled(int page) const {
  if (page == kPageIdxPrinterSelect) {
    return true;
  }
  if (page >= kPageIdxAmsFirst && page <= kPageIdxAmsLast) {
    return ams_unit_present_[page - kPageIdxAmsFirst];
  }
  if (page == kPageIdxMain) {
    return true;
  }
  if (page == kPageIdxPreview) {
    return preview_page_available_;
  }
  if (page == kPageIdxCamera) {
    return camera_page_available_;
  }
  return false;
}

lv_obj_t* Ui::page_object(int page) const {
  if (page == kPageIdxPrinterSelect) {
    return page0_;
  }
  if (page >= kPageIdxAmsFirst && page <= kPageIdxAmsLast) {
    return ams_pages_[page - kPageIdxAmsFirst];
  }
  if (page == kPageIdxMain) {
    return page1_;
  }
  if (page == kPageIdxPreview) {
    return page2_;
  }
  if (page == kPageIdxCamera) {
    return page3_;
  }
  return nullptr;
}

int Ui::next_enabled_page(int page, int direction) const {
  int candidate = page + direction;
  while (candidate >= 0 && candidate <= kPageIdxLast) {
    if (page_enabled(candidate)) {
      return candidate;
    }
    candidate += direction;
  }
  return page;
}

int Ui::clamp_enabled_page(int page) const {
  if (page_enabled(page)) {
    return page;
  }

  for (int candidate = 0; candidate <= kPageIdxLast; ++candidate) {
    if (page_enabled(candidate)) {
      return candidate;
    }
  }

  return 0;
}

int Ui::nearest_enabled_page_for_scroll() const {
  lv_obj_update_layout(pager_);
  int scroll_x = lv_obj_get_scroll_x(pager_);
  if (scroll_x < 0) {
    scroll_x = -scroll_x;
  }

  // With LV_SCROLL_SNAP_NONE + scroll_throw=90, the throw decays in ~3 ticks so
  // scroll_x already reflects the post-throw resting position when SCROLL_END fires.
  // Simply pick whichever page center is closest to the viewport center.
  const int viewport_center = scroll_x + (board::kDisplayWidth / 2);
  int best_page = clamp_enabled_page(active_page_);
  int best_distance = INT32_MAX;

  for (int page = 0; page <= kPageIdxLast; ++page) {
    if (!page_enabled(page)) {
      continue;
    }

    lv_obj_t* object = page_object(page);
    if (object == nullptr) {
      continue;
    }

    const int page_center = lv_obj_get_x(object) + (board::kDisplayWidth / 2);
    const int distance = std::abs(page_center - viewport_center);
    if (distance < best_distance) {
      best_distance = distance;
      best_page = page;
    }
  }

  return best_page;
}

void Ui::set_active_page(int page) {
  const int clamped_page = clamp_enabled_page(page);
  const int previous_page = active_page_;
  lv_obj_update_layout(pager_);
  if (lv_obj_t* target_page = page_object(clamped_page); target_page != nullptr) {
    // lv_obj_scroll_to_view can leave a small residual offset depending on
    // scroll direction.  Use the page's exact x-position for a pixel-perfect snap.
    const int target_x = lv_obj_get_x(target_page);
    pager_snap_in_progress_ = true;
    lv_obj_scroll_to_x(pager_, target_x, LV_ANIM_OFF);
    pager_snap_in_progress_ = false;
  }
  active_page_ = clamped_page;
  if (clamped_page == 0 && previous_page != 0) {
    replay_card_animations_locked();
  }
  if (active_page_ == kPageIdxPreview) {
    preview_image_visible_ = ensure_preview_image_loaded_locked(false);
  }
  scrolling_ = false;
  apply_page0_parallax(true);
  // Ring timer resume is handled by apply_ring_visual_locked below.
  apply_page_visibility();
  if (deferred_snapshot_pending_) {
    apply_snapshot_locked(deferred_snapshot_, true);
  } else if (previous_page != clamped_page) {
    apply_snapshot_locked(last_snapshot_, true);
  }
}

void Ui::schedule_pager_snap(int page) {
  pending_pager_snap_page_ = clamp_enabled_page(page);
  if (pager_snap_timer_ != nullptr) {
    lv_timer_delete(pager_snap_timer_);
    pager_snap_timer_ = nullptr;
  }
  pager_snap_timer_ = lv_timer_create(&Ui::pager_snap_timer_cb, 1, this);
}

void Ui::set_pager_scroll_locked(bool locked) {
  if (pager_ == nullptr || pager_scroll_locked_ == locked) {
    return;
  }

  pager_scroll_locked_ = locked;
  lv_obj_set_scroll_dir(pager_, (locked || kOnxUiLayout) ? LV_DIR_NONE : LV_DIR_HOR);

  if (!locked) {
    return;
  }

  // Snap back to the currently active page as soon as brightness control wins
  // the gesture, so a slightly diagonal drag doesn't leave the pager half-way
  // between pages.
  lv_obj_update_layout(pager_);
  if (lv_obj_t* target_page = page_object(active_page_); target_page != nullptr) {
    const int target_x = lv_obj_get_x(target_page);
    lv_obj_scroll_to_x(pager_, target_x, LV_ANIM_OFF);
  }
  scrolling_ = false;
  apply_page0_parallax(true);
  apply_page_visibility();
}

void Ui::handle_pager_event(lv_event_t* event) {
  const lv_event_code_t code = lv_event_get_code(event);

  if (pager_scroll_locked_ || pager_snap_in_progress_) {
    return;
  }

  if (code == LV_EVENT_SCROLL) {
    apply_page0_parallax();
    return;
  }

  if (code != LV_EVENT_SCROLL_BEGIN && code != LV_EVENT_SCROLL_END) {
    return;
  }

  if (code == LV_EVENT_SCROLL_BEGIN) {
    scrolling_ = true;

    apply_page_visibility();
    return;
  }

  int scroll_x = lv_obj_get_scroll_x(pager_);
  if (scroll_x < 0) {
    scroll_x = -scroll_x;
  }

  (void)scroll_x;
  schedule_pager_snap(nearest_enabled_page_for_scroll());
}

void Ui::handle_screen_event(lv_event_t* event) {
  const lv_event_code_t code = lv_event_get_code(event);
  if (code != LV_EVENT_PRESSED && code != LV_EVENT_PRESSING && code != LV_EVENT_RELEASED &&
      code != LV_EVENT_PRESS_LOST && code != LV_EVENT_LONG_PRESSED) {
    return;
  }

  lv_indev_t* indev = lv_indev_get_act();
  if (indev == nullptr) {
    return;
  }

  lv_point_t point = {};
  lv_indev_get_point(indev, &point);

  if (code == LV_EVENT_PRESSED) {
    set_pager_scroll_locked(false);
    if (screen_power_mode_ == ScreenPowerMode::kOff) {
      // First touch wakes the screen; a second touch performs UI actions.
      note_activity(true);
      gesture_active_ = false;
      swipe_switched_ = false;
      overlay_visible_ = false;
      return;
    }

    note_activity(false);
    gesture_active_ = true;
    swipe_switched_ = false;
    overlay_visible_ = false;
    gesture_start_x_ = point.x;
    gesture_start_y_ = point.y;
    gesture_start_brightness_ = user_brightness_percent_;
    return;
  }

  if (code == LV_EVENT_PRESSING && gesture_active_) {
    note_activity(false);
    const int dx = static_cast<int>(point.x - gesture_start_x_);
    const int dy = static_cast<int>(gesture_start_y_ - point.y);
    const int abs_dx = std::abs(dx);
    const int abs_dy = std::abs(dy);

    if (swipe_switched_) {
      return;
    }

    if (!overlay_visible_) {
      // Resolve the gesture once: either a horizontal page swipe or a mostly
      // vertical brightness drag. This avoids accidental brightness changes
      // while the finger is moving diagonally during page navigation.
      const bool horizontal_swipe =
          abs_dx >= kSwipeThresholdPx &&
          abs_dx >= (abs_dy - kGestureAxisLockMarginPx);
      const bool vertical_brightness =
          abs_dy >= kSwipeThresholdPx &&
          abs_dx <= kBrightnessHorizontalTolerancePx &&
          abs_dy >= (abs_dx + kGestureAxisLockMarginPx);

      if (horizontal_swipe) {
        swipe_switched_ = true;
        if (kOnxUiLayout) {
          const int direction = dx < 0 ? 1 : -1;
          schedule_pager_snap(next_enabled_page(active_page_, direction));
        }
        return;
      }
      if (!vertical_brightness) {
        return;
      }

      set_pager_scroll_locked(true);
    }

    const float delta = static_cast<float>(dy) * (100.0f / 250.0f);
    const int new_brightness =
        std::clamp(gesture_start_brightness_ + static_cast<int>(std::lround(delta)),
                   kManualMinBrightnessPercent, 100);
    set_brightness_percent(new_brightness);

    char buffer[8] = {};
    std::snprintf(buffer, sizeof(buffer), "%d%%", user_brightness_percent_);
    set_label_text_if_changed(brightness_overlay_, buffer);
    lv_obj_clear_flag(brightness_overlay_, LV_OBJ_FLAG_HIDDEN);
    overlay_visible_ = true;
    return;
  }

  if (code == LV_EVENT_LONG_PRESSED && gesture_active_) {
    note_activity(false);
    if (!overlay_visible_ && !scrolling_ && !swipe_switched_) {
      portal_unlock_requested_.store(true);
    }
    return;
  }

  if ((code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) && gesture_active_) {
    note_activity(false);
    const int dx = static_cast<int>(point.x - gesture_start_x_);
    const int dy = static_cast<int>(gesture_start_y_ - point.y);
    const int abs_dx = std::abs(dx);
    const int abs_dy = std::abs(dy);
    const bool swipe_locked = swipe_switched_;

    gesture_active_ = false;
    swipe_switched_ = false;
    set_pager_scroll_locked(false);
    if (overlay_visible_) {
      lv_obj_add_flag(brightness_overlay_, LV_OBJ_FLAG_HIDDEN);
      overlay_visible_ = false;
      return;
    }
    if (swipe_locked) {
      return;
    }

    // Horizontal page swiping is handled by the LVGL pager (flex-row +
    // LV_SCROLL_SNAP_NONE → handle_pager_event snaps to nearest page on SCROLL_END).
    // Only handle taps here (camera refresh on page 3).
    if (active_page_ == kPageIdxCamera && camera_page_available_ && abs_dx < 12 && abs_dy < 12) {
      std::lock_guard<std::mutex> lock(camera_refresh_mutex_);
      camera_refresh_requested_ = true;
    }
  }
}

void Ui::update_portal_access_visuals_locked() {
  const bool show_page1 = !scrolling_ && active_page_ == kPageIdxMain;
  const bool portal_hint_has_priority = portal_pin_active_ || portal_session_active_;
  const bool show_hint = portal_hint_label_ != nullptr && show_page1 && !portal_hint_text_.empty() &&
                         (kOnxLandscapeLayout || portal_hint_has_priority || !detail_visible_);
  set_hidden(portal_hint_label_, !show_hint);
  if (show_hint) {
    if (kOnxUiLayout) {
      lv_obj_set_size(portal_hint_label_, kOnxLandscapeLayout ? 456 : 296,
                      kOnxLandscapeLayout ? 20 : 16);
      lv_label_set_long_mode(portal_hint_label_, LV_LABEL_LONG_DOT);
      lv_obj_align(portal_hint_label_, LV_ALIGN_TOP_LEFT,
                   12, kOnxLandscapeLayout ? 292 : 462);
    }
    set_label_text_if_changed(portal_hint_label_, portal_hint_text_);
    if (!kOnxLandscapeLayout) {
      set_hidden(detail_label_, true);
    }
  } else if (detail_label_ != nullptr && show_page1 && detail_visible_) {
    set_hidden(detail_label_, false);
  }

  const bool show_overlay = portal_overlay_card_ != nullptr && portal_pin_active_;
  set_hidden(portal_overlay_card_, !show_overlay);
  if (!show_overlay) {
    return;
  }

  set_label_text_if_changed(portal_overlay_title_, portal_overlay_title_text_);
  set_label_text_if_changed(portal_overlay_value_, portal_overlay_value_text_);
  set_label_text_if_changed(portal_overlay_detail_, portal_overlay_detail_text_);
}

void Ui::handle_logo_event(lv_event_t* event) {
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
    return;
  }

  if (scrolling_ || !show_logo_ || !last_snapshot_.chamber_light_supported) {
    return;
  }

  note_activity(false);
  chamber_light_toggle_requested_.store(true);
}

void Ui::set_brightness_percent(int brightness_percent) {
  const int clamped = std::clamp(brightness_percent, 0, 100);
  if (user_brightness_percent_ == clamped) {
    return;
  }

  user_brightness_percent_ = clamped;
  apply_brightness_policy();
}

void Ui::note_activity(bool wake_display_now) {
  last_activity_tick_ms_.store(lv_tick_get());
  if (wake_display_now) {
    wake_display();
  }
}

void Ui::wake_display() {
  if (screen_power_mode_ == ScreenPowerMode::kAwake) {
    return;
  }

  const bool was_off = screen_power_mode_ == ScreenPowerMode::kOff;
  screen_power_mode_ = ScreenPowerMode::kAwake;
  apply_brightness_policy();
  if (was_off) {
    bsp_display_resume();
  }
}

void Ui::request_wake_display() {
  note_activity(true);
}

void Ui::set_battery_display_policy(const BatteryDisplayPolicy& policy) {
  battery_display_policy_ = policy;
}

void Ui::apply_brightness_policy() {
  int target_brightness = user_brightness_percent_;
  if (screen_power_mode_ == ScreenPowerMode::kDimmed) {
    if (battery_display_policy_.dim_brightness_percent > 0) {
      target_brightness = std::clamp(battery_display_policy_.dim_brightness_percent, 1, 100);
    } else {
      target_brightness = std::max(8, std::min(18, std::max(1, user_brightness_percent_ / 3)));
    }
  } else if (screen_power_mode_ == ScreenPowerMode::kOff) {
    target_brightness = 0;
  }

  if (applied_brightness_percent_ == target_brightness) {
    return;
  }

  applied_brightness_percent_ = target_brightness;
  bsp_display_brightness_set(target_brightness);
}

void Ui::update_power_save(bool on_battery, bool print_active) {
  const uint32_t now = lv_tick_get();
  const uint32_t idle_ms = now - last_activity_tick_ms_.load();

  // While the LVGL worker is paused (screen off), LVGL touch events are not
  // processed.  Poll the raw touch-interrupt GPIO so a finger press can still
  // wake the display.  CST9217 pulls INT (GPIO 11) low on contact.
  if (screen_power_mode_ == ScreenPowerMode::kOff && raw_touch_irq_asserted()) {
    note_activity(true);  // updates last_activity_tick_ms_ + calls wake_display()
    return;               // re-evaluate on next call with fresh idle_ms
  }

  const uint32_t dim_timeout = (print_active
      ? battery_display_policy_.dim_timeout_active_s
      : battery_display_policy_.dim_timeout_idle_s) * 1000U;
  const uint32_t off_timeout = (print_active
      ? battery_display_policy_.off_timeout_active_s
      : battery_display_policy_.off_timeout_idle_s) * 1000U;

  ScreenPowerMode target_mode = ScreenPowerMode::kAwake;
  if (on_battery || battery_display_policy_.usb_power_save_enabled) {
    if (battery_display_policy_.screen_off_enabled && idle_ms >= off_timeout) {
      target_mode = ScreenPowerMode::kOff;
    } else if (battery_display_policy_.dim_enabled && idle_ms >= dim_timeout) {
      target_mode = ScreenPowerMode::kDimmed;
    }
  }

  if (screen_power_mode_ != target_mode) {
    const bool was_off = screen_power_mode_ == ScreenPowerMode::kOff;
    const bool going_off = target_mode == ScreenPowerMode::kOff;
    screen_power_mode_ = target_mode;

    // IMPORTANT: When turning the screen off, pause the LVGL worker BEFORE
    // setting brightness to 0.  The AMOLED panel stops generating the TE
    // signal on GPIO13 at brightness 0.  If lv_timer_handler() is in the
    // flush path it will block forever on xSemaphoreTake(te_vsync_sem,
    // portMAX_DELAY).  Pausing first ensures the worker finishes its current
    // render cycle (while TE is still active) and then idles.
    //
    // If pause times out (worker stuck in flush), we must NOT kill the TE
    // signal — that would permanently deadlock the worker.  Instead, abort
    // the screen-off transition and stay in the current power mode.
    if (going_off && !was_off) {
      esp_err_t pause_ret = bsp_display_pause(1000);
      if (pause_ret != ESP_OK) {
        ESP_LOGW(kTag, "LVGL worker pause timeout — aborting screen-off to avoid TE deadlock (%s)",
                 esp_err_to_name(pause_ret));
        // Treat a failed screen-off attempt like fresh activity so we don't
        // immediately hammer pause() again on the next main-loop iteration.
        last_activity_tick_ms_.store(now);
        screen_power_mode_ = was_off ? ScreenPowerMode::kOff : ScreenPowerMode::kAwake;
        return;
      }
    }

    apply_brightness_policy();

    if (was_off && !going_off) {
      bsp_display_resume();
    }
  }
}

bool Ui::is_low_power_mode_active() const {
  return screen_power_mode_ != ScreenPowerMode::kAwake;
}

void Ui::pager_event_cb(lv_event_t* event) {
  auto* ui = static_cast<Ui*>(lv_event_get_user_data(event));
  if (ui != nullptr) {
    ui->handle_pager_event(event);
  }
}

void Ui::pager_snap_timer_cb(lv_timer_t* timer) {
  auto* ui = static_cast<Ui*>(lv_timer_get_user_data(timer));
  if (ui == nullptr) {
    lv_timer_delete(timer);
    return;
  }

  const int page = ui->pending_pager_snap_page_;
  ui->pending_pager_snap_page_ = -1;
  if (ui->pager_snap_timer_ == timer) {
    ui->pager_snap_timer_ = nullptr;
  }
  lv_timer_delete(timer);

  if (page >= 0) {
    ui->set_active_page(page);
  }
}

void Ui::screen_event_cb(lv_event_t* event) {
  auto* ui = static_cast<Ui*>(lv_event_get_user_data(event));
  if (ui != nullptr) {
    ui->handle_screen_event(event);
  }
}

void Ui::logo_event_cb(lv_event_t* event) {
  auto* ui = static_cast<Ui*>(lv_event_get_user_data(event));
  if (ui != nullptr) {
    ui->handle_logo_event(event);
  }
}

void Ui::remaining_row_event_cb(lv_event_t* event) {
  auto* ui = static_cast<Ui*>(lv_event_get_user_data(event));
  if (ui != nullptr) {
    ui->handle_remaining_row_click();
  }
}

void Ui::handle_remaining_row_click() {
  if (scrolling_) {
    return;
  }
  if (kOnxLandscapeLayout) {
    return;
  }
  // We are inside an LVGL event \u2014 the lvgl_port task already holds the
  // display lock, so no extra locking is needed before mutating widgets.
  show_eta_ = !show_eta_;
  // Re-apply the cached snapshot so the row updates immediately.
  apply_snapshot_locked(last_snapshot_, false);
}

}  // namespace printsphere
