# Supported Boards

This document lists the public board support status for OpenNextion-printsphere.

## v0.1.0 Included Target

| Board | Display size | Orientation | Release status | Notes |
| --- | --- | --- | --- | --- |
| [ONX3248G035](https://nextion.tech/wiki/onx3248g035/) | 3.5 inch | Landscape | Included in `v0.1.0` | First public release target and recommended configuration |

`v0.1.0` only includes the [ONX3248G035](https://nextion.tech/wiki/onx3248g035/)
landscape full initial flashing image.

## Not Included In v0.1.0

| Board | Display size | Orientation | Status | Notes |
| --- | --- | --- | --- | --- |
| [ONX3248G035](https://nextion.tech/wiki/onx3248g035/) | 3.5 inch | Portrait | Not a public `v0.1.0` target | Requires a separate public build profile and final desktop UI validation |
| [ONX2432G028](https://nextion.tech/wiki/onx2432g028/) | 2.8 inch | Landscape / portrait | Future target | Requires its own BSP, board profile, and release validation |

## Notes

- Do not assume that another OpenNextion or Nextion display is compatible only
  because it uses ESP32.
- Each board needs its own display, touch, backlight, resolution, orientation,
  and LVGL integration validation.
- Firmware for unsupported boards or orientations should not be flashed unless
  a matching public board profile and release asset are provided.
- Release firmware is subject to the repository `LICENSE`, the Federation
  Non-Commercial License (FNCL) v1.1.
