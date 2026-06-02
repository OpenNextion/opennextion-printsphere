# ONX BSP Board Smoke

This firmware is a board-level diagnostics smoke for ONX3248G035. It does not
start PrintSphere, Wi-Fi, Bambu protocol code, Web Config, OTA, camera, LVGL, or
SD mounting.

It logs:

- I2C bus readiness on `I2C_NUM_0`, SDA `GPIO8`, SCL `GPIO7`.
- PCF8574 raw state and pin map:
  - pin1 `I2S_CTRL`
  - pin2 `RTC_INT`
  - pin3 `CHG_N`
  - pin4 `CAM_PWDN`
  - pin5 `TP_INT`
  - pin6 `LCD_RST`
  - pin7 `SDCS`
- `CHG_N` raw level only. The low-active charging assumption is pending
  validation; this smoke does not report a charging boolean.
- Battery ADC raw value from GPIO4 / ADC1 channel 3 and an uncalibrated ADC-pad
  millivolt estimate. Divider ratio and battery percentage are deferred.
- `TP_INT` raw level through PCF8574 while touch remains polling-based.
- `SDCS` held high. SD mount is deferred.
- Backlight set/get sequence `30 -> 100 -> 30`.

Build:

```sh
cd /Users/alex/Documents/codex_project/Nextion_project_PrintSphere
export IDF_TOOLS_PATH="$PWD/.tools/espressif"
export IDF_COMPONENT_MANAGER=0
. "$PWD/.tools/esp-idf-v6.0.1/export.sh"
idf.py -C examples/onx_bsp_board_smoke build
```

Flash only when this firmware is the assigned validation target, using
`docs/BUILD_FLASH.md` standard flow from `examples/onx_bsp_board_smoke/build`.
