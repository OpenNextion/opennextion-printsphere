# ONX3248G035 Hardware Configuration

This document records the BSP hardware configuration used by the ONX BSP smoke
firmware and tracks LCD configuration candidates while visual validation is in
progress.

## LCD

- Controller: ST7796/ST7796U compatible SPI LCD.
- Resolution: `320 x 480`.
- Smoke coordinate system: portrait, origin at top-left, `x` increasing to the
  right, `y` increasing downward.
- SPI host: `SPI2_HOST`.
- SPI clock: `80 MHz`.
- SPI pins:
  - SCLK: GPIO5.
  - MOSI: GPIO1.
  - DC: GPIO3.
  - CS: GPIO2.
- Backlight: GPIO6 PWM through LEDC.
- Reset: PCF8574 pin 6 controls LCD reset.

## LCD Init State

The BSP sets the ST7796U state explicitly in
`components/onx3248g035_bsp/src/onx3248g035_bsp.c`.

- Configuration symbols under test:
  - `ONX_LCD_MADCTL_VALUE`.
  - `ONX_LCD_COLMOD_VALUE`.
  - `ONX_LCD_INVERT_CMD`.
  - `lcd_pack_rgb565()`.
- `COLMOD`: `0x55`, 16-bit RGB565 pixels.
- Current panel-init `MADCTL`: `LCD_CMD_MX_BIT | LCD_CMD_BGR_BIT`, value
  `0x48`.
- Current LVGL display mirror: not applied; LVGL uses the panel-init
  `MADCTL=0x48` state.
- `MX`: enabled by panel init for Recovery Candidate A. Candidate 1 tested the
  dynamic `mirror_x=1` path from a `0x08` base state; this recovery candidate
  tests the historical init-time `MX|BGR` path after the panel transport
  lifetime fix.
- `MY`: disabled.
- `MV`: disabled.
- `BGR`: enabled, matching the official ONX examples that configure
  `LCD_RGB_ELEMENT_ORDER_BGR`.
- Display inversion: current state remains `LCD_CMD_INVOFF`.

The official ST7796U component's default vendor init sequence sends `INVON`, but
the official board LCD initialization then calls `esp_lcd_panel_invert_color`
with `false`, which sends `INVOFF`. The BSP sends the final `INVOFF` command
directly at the end of the init sequence.

## Candidate Matrix

The candidates are BSP/panel configuration changes, not text renderer, color
label, touch target drawing, or LVGL UI layout workarounds.

| Candidate | Panel init MADCTL | LVGL dynamic mirror | Intended observation |
| --- | --- | --- | --- |
| Baseline before M4 | `MX|BGR` (`0x48`) | no-op `mirror(true,false)` | Known to still show user-visible mirror/display abnormality in LVGL smoke |
| Candidate 1 | `BGR` (`0x08`) | `mirror_x=1` writes `MX|BGR` (`0x48`) | Tests official-style dynamic mirror path; user still reports abnormality, so not final |
| Candidate 2 | `BGR` (`0x08`) | no mirror call; final `MADCTL=0x08` | Tests whether LVGL custom flush already has the expected orientation without `MX` |
| Candidate 3 | `BGR` (`0x08`) | no mirror call; final `MADCTL=0x08` | Moved LVGL RGB565 byte swap to the LVGL flush callback; visual result was no display, so this candidate failed |
| Candidate 2 + transport fix | `BGR` (`0x08`) | no mirror call; final `MADCTL=0x08` | Restores Candidate 2 RGB565 payload path and keeps the DMA queue drain reliability fix |
| Recovery Candidate A | `MX|BGR` (`0x48`) | no mirror call; final `MADCTL=0x48` | Restores the historically visible init-time MX+BGR path while keeping the DMA queue drain reliability fix |

## Current Modification Points

Configuration constants:

```c
#define ONX_LCD_MADCTL_VALUE (LCD_CMD_MX_BIT | LCD_CMD_BGR_BIT)
#define ONX_LCD_COLMOD_VALUE 0x55
#define ONX_LCD_INVERT_CMD LCD_CMD_INVOFF
```

LVGL display mirror state:

```c
/* no esp_lcd_panel_mirror() call in onx_bsp_lvgl_start() */
```

LVGL RGB565 payload byte order:

```c
static uint16_t panel_pack_rgb565(uint16_t rgb565)
{
    return __builtin_bswap16(rgb565);
}
```

The ONX `esp_lcd_panel_t` wrapper owns LVGL RGB565 SPI byte-order conversion.
Candidate 3's `lv_draw_sw_rgb565_swap()` flush-callback experiment produced no
visible display and is not the current validation target.

Panel transport lifetime:

```c
esp_lcd_panel_io_tx_param(io, -1, NULL, 0);
```

The panel wrapper drains the ESP-IDF SPI panel IO color-transfer queue after
each chunk sent by `esp_lcd_panel_io_tx_color()`. This is a transport reliability
fix: queued SPI DMA transfers may outlive the `tx_color()` call, and the staging
buffer must not be reused or freed until the queued color transfer is complete.
It does not change the LCD register state, LVGL mirror state, color constants,
or address window.


Initialization commands:

```c
{LCD_CMD_MADCTL, s_cmd_madctl, sizeof(s_cmd_madctl), 0}
{LCD_CMD_COLMOD, s_cmd_colmod, sizeof(s_cmd_colmod), 0}
{ONX_LCD_INVERT_CMD, NULL, 0, 0}
```

Pixel payload helper:

```c
static uint16_t lcd_pack_rgb565(uint16_t rgb565)
{
    return __builtin_bswap16(rgb565);
}
```

Address-window commands remain standard and are not used to hide the direction
issue:

```c
LCD_CMD_CASET: x_start to x_end - 1
LCD_CMD_RASET: y_start to y_end - 1
LCD_CMD_RAMWR: selected-window pixel payload
```

What Recovery Candidate A is testing:

- Whether restoring the historical init-time `MADCTL=MX|BGR` state recovers the
  LVGL smoke display after both Candidate 3 and Candidate 2 + transport fix
  produced no visible display.
- Whether the panel transport lifetime fix can remain enabled without causing a
  blank screen when the controller scan state returns to `0x48`.
- `MADCTL.BGR` matches the ONX official BGR panel element order.
- `INVOFF` remains unchanged because the official board layer also ends in
  inversion disabled.
- `COLMOD=0x55` keeps the controller in 16-bit RGB565 mode.
- Direct smoke still uses `lcd_pack_rgb565()` before raw `tx_color` writes.
- LVGL smoke uses the panel wrapper byte swap before SPI transfer.

## Defect-to-Configuration Map

The observed failures are being tracked as ST7796U controller-state issues, not
as smoke page renderer issues.

| Observed defect | Root configuration point | Current handling |
| --- | --- | --- |
| LVGL text or labels mirrored | `MADCTL.MX` may be required for the local custom LVGL flush path to be visible, but may still mirror text | Recovery Candidate A restores init-time `MX|BGR`; no dynamic mirror call |
| Red/blue channel mismatch or color labels not matching visible colors | `MADCTL.BGR` must match the ONX panel element order | Keep `LCD_CMD_BGR_BIT` |
| White and black inverted | Panel inversion state left enabled | Keep final `LCD_CMD_INVOFF` through `ONX_LCD_INVERT_CMD` |
| RGB565 words producing wrong colors over SPI | SPI color payload byte order differs from logical `uint16_t` color constants | LVGL path uses panel-wrapper byte swap; direct smoke keeps `lcd_pack_rgb565()` |
| Direction issue tempting coordinate offsets | Address window should express logical portrait coordinates only | Keep `CASET`/`RASET` as `x_start..x_end-1`, `y_start..y_end-1` with no offsets |

Recovery Candidate A has one relevant LVGL `MADCTL` state:

- Panel init and LVGL runtime: `0x48`, MX plus BGR.

`MY` and `MV` remain clear in this candidate, so the controller stays in
portrait `320 x 480`; the candidate does not swap axes or apply a vertical
mirror.

## RGB565 Bus Byte Order

Logical colors remain standard RGB565 values:

- Red: `0xF800`.
- Green: `0x07E0`.
- Blue: `0x001F`.
- White: `0xFFFF`.
- Black: `0x0000`.

Before writing pixels through `esp_lcd_panel_io_tx_color`, the BSP byte-swaps
logical RGB565 values at the transport boundary:

- Direct smoke raw path:
  `components/onx3248g035_bsp/src/onx3248g035_bsp.c` uses
  `lcd_pack_rgb565()`.
- LVGL / `esp_lcd_panel_t` wrapper path:
  `components/onx3248g035_bsp/src/onx3248g035_bsp_compat.c` uses
  `panel_pack_rgb565()`.

Both helpers currently perform the same `__builtin_bswap16()` conversion. This
matches the official sample's use of swapped 16-bit color data for SPI LCD
transfers.

## Address Window

The BSP uses standard ST7796 address windows:

- `CASET`: `x_start` to `x_end - 1`.
- `RASET`: `y_start` to `y_end - 1`.
- `RAMWR`: pixel payload for the selected window.

No x/y gap is currently applied. The smoke firmware validates the full
`0..319`, `0..479` logical coordinate range.

Do not introduce CASET/RASET gaps, coordinate offsets, or draw-time mirroring as
a substitute for the MADCTL/INVOFF/RGB565 configuration under test. Any future
panel-handle or LVGL adapter path must preserve this portrait coordinate system
unless a new hardware validation run records a different result.

The M3 LVGL adapter follows the same rule: LVGL flushes through the ONX
`esp_lcd_panel_handle_t` wrapper, and Recovery Candidate A uses init-time
`MX|BGR` while keeping the panel wrapper byte swap. LVGL labels, color bands, or
touch targets must not compensate for mirror, inversion, RGB/BGR order, byte
order, or CASET/RASET offset mistakes.

## Touch

- Controller: CST826 at I2C address `0x15`.
- I2C bus: `I2C_NUM_0`.
- I2C pins:
  - SDA: GPIO8.
  - SCL: GPIO7.
- PCF8574 pin 5 is the touch interrupt input path in the board mapping, but the
  smoke firmware currently polls CST826 data registers.

Accepted touch evidence from M2 showed:

- `swap_xy`: no.
- `mirror_x`: no.
- `mirror_y`: no.
- Observed x range: roughly `23..290`.
- Observed y range: roughly `28..458`.
- Coordinates increase left-to-right and top-to-bottom in portrait orientation.

If LCD scan direction is changed later, touch mapping must be revalidated
against the displayed `TL`, `TR`, `BR`, `BL`, and `CENTER` targets.

Current LVGL touch validation status:

- The LVGL smoke screen displays three touch targets: `TOUCH TOP LEFT`,
  `TOUCH CENTER`, and `TOUCH BOTTOM RIGHT`.
- The initial LVGL smoke event handler was attached only to the root screen.
  The visible target boxes are clickable objects, so tapping the center of a
  box can be handled by the box or label instead of the screen. That made touch
  logging unreliable even when the CST826 input path was alive.
- The LVGL smoke validation program now attaches the same touch callback to the
  root screen, each touch target box, and each label inside the box. This is a
  validation-program event routing fix only; it does not change CST826
  `swap_xy`, `mirror_x`, or `mirror_y`.
- LVGL touch mapping validation passed after the event-routing fix. Captured
  examples:
  - `Touch x=86 y=41 target=TOUCH TOP LEFT`.
  - `Touch x=182 y=252 target=TOUCH CENTER`.
  - `Touch x=280 y=450 target=TOUCH BOTTOM RIGHT`.
- The accepted LVGL touch transform remains `swap_xy=0`, `mirror_x=0`, and
  `mirror_y=0`.

## Landscape Orientation Probe

Portrait remains the accepted stable ONX path for PrintSphere:

- Panel init `MADCTL=0x48` (`MX|BGR`).
- `COLMOD=0x55`.
- Final `INVOFF`.
- RGB565 payload byte swap at the BSP transport boundary.
- Touch transform `swap_xy=0`, `mirror_x=0`, `mirror_y=0`.

Landscape support is not accepted yet. The BSP hardware thread uses
`examples/onx_bsp_orientation_probe` as a direct-panel validation firmware
before any PrintSphere UI landscape layout work.

The probe does not change the default BSP portrait init constants. It
temporarily writes `MADCTL` at runtime for four landscape scan candidates:

- `LANDSCAPE MV 28`: `MADCTL=0x28` (`MV|BGR`), logical display `480x320`.
  Touch mapping under test: `mapped_x=raw_y`, `mapped_y=raw_x`.
- `LANDSCAPE MX MV 68`: `MADCTL=0x68` (`MX|MV|BGR`), logical display
  `480x320`. Touch mapping under test: `mapped_x=raw_y`,
  `mapped_y=319-raw_x`.
- `LANDSCAPE MY MV A8`: `MADCTL=0xA8` (`MY|MV|BGR`), logical display
  `480x320`. Touch mapping under test: `mapped_x=479-raw_y`,
  `mapped_y=raw_x`.
- `LANDSCAPE MX MY MV E8`: `MADCTL=0xE8` (`MX|MY|MV|BGR`), logical display
  `480x320`. Touch mapping under test: `mapped_x=479-raw_y`,
  `mapped_y=319-raw_x`.

The validation screen displays `TOP`, `BOTTOM`, `LEFT`, `RIGHT`, and touch
targets for `TAP TOP LEFT`, `TAP TOP RIGHT`, `TAP BOTTOM LEFT`,
`TAP BOTTOM RIGHT`, and `TAP CENTER`. Serial logs must include raw CST826
coordinates, mapped landscape coordinates, the inferred target, and the active
transform. These logs are used to decide whether one landscape direction can
move to UI design work or whether a single touch transform bit (`swap_xy`,
`mirror_x`, or `mirror_y`) must be tested next.

Landscape probe evidence so far:

- `LANDSCAPE MX MV 68` (`MADCTL=0x68`) was built, flashed through the standard
  flow, and captured with touch logs. Touch mapping for five visible target
  taps was internally consistent, but the user reported the displayed font was
  mirrored. Therefore `0x68` is not accepted as the final landscape display
  scan state.
- The next probe firmware keeps the same direct draw path and RGB565 byte swap
  but cycles all four `MV` landscape `MADCTL` mirror combinations. This is a
  hardware scan-state diagnostic, not a UI text mirroring workaround.
- Four-candidate probe build/flash evidence: `examples/onx_bsp_orientation_probe`
  built successfully with binary size `0x3bd10`, then flashed through the
  standard flow on 2026-06-05 using `/dev/tty.wchusbserial10`, `115200`, and
  `write-flash @flash_args`; esptool verified the written bootloader,
  partition-table, and app hashes and hard reset the board. Runtime capture
  confirmed candidate page transitions for `0xE8`, `0x28`, `0x68`, and `0xA8`.
  No touch press lines were captured in that 140 second window. Visual selection
  of the non-mirrored landscape scan state is still pending.
- Re-run visual result: the user selected `LANDSCAPE MX MY MV E8` as the
  landscape candidate to keep evaluating. A 150 second runtime capture showed
  `MADCTL=0xE8` page readiness and touch logs mapping the visible five-target
  set consistently:
  - Bottom-left tap: raw `(64,424)` mapped to `(55,255)`, target
    `BOTTOM LEFT`.
  - Bottom-right tap: raw `(70,88)` mapped to `(391,249)`, target
    `BOTTOM RIGHT`.
  - Center tap: raw `(183,249)` mapped to `(230,136)`, target `CENTER`.
  - Top-left tap: raw `(272,401)` mapped to `(78,47)`, target `TOP LEFT`.
  - Top-right tap: raw `(280,85)` mapped to `(394,39)`, target `TOP RIGHT`.
  This makes the current low-level landscape candidate
  `MADCTL=0xE8` (`MX|MY|MV|BGR`) with touch transform
  `mapped_x=479-raw_y`, `mapped_y=319-raw_x`. This is a BSP hardware-probe
  result only; PrintSphere UI layout has not been changed for landscape.

## Official ONX Source Comparison

Reference sources:

- `Example Programs/ESP-IDF/01_touch_test/components/esp_lcd_st7796u/esp_lcd_st7796u.c`
  in the OpenNextion ONX3248G035 repository.
- `Example Programs/ESP-IDF/01_touch_test/main/touch_test.c` in the same
  repository.
- `Example Programs/ESP-IDF/09_outofbox_demo/components/board/lcd_st7796u.c`
  in the same repository.
- `Example Programs/ESP-IDF/09_outofbox_demo/components/board/board_lcd.c`
  and `touch_cst826.c` in the same repository.

Official source observations:

- The official board LCD config uses SPI mode 0, 8-bit commands and parameters,
  an 80 MHz pixel clock, `LCD_RGB_ELEMENT_ORDER_BGR`, and 16 bits per pixel.
- The official ST7796U component maps BGR element order to `LCD_CMD_BGR_BIT`
  and 16 bits per pixel to `COLMOD=0x55`.
- The official ST7796U component's draw path uses standard `CASET`, `RASET`,
  and `RAMWR` windows with `x_end - 1` and `y_end - 1`; offsets are only panel
  gaps, not a mirror workaround.
- The official ST7796U component exposes mirror and swap operations by rewriting
  `MADCTL.MX`, `MADCTL.MY`, and `MADCTL.MV`.
- The official board initialization calls `esp_lcd_panel_invert_color(...,
  false)`, producing a final `INVOFF` state.
- The official LVGL port sets display `mirror_x=1` while keeping `swap_xy=0`
  and `mirror_y=0`.
- The official CST826 touch config uses `x_max=320`, `y_max=480`,
  `swap_xy=0`, `mirror_x=0`, and `mirror_y=0`.

Matches:

- ST7796U vendor power/gamma sequence.
- SPI mode 0, 8-bit commands, 8-bit parameters, 80 MHz pixel clock.
- `COLMOD=0x55`.
- BGR element order.
- Final display inversion disabled.
- No XY swap for touch.

Intentional differences:

- Recovery Candidate A restores the historical init-time `MX|BGR` state and
  intentionally does not request an additional dynamic LVGL mirror. This is a
  recovery diagnostic to confirm whether the display can become visible again;
  it is not a final accepted orientation decision.
- Candidate 3 tested the LVGL v9 flush-callback RGB565 swap hook, but the user
  reported no visible display. The current target restores the panel-wrapper
  byte swap and keeps the transport lifetime drain.
- The smoke firmware sends final `INVOFF` directly instead of sending `INVON`
  first and then calling the panel API to disable inversion.
- The smoke firmware packs RGB565 bus byte order in a single helper because it
  writes raw `uint16_t` buffers directly.

Rejected approaches:

- Mirroring glyph columns or touch labels in software.
- Swapping color constants such as using `0x001F` for red.
- Adding CASET/RASET offsets to compensate for scan direction.
- Re-enabling `INVON` and compensating in drawing code.
- Applying LVGL-only rotation flags in a future panel adapter without preserving
  the same final ST7796U register state.

## Visual Acceptance

The smoke firmware has two screen pages:

1. A color page with labeled `RED`, `GREEN`, `BLUE`, `WHITE`, and `BLACK`
   regions.
2. A touch and direction page with `TOP`, `BOTTOM`, `LEFT`, `RIGHT`, `X`, `Y`,
   `TL`, `TR`, `BR`, `BL`, and `CENTER`.

Visual pass requires:

- Labels are not mirrored.
- `TOP` appears at the physical top.
- `BOTTOM` appears at the physical bottom.
- `LEFT` appears on the physical left.
- `RIGHT` appears on the physical right.
- White and black are not inverted.
- Red, green, and blue labels match their visible color blocks.

## Visual Result

Historical M2 direct-smoke visual confirmation on 2026-06-02:

- Color labels match the visible colors.
- White and black are correct and not inverted.
- Touch page text is readable and not mirrored.
- Touch label positions match expectation.

Current M4 LVGL smoke status:

- The user reported that the current LVGL smoke baseline still shows color or
  text mirror/orientation problems.
- Therefore `MADCTL=0x48` preloaded during panel init is no longer treated as a
  final accepted LVGL configuration.
- Candidate 2 was built and flashed through the standard `examples/onx_bsp_lvgl_smoke`
  flow on 2026-06-02 using `/dev/tty.wchusbserial10`, `115200`, and
  `write-flash @flash_args`; esptool verified the written image hash and hard
  reset the board.
- The short post-flash serial capture did not include firmware runtime logs, so
  the remaining acceptance signal for Candidate 2 is user visual confirmation.
- Candidate 3 was built and flashed through the standard `examples/onx_bsp_lvgl_smoke`
  flow on 2026-06-02 using `/dev/tty.wchusbserial10`, `115200`, and
  `write-flash @flash_args`; esptool verified the written image hash and hard
  reset the board.
- After adding the panel transport lifetime drain, Candidate 3 was rebuilt and
  flashed again through the same standard flow; esptool again verified the
  written image hash and hard reset the board.
- The short post-flash serial capture did not include firmware runtime logs, so
  the remaining acceptance signal for Candidate 3 was user visual confirmation.
- Candidate 3 visual result: user reported no visible display. Candidate 3 is
  rejected and must not be marked final.
- Recovery Candidate A restores the historical init-time `MX|BGR` display state
  while keeping the panel transport lifetime drain. It was built and flashed
  through the standard `examples/onx_bsp_lvgl_smoke` flow on 2026-06-02 using
  `/dev/tty.wchusbserial10`, `115200`, and `write-flash @flash_args`; esptool
  verified the written image hash and hard reset the board. Short runtime log
  capture was unavailable because the serial capture approval timed out.
- Recovery Candidate A visual result: user reported that the screen still did
  not light and showed no display. Recovery Candidate A is rejected and must not
  be marked final.
- Next isolation step: flash the existing direct smoke,
  `examples/onx_bsp_smoke`, to distinguish LVGL adapter/start/flush failure
  from lower-level panel init, transport, backlight, or reset failure.
- Direct smoke isolation result: after flashing `examples/onx_bsp_smoke`
  through the standard flow, the user reported that display succeeded and the
  original color bars and touch targets were visible. The direct smoke path
  explicitly initializes backlight PWM, sets brightness to 30%, initializes the
  LCD, then sets brightness to 100% before drawing the visible pages.
- LVGL Isolation 1: `examples/onx_bsp_lvgl_smoke` performs a short direct raw
  color-bar prefill before starting LVGL. This is a path diagnostic, not a UI
  workaround: if the prefill appears and LVGL then blanks the screen, the
  failure is inside LVGL start/flush/panel-wrapper; if the prefill is not
  visible, the same firmware is failing before LVGL takes over.
- LVGL Isolation 1 also protects the panel wrapper cached `MADCTL` state as
  `0x48` so an accidental `mirror(false, false)` call cannot reset Recovery A
  back to `0x08`.
- LVGL Isolation 1 build/flash evidence: `examples/onx_bsp_lvgl_smoke` built
  successfully with binary size `0x8ba80`, then flashed through the standard
  flow on 2026-06-02 using `/dev/tty.wchusbserial10`, `115200`, and
  `write-flash @flash_args`; esptool verified the written image hash and hard
  reset the board.
- LVGL Isolation 1 runtime log capture was unavailable because the serial
  capture approval timed out.
- LVGL Isolation 1 visual timing is unconfirmed: after the firmware was flashed
  and reset, the user observed a black screen, but could not determine whether
  the 1.5 second direct raw prefill appeared before observation. Direct smoke
  remains the confirmed visible raw LCD path.
- LVGL Isolation 2 is a runtime trace firmware, not a fix candidate. It extends
  the direct raw prefill hold to 10 seconds and logs prefill start, draw result,
  hold start, and hold end before entering `bsp_display_start_with_config()`.
  It also logs `onx_bsp_lvgl_start()` stage progress, the first LVGL flush
  windows, pixel summaries, return values, LVGL task heartbeat, and the first
  panel wrapper draw/chunk transfer results. The intended observation is
  whether direct prefill color bars are visible during the first 10 seconds
  after reboot, then whether the screen turns black or reaches the LVGL
  validation page.
- LVGL Isolation 2 runtime evidence showed app startup, LCD init, direct prefill
  draw, LVGL start, non-black flushes, panel draw, SPI tx/drain, touch input,
  and LVGL heartbeat all reporting success. The user still saw black during the
  10 second prefill window and after LVGL started. That result shifts the next
  diagnostic away from color, direction, byte order, and LVGL startup toward LCD
  visible-state parity.
- LVGL Isolation 3 is a backlight-visible-state parity candidate. It does not
  change MADCTL, COLMOD, INVOFF, RGB565 byte order, CASET/RASET, LVGL labels,
  color constants, or touch target layout. It makes `examples/onx_bsp_lvgl_smoke`
  match the direct smoke visible-state sequence by explicitly initializing
  backlight and setting brightness to 100% before the direct prefill, then
  explicitly enabling backlight again after `bsp_display_brightness_init()` in
  the LVGL adapter start path.
- LVGL Isolation 3 visual result: user reported that the prefill stripes became
  visible and then the LVGL touch target overlay appeared. This validates the
  backlight visible-state fix. The LVGL adapter path must explicitly enable
  backlight after display start and the LVGL smoke must keep backlight enabled
  before the diagnostic prefill.
