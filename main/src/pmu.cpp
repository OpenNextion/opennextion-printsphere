#include "printsphere/pmu.hpp"

#include "esp_log.h"

namespace printsphere {

namespace {
constexpr char kTag[] = "printsphere.pmu";
}  // namespace

esp_err_t PmuManager::initialize() {
  if (initialized_) {
    return ESP_OK;
  }

  initialized_ = true;
  ESP_LOGW(kTag, "ONX profile has no AXP2101 PMU; power snapshot unavailable");
  return ESP_OK;
}

PowerSnapshot PmuManager::sample() const {
  return PowerSnapshot{};
}

}  // namespace printsphere
