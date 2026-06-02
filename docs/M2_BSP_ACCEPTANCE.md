# M2 BSP Acceptance

This document defines the remaining manual acceptance checks for M2 ONX board
bring-up after commit `209e836 Add ONX BSP smoke bring-up`.

## Scope

The smoke firmware validates the ONX3248G035 hardware BSP only. It does not run
the PrintSphere application, LVGL production UI, Wi-Fi, Bambu protocol, Web
Config, OTA, SD card, battery, or camera paths.

Already accepted by build and serial evidence:

- ESP-IDF build for `examples/onx_bsp_smoke`.
- Flash to `/dev/cu.wchusbserial10`.
- I2C init on GPIO8/GPIO7.
- PCF8574 init at `0x38`.
- GPIO6 backlight PWM setup and duty changes.
- ST7796 SPI init and color-fill transfers.
- CST826 init at `0x15`, chip ID `0x11`.
- Continuous touch sampling loop.
- Enhanced smoke screen flow with a labeled color page and labeled touch target
  page.

Remaining manual checks:

- Visual LCD color, label readability, and orientation check.
- Labeled target touch check.

## Setup

The smoke firmware should already be flashed. If it needs to be rebuilt or
reflashed, use the project standard environment:

```sh
cd /Users/alex/Documents/codex_project/Nextion_project_PrintSphere
export IDF_TOOLS_PATH="$PWD/.tools/espressif"
export IDF_COMPONENT_MANAGER=0
. "$PWD/.tools/esp-idf-v6.0.1/export.sh"
idf.py -C examples/onx_bsp_smoke build
idf.py -C examples/onx_bsp_smoke -p /dev/cu.wchusbserial10 flash
```

## Serial Capture

Use this command when the user is ready to tap the screen:

```sh
cd /Users/alex/Documents/codex_project/Nextion_project_PrintSphere
export IDF_TOOLS_PATH="$PWD/.tools/espressif"
export IDF_COMPONENT_MANAGER=0
. "$PWD/.tools/esp-idf-v6.0.1/export.sh"
python -m esptool --chip esp32s3 -p /dev/cu.wchusbserial10 run
python -c 'import serial,time; p="/dev/cu.wchusbserial10"; s=serial.Serial(p,115200,timeout=0.2); end=time.time()+60; buf=[]; print("Capture started: tap TL, TR, BR, BL, center");
while time.time()<end:
    data=s.read(4096)
    if data:
        text=data.decode("utf-8","replace")
        print(text,end="")
s.close()'
```

Expected boot evidence before tapping:

```text
M2 color acceptance page: verify RED/GREEN/BLUE/WHITE/BLACK labels
LCD labeled color page complete: RED GREEN BLUE WHITE BLACK
Touch init complete: CST826 addr=0x15 chip_id=0x11
M2 touch acceptance page: tap TL, TR, BR, BL, CENTER targets
LCD touch target page complete: TL TR BR BL CENTER
ONX3248G035 BSP smoke init complete; touch sampling is running
Touch sampler still running; waiting for touch
```

Expected touch evidence while tapping:

```text
Touch: points=1 x=... y=... target=...
```

## User Procedure

1. Confirm the board screen is on.
2. On the color page, confirm five labeled blocks are visible:
   - `RED`
   - `GREEN`
   - `BLUE`
   - `WHITE`
   - `BLACK`
3. Record whether each label matches the visible block color.
4. Confirm labels are readable: black text on bright blocks, white text on dark
   blocks.
5. Record the orientation: portrait `320 x 480`, landscape `480 x 320`, rotated,
   mirrored, or otherwise wrong.
6. Wait for the firmware to switch to the touch page.
7. Tap the visible target markers in this order:
   - `TL`
   - `TR`
   - `BR`
   - `BL`
   - `CENTER`
8. Pause about one second between taps.

## Pass Criteria

Visual LCD check passes when:

- The screen is not black, white-only, or corrupted.
- The five labeled color blocks are visible.
- `RED`, `GREEN`, `BLUE`, `WHITE`, and `BLACK` labels match the visible colors.
- Labels are readable on their backgrounds.
- Orientation is known and can be used by the UI thread.

Touch check passes when:

- Each of the five labeled target taps produces at least one `Touch:` line.
- Serial `target=...` inference matches or is close to the tapped screen label.
- Coordinates change in the expected direction across corners.
- The coordinate range is plausible for the panel, roughly `0..319` on one axis
  and `0..479` on the other, allowing small edge offsets.
- Any needed `swap`, `mirror_x`, `mirror_y`, or offset correction can be inferred.

M2 should remain open if:

- The LCD is blank, white-only, severely corrupted, or unreadable.
- Color order is unclear.
- Orientation cannot be determined.
- Touch produces no coordinates.
- The touch target page is missing or unreadable.
- Coordinates are unstable enough that corner mapping cannot be inferred.

## Record Template

```text
Date:
Branch:
Commit:
Firmware:
Serial port:

LCD visual:
- Labeled color blocks visible:
- Color order:
- Label readability:
- Orientation:
- Brightness visible:
- Notes:

Touch taps:
- TL:     x=, y=, target=
- TR:     x=, y=, target=
- BR:     x=, y=, target=
- BL:     x=, y=, target=
- CENTER: x=, y=, target=

Mapping inference:
- swap_xy:
- mirror_x:
- mirror_y:
- x_range:
- y_range:
- offset notes:

Decision:
- pass / needs BSP correction / needs manual retest
```

## Acceptance Record - 2026-06-02 Touch Capture

Branch: `feature/onx-bsp-bringup`

Commits:

- `209e836 Add ONX BSP smoke bring-up`
- `322a419 Document M2 BSP acceptance procedure`

Firmware: `examples/onx_bsp_smoke`

Serial port: `/dev/cu.wchusbserial10`

Serial capture result:

- Boot evidence lines: 8.
- Touch lines: 65.
- No panic, reboot, or watchdog observed during the capture window.

Representative touch taps:

```text
Top-left:     points=1 x=56  y=28
Top-right:    points=1 x=283 y=53
Bottom-right: points=1 x=274 y=439
Bottom-left:  points=1 x=39  y=458
Center:       points=1 x=172 y=223
```

Second repeated sequence:

```text
Top-left:     points=1 x=42  y=39
Top-right:    points=1 x=254 y=56
Bottom-right: points=1 x=290 y=435
Bottom-left:  points=1 x=48  y=453
Center:       points=1 x=188 y=257
```

Mapping inference:

- `swap_xy`: no.
- `mirror_x`: no.
- `mirror_y`: no.
- Observed x range: roughly `23..290`.
- Observed y range: roughly `28..458`.
- The touch coordinate orientation matches a portrait panel with x increasing
  left-to-right and y increasing top-to-bottom.

Decision:

- Touch coordinate check: pass.
- Visual LCD color and orientation check: still needs user confirmation.

## Acceptance Record - 2026-06-02 Enhanced Smoke Firmware

Branch: `feature/onx-bsp-bringup`

Firmware: `examples/onx_bsp_smoke`

Serial port: `/dev/cu.wchusbserial10`

Build result:

- `idf.py -C examples/onx_bsp_smoke build` passed.
- Binary size: `0x3db80`.

Flash result:

- High-speed `idf.py flash` hit an intermittent pySerial readiness error while
  connecting to the CH340 serial port.
- Low-speed direct esptool flash through `/dev/tty.wchusbserial10` passed with
  `-b 115200` and verified all written data.

Serial evidence:

```text
M2 color acceptance page: verify RED/GREEN/BLUE/WHITE/BLACK labels
LCD labeled color page complete: RED GREEN BLUE WHITE BLACK
Touch init complete: CST826 addr=0x15 chip_id=0x11
M2 touch acceptance page: tap TL, TR, BR, BL, CENTER targets
LCD touch target page complete: TL TR BR BL CENTER
ONX3248G035 BSP smoke init complete; touch sampling is running
Touch sampler still running; waiting for touch
```

Decision:

- Enhanced labeled smoke firmware: pass by build, flash, and serial evidence.
- Visual confirmation by the user is still required for label readability,
  visible colors, and screen orientation.
