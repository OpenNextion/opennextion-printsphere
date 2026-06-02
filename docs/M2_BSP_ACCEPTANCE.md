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

Remaining manual checks:

- Visual LCD color and orientation check.
- Four-corner touch coordinate check.

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
LCD color bars complete
Touch init complete: CST826 addr=0x15 chip_id=0x11
ONX3248G035 BSP smoke init complete; touch sampling is running
Touch sampler still running; waiting for touch
```

Expected touch evidence while tapping:

```text
Touch: points=1 x=... y=...
```

## User Procedure

1. Confirm the board screen is on.
2. Confirm whether color bars are visible.
3. Record whether the bars look red, green, blue, white, and black.
4. Record the orientation: portrait `320 x 480`, landscape `480 x 320`, rotated,
   mirrored, or otherwise wrong.
5. When serial capture starts, tap in this order:
   - Top-left corner.
   - Top-right corner.
   - Bottom-right corner.
   - Bottom-left corner.
   - Center.
6. Pause about one second between taps.

## Pass Criteria

Visual LCD check passes when:

- The screen is not black, white-only, or corrupted.
- Color bars are visible.
- Red, green, blue, white, and black are distinguishable.
- Orientation is known and can be used by the UI thread.

Touch check passes when:

- Each of the five taps produces at least one `Touch:` line.
- Coordinates change in the expected direction across corners.
- The coordinate range is plausible for the panel, roughly `0..319` on one axis
  and `0..479` on the other, allowing small edge offsets.
- Any needed `swap`, `mirror_x`, `mirror_y`, or offset correction can be inferred.

M2 should remain open if:

- The LCD is blank, white-only, severely corrupted, or unreadable.
- Color order is unclear.
- Orientation cannot be determined.
- Touch produces no coordinates.
- Coordinates are unstable enough that corner mapping cannot be inferred.

## Record Template

```text
Date:
Branch:
Commit:
Firmware:
Serial port:

LCD visual:
- Color bars visible:
- Color order:
- Orientation:
- Brightness visible:
- Notes:

Touch taps:
- Top-left:     x=, y=
- Top-right:    x=, y=
- Bottom-right: x=, y=
- Bottom-left:  x=, y=
- Center:       x=, y=

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
