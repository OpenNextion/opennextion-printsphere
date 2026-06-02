#pragma once

#include "driver/gpio.h"

namespace printsphere::board {

#if defined(PRINTSPHERE_BOARD_ONX3248G035) && PRINTSPHERE_BOARD_ONX3248G035
constexpr int kDisplayWidth = 320;
constexpr int kDisplayHeight = 480;
#elif defined(PRINTSPHERE_BOARD_WAVESHARE_AMOLED_1_75) && PRINTSPHERE_BOARD_WAVESHARE_AMOLED_1_75
constexpr int kDisplayWidth = 466;
constexpr int kDisplayHeight = 466;
#endif

constexpr gpio_num_t kI2cScl = GPIO_NUM_14;
constexpr gpio_num_t kI2cSda = GPIO_NUM_15;

constexpr gpio_num_t kTouchInterrupt = GPIO_NUM_11;
constexpr gpio_num_t kTouchReset = GPIO_NUM_40;

constexpr gpio_num_t kQspiClk = GPIO_NUM_38;
constexpr gpio_num_t kQspiData0 = GPIO_NUM_4;
constexpr gpio_num_t kQspiData1 = GPIO_NUM_5;
constexpr gpio_num_t kQspiData2 = GPIO_NUM_6;
constexpr gpio_num_t kQspiData3 = GPIO_NUM_7;

constexpr int kAxp2101Address = 0x34;
constexpr char kBoardName[] = "Waveshare ESP32-S3 AMOLED 1.75";

}  // namespace printsphere::board
