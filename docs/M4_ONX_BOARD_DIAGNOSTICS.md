# M4 ONX Board Diagnostics

This document defines the ONX3248G035 board-level diagnostics smoke. It is not a
PrintSphere application acceptance target and does not cover Wi-Fi, Bambu,
Web Config, OTA, camera, or long-duration stability.

## Firmware

- Project: `examples/onx_bsp_board_smoke`.
- Component: `components/onx3248g035_bsp`.
- Build mode: `IDF_COMPONENT_MANAGER=0`.
- Flash flow: standard `docs/BUILD_FLASH.md` only, from
  `examples/onx_bsp_board_smoke/build`, using `/dev/tty.wchusbserial10`,
  `115200`, and `write-flash @flash_args`.

## Interface Status

Implemented:

- I2C init through `onx_bsp_i2c_init()`, logging `I2C_NUM_0`, SDA `GPIO8`, and
  SCL `GPIO7`.
- PCF8574 init/read/write through existing `onx_bsp_pcf8574_*` APIs.
- PCF8574 `SDCS` policy: pin7 held high during diagnostics.
- Backlight PWM set/get through `onx_bsp_backlight_*`, logging `30 -> 100 -> 30`.
- Battery ADC raw read through `onx_bsp_battery_adc_read_raw()`, using GPIO4 /
  ADC1 channel 3.

Logged raw only:

- PCF8574 raw state and pin map:
  - pin1 `I2S_CTRL`
  - pin2 `RTC_INT`
  - pin3 `CHG_N`
  - pin4 `CAM_PWDN`
  - pin5 `TP_INT`
  - pin6 `LCD_RST`
  - pin7 `SDCS`
- `CHG_N`: raw level only; low-active charging assumption is pending validation.
  The diagnostics firmware must not print `charging=true` or `charging=false`.
- `TP_INT`: raw PCF8574 pin5 level only; CST826 touch remains polling-based.
- Battery ADC: raw ADC code and uncalibrated ADC-pad millivolt estimate only.
  Divider ratio, calibration, and battery percentage are deferred.

Deferred:

- SD mount. The diagnostics smoke keeps `SDCS` high and does not mount a card.
  A later SD task should classify mount failures as no-card, driver, pin, or
  unknown without affecting display/touch smoke.
- Battery calibration, divider ratio, fuel-gauge percentage, and final charging
  semantics.
- Interrupt-driven touch through `TP_INT`.

## LCD Constraint

This diagnostics target does not change the current LCD candidate state recorded
in `docs/ONX3248G035_HARDWARE_CONFIG.md`. It must not use UI mirroring, color
constant swaps, coordinate offsets, or CASET/RASET gaps to compensate for
display issues. Visual LCD validation remains with `examples/onx_bsp_smoke` and
`examples/onx_bsp_lvgl_smoke`.

## 2026-06-02 Hardware Validation

Build and flash:

- `IDF_COMPONENT_MANAGER=0 idf.py -C examples/onx_bsp_board_smoke build` passed.
- `onx_bsp_board_smoke.bin` size: `0x337c0`.
- Standard flash from `examples/onx_bsp_board_smoke/build` passed using
  `/dev/tty.wchusbserial10`, `115200`, `--before default-reset --after
  hard-reset`, and `write-flash @flash_args`.
- Bootloader, partition table, and app image hashes were verified; the board
  hard-reset via RTS.

Captured runtime evidence:

- Firmware start: `ONX3248G035 board diagnostics smoke start`.
- I2C: `I2C bus ready evidence: port=0 SDA=GPIO8 SCL=GPIO7`.
- PCF8574: `addr=0x38`, initial raw state `0xFB`.
- Pin map:
  - pin1 `I2S_CTRL=1`
  - pin2 `RTC_INT=1`
  - pin3 `CHG_N=0`
  - pin4 `CAM_PWDN=1`
  - pin5 `TP_INT=1`
  - pin6 `LCD_RST=1`
  - pin7 `SDCS=1`
- `CHG_N`: raw level `0`; still raw-only, active-low charging semantics remain
  pending validation and no charging boolean is reported.
- `TP_INT`: raw level `1`; touch interrupt remains raw-only and CST826 touch
  remains polling-based.
- `SDCS`: raw level `1`; SD chip-select policy is high/not-selected and SD
  mount was not attempted.
- Battery ADC raw-only examples:
  - initial `raw=3563`, `adc_mv_uncalibrated=2871`.
  - repeat values observed around `raw=3555..3589`,
    `adc_mv_uncalibrated=2864..2892`.
  These are ADC-pad estimates only; divider ratio, calibration, and battery
  percentage remain deferred.
- Backlight set/get sequence passed:
  - `set=30 get=30`
  - `set=100 get=100`
  - `set=30 get=30`
- Touch polling init completed: `CST826 addr=0x15 chip_id=0x11`; the diagnostic
  sample logged `points=0`, which is acceptable because this smoke is not a
  touch interaction test.
- Completion: `Board diagnostics smoke complete: raw-only battery/charge/TP_INT,
  SDCS high, no SD mount attempted`.
- Repeat loop evidence was captured, including `Board diagnostics still
  running`, repeated PCF8574 pin map, backlight state, and
  `Battery ADC repeat raw-only`.

Validation status:

- Implemented and hardware-logged: I2C bus init, PCF8574 init/read/write, SDCS
  high policy, backlight PWM set/get, battery ADC raw read, CST826 polling init.
- Logged raw only: `CHG_N`, `TP_INT`, PCF8574 pin state, ADC raw and
  uncalibrated ADC-pad millivolts.
- Deferred: SD mount, battery calibration/divider/fuel percentage, final charge
  semantics, and interrupt-driven touch.
