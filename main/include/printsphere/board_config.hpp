#pragma once

#include "driver/gpio.h"

namespace printsphere::board {

#if defined(PRINTSPHERE_BOARD_ONX3248G035) && PRINTSPHERE_BOARD_ONX3248G035
#if defined(PRINTSPHERE_ONX_ORIENTATION_LANDSCAPE) && PRINTSPHERE_ONX_ORIENTATION_LANDSCAPE
constexpr int kDisplayWidth = 480;
constexpr int kDisplayHeight = 320;
constexpr bool kDisplayLandscape = true;
constexpr char kDisplayOrientation[] = "landscape";
#else
#error "Unsupported ONX3248G035 orientation for the public build"
#endif
#else
#error "Unsupported PrintSphere board profile for the public build"
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
constexpr char kBoardName[] = "ONX3248G035";

}  // namespace printsphere::board
