#pragma once

#include "esp_idf_version.h"
#include "esp_timer.h"

/*
 * esp_lvgl_adapter 0.4.3 gates esp_timer_stop_blocking() on IDF >= 6.0.0,
 * but ESP-IDF v6.0.1 does not export that API. Keep this shim scoped to the
 * affected local IDF version and compile target.
 */
#if ESP_IDF_VERSION == ESP_IDF_VERSION_VAL(6, 0, 1)
#ifndef esp_timer_stop_blocking
#define esp_timer_stop_blocking(timer, timeout_ticks) esp_timer_stop(timer)
#endif
#endif
