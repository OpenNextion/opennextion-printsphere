# M3 ONX BSP Integration

This document defines the hardware BSP boundary for starting PrintSphere on the
ONX3248G035 board after M2 smoke acceptance.

## Current Baseline

- Branch: `feature/onx-bsp-bringup`.
- Accepted smoke commit: `cfb058e Stabilize ONX BSP smoke flow and LCD config`.
- ONX smoke project: `examples/onx_bsp_smoke`.
- Verified hardware:
  - ST7796U SPI LCD, `320 x 480`.
  - CST826 touch over I2C.
  - GPIO6 LEDC backlight PWM.
  - PCF8574 reset/control expander at `0x38`.
  - I2C bus on SDA GPIO8 and SCL GPIO7.
- Final LCD config is recorded in `docs/ONX3248G035_HARDWARE_CONFIG.md`.

## Source BSP API Surface

The current PrintSphere application includes
`bsp/esp32_s3_touch_amoled_1_75.h` and uses these BSP APIs directly:

- `bsp_display_start_with_config()`.
- `bsp_display_rotation_set()`.
- `bsp_display_lock()`.
- `bsp_display_unlock()`.
- `bsp_display_brightness_set()`.
- `bsp_i2c_init()`.
- `bsp_i2c_get_handle()`.

The source Waveshare BSP also exposes:

- `bsp_display_new()`.
- `bsp_touch_new()`.
- `bsp_display_get_input_dev()`.
- `bsp_display_brightness_init()`.
- `bsp_display_brightness_get()`.
- `bsp_display_backlight_on()`.
- `bsp_display_backlight_off()`.
- `bsp_i2c_deinit()`.
- `bsp_sdcard_mount()`.
- `bsp_sdcard_unmount()`.
- audio and IO-expander APIs.

M3 only needs the APIs that are used by PrintSphere at boot plus the dependent
display/touch helper APIs used by the BSP implementation.

## ONX Compatibility Boundary

The ONX BSP component now exposes low-level handles needed by a future
PrintSphere-compatible adapter:

- `onx_bsp_i2c_get_handle()`: returns the initialized I2C master bus handle.
- `onx_bsp_lcd_get_io_handle()`: returns the initialized LCD SPI IO handle.
- `onx_bsp_backlight_get()`: returns the last requested LEDC brightness percent.

These are intentionally ONX-named primitives used by the temporary ONX
`bsp_*` compatibility layer. Build/Release must select exactly one local board
component for a firmware so the ONX and Waveshare implementations of the same
public `bsp_*` symbols are never linked together.

## LCD Configuration Constraint

Every M3 display path must preserve the verified ST7796U configuration from
`docs/ONX3248G035_HARDWARE_CONFIG.md`.

Required constants and commands:

- `ONX_LCD_MADCTL_VALUE = LCD_CMD_MX_BIT | LCD_CMD_BGR_BIT`, value `0x48`.
- `ONX_LCD_COLMOD_VALUE = 0x55`.
- `ONX_LCD_INVERT_CMD = LCD_CMD_INVOFF`.
- RGB565 payloads go through `lcd_pack_rgb565()` before SPI color transfer.
- `CASET` and `RASET` remain the standard portrait `320 x 480` address window
  without scan-direction offsets.

This is the accepted LCD fix. M3 must not reintroduce mirror, inversion, BGR/RGB,
or byte-order drift while adding `bsp_display_*`, LVGL, or panel-handle support.
If a future `esp_lcd_panel_handle_t` wrapper or official ST7796U panel component
is introduced, its init sequence must produce the same final panel state.

## Required M3 Compatible APIs

### I2C

Target behavior:

- `bsp_i2c_init()` maps to `onx_bsp_i2c_init()`.
- `bsp_i2c_get_handle()` maps to `onx_bsp_i2c_get_handle()`.
- `bsp_i2c_deinit()` can initially return `ESP_ERR_NOT_SUPPORTED` or no-op only
  if the app never calls it. The ONX smoke firmware does not require deinit.

Status:

- ONX I2C init and handle access are ready.
- The compatibility layer now exports `bsp_i2c_init()` and
  `bsp_i2c_get_handle()` from
  `components/onx3248g035_bsp/src/onx3248g035_bsp_compat.c`.
- `bsp_i2c_deinit()` returns `ESP_ERR_NOT_SUPPORTED` because the ONX bring-up
  path keeps the shared board I2C bus alive for PCF8574, CST826, and future
  board controls.

### Brightness and Backlight

Target behavior:

- `bsp_display_brightness_init()` maps to `onx_bsp_backlight_init()`.
- `bsp_display_brightness_set(percent)` maps to
  `onx_bsp_backlight_set(percent)`.
- `bsp_display_brightness_get()` maps to `onx_bsp_backlight_get()`.
- `bsp_display_backlight_on()` maps to brightness `100`.
- `bsp_display_backlight_off()` maps to brightness `0`.

Status:

- ONX GPIO6 LEDC PWM is verified and has the required state getter.
- The compatibility layer maps `bsp_display_brightness_init()`,
  `bsp_display_brightness_set()`, `bsp_display_brightness_get()`,
  `bsp_display_backlight_on()`, and `bsp_display_backlight_off()` to the
  verified ONX backlight API.

### Display Init and LVGL Adapter

Target behavior:

- `bsp_display_start_with_config()` must create an LVGL display for a
  `320 x 480` ST7796U SPI panel.
- The display path must preserve the verified LCD config:
  - `MADCTL=0x48`.
  - `COLMOD=0x55`.
  - `INVOFF`.
  - RGB565 payload byte-swap.
  - Standard `CASET`/`RASET` portrait windows with no direction-hiding offsets.
- `bsp_display_lock()` and `bsp_display_unlock()` should forward to the LVGL
  adapter lock once the ONX LVGL display path owns the adapter.
- `bsp_display_rotation_set()` must either:
  - start with portrait-only `BSP_DISPLAY_ROTATE_0`, or
  - implement the same rotation enum with a tested ST7796/LVGL coordinate
    transform.

Implementation note:

- The verified smoke driver writes directly through `esp_lcd_panel_io_tx_color`
  and does not yet expose an `esp_lcd_panel_handle_t`.
- M3 needs a real panel-handle path before full PrintSphere can start. That can
  be done by wrapping the verified ST7796U draw-window path in an
  `esp_lcd_panel_t` implementation, or by importing/adapting the official ONX
  ST7796U panel component into the ONX BSP component.
- This is not a drawing-layer workaround. The panel-handle path must retain the
  verified ST7796U init configuration.

Status:

- A temporary compatibility header is present at
  `components/onx3248g035_bsp/include/bsp/esp32_s3_touch_amoled_1_75.h`.
  Build/Release must ensure that only one BSP component providing this include
  path and the `bsp_*` symbols is selected for a firmware.
- `BSP_LCD_H_RES=320`, `BSP_LCD_V_RES=480`, `BSP_LCD_TOUCH_INT=GPIO_NUM_NC`,
  and ONX pin aliases are exported for the ONX profile.
- `bsp_display_new()` initializes the verified low-level ONX LCD path and can
  return the `esp_lcd_panel_io_handle_t`, but it deliberately returns
  `ESP_ERR_NOT_SUPPORTED` because there is no ONX `esp_lcd_panel_handle_t` yet.
- `bsp_display_start()` and `bsp_display_start_with_config()` deliberately
  return `NULL` after logging the missing LVGL adapter path. This is a blocker,
  not a fake display success.
- `bsp_display_rotation_set()` accepts only `BSP_DISPLAY_ROTATE_0` for the
  accepted portrait orientation. Other rotations return `ESP_ERR_NOT_SUPPORTED`
  until ST7796U and touch transforms are retested.

### Touch

Target behavior:

- `bsp_touch_new()` must create an `esp_lcd_touch_handle_t` for CST826 at
  `0x15`.
- Initial flags for the accepted portrait orientation:
  - `swap_xy = 0`.
  - `mirror_x = 0`.
  - `mirror_y = 0`.
- If App/UI keeps calling `apply_touch_rotation_flags()`, the ONX adapter must
  either honor those flags or start M3 in a fixed portrait orientation and block
  rotation until retested.

Status:

- Raw CST826 polling is verified.
- `bsp_touch_new()` initializes the verified raw CST826 path, clears the output
  handle, and returns `ESP_ERR_NOT_SUPPORTED`.
- `esp_lcd_touch` handle creation and LVGL input-device registration remain
  M3 blockers.

### PMU and Power

Current PrintSphere `PmuManager` assumes AXP2101 on the BSP I2C bus.
ONX3248G035 does not have AXP2101.

Replacement boundary:

- For M3 boot, App/Protocol should either:
  - bypass AXP2101 initialization on ONX and report power as unavailable, or
  - use an ONX power provider that reads battery ADC and charger state.
- ONX charge status can use PCF8574 pin 3 (`CHG_N`) once semantics are verified.
- Battery voltage needs an ADC path and calibration before it can replace the
  AXP2101 fuel-gauge percentage.

Status:

- App/Profile fallback is in place for `PRINTSPHERE_BOARD_ONX3248G035`: PMU
  initialization does not include or probe XPowersLib/AXP2101, returns `ESP_OK`,
  and reports power as unavailable through the default `PowerSnapshot`.
- This is a demo-first temporary strategy for M3 boot only. Battery ADC,
  calibration, USB/charge status, and PCF8574 `CHG_N` semantics remain separate
  bring-up work.
- Do not probe AXP2101 on ONX during M3.

### SD Card

The source Waveshare BSP has SDMMC mount APIs. ONX has PCF8574-controlled SD CS
in the board mapping, but SD has not been validated in M2.

Status:

- Deferred for M3 unless the app requires local storage beyond NVS and embedded
  assets.
- If needed later, implement as a separate ONX SD bring-up task.
- The compatibility layer exports `bsp_sdcard_mount()` and
  `bsp_sdcard_unmount()` shape only. Mount returns `ESP_ERR_NOT_SUPPORTED`;
  unmount is a no-op.

### Audio, Camera, IO Expander Compatibility

- Audio APIs from the Waveshare BSP are not used by PrintSphere startup and are
  out of scope for M3.
- Camera paths in PrintSphere are network camera clients, not a local board
  camera driver.
- PCF8574 is already used directly by ONX BSP for LCD reset and can be extended
  for board controls as needed.

## Board Selection Recommendation

Do not compile both the Waveshare BSP implementation and an ONX implementation
of the same `bsp_*` symbols into one firmware.

Build/Release status:

1. The current board selection option is
   `PRINTSPHERE_BOARD=waveshare_amoled_1_75|onx3248g035`.
2. The root `CMakeLists.txt` limits local components per board profile before
   `project()`, and `main/CMakeLists.txt` depends on exactly one board BSP
   component:
   - `esp32_s3_touch_amoled_1_75` for current Waveshare builds.
   - `onx3248g035_bsp` for ONX builds.
3. Keep the `bsp/*` public header shape compatible so `ui.cpp` can stay stable
   for M3.
4. Add an ONX-specific PMU/power provider or a temporary no-PMU provider.

## Minimal M3 Acceptance Target

M3 is ready when a minimal PrintSphere-on-ONX firmware can:

- Build with the ONX BSP adapter selected.
- Initialize the verified ST7796U display in portrait.
- Start LVGL and render the first PrintSphere screen.
- Initialize CST826 touch and receive pointer input.
- Set brightness through GPIO6 PWM.
- Avoid AXP2101 failures by using an ONX power fallback.

No Bambu protocol, MQTT, Web Config, OTA, SD card, battery percentage, or UI
layout migration is required for the first M3 hardware start milestone unless
the main thread explicitly expands scope.
