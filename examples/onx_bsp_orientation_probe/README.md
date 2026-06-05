# ONX BSP Orientation Probe

This firmware validates optional ONX3248G035 landscape display and touch
transforms at the BSP/hardware layer. It does not start PrintSphere, LVGL,
Wi-Fi, Bambu protocol code, Web Config, OTA, camera, or SD mounting.

The probe cycles through four direct-panel ST7796 `MADCTL` candidates:

- `LANDSCAPE MV 28`: `MADCTL=MV|BGR` (`0x28`).
- `LANDSCAPE MX MV 68`: `MADCTL=MX|MV|BGR` (`0x68`).
- `LANDSCAPE MY MV A8`: `MADCTL=MY|MV|BGR` (`0xA8`).
- `LANDSCAPE MX MY MV E8`: `MADCTL=MX|MY|MV|BGR` (`0xE8`).

Each page shows `TOP`, `BOTTOM`, `LEFT`, `RIGHT`, the candidate name, and five
targets: `TAP TOP LEFT`, `TAP TOP RIGHT`, `TAP BOTTOM LEFT`,
`TAP BOTTOM RIGHT`, and `TAP CENTER`.

Serial output logs raw CST826 coordinates, mapped landscape coordinates, target
classification, and the transform used for the active candidate. Use it to
confirm whether the visible target and the logged target agree before any
PrintSphere UI landscape work starts.

Current accepted low-level landscape candidate: `LANDSCAPE MX MY MV E8`
(`MADCTL=0xE8`) with touch mapping `mapped_x=479-raw_y` and
`mapped_y=319-raw_x`. This is a BSP hardware-probe result only; PrintSphere UI
layout still needs a separate landscape design and implementation pass.

Rejected result: `LANDSCAPE MX MV 68` was visually reported as mirrored, so it
is not accepted as the landscape display value. The four-way matrix remains as
a diagnostic tool to re-check scan state without UI text mirroring, coordinate
offsets, or color-value workarounds.

Build:

```sh
cd /Users/alex/Documents/codex_project/Nextion_project_PrintSphere
export IDF_TOOLS_PATH="$PWD/.tools/espressif"
export IDF_COMPONENT_MANAGER=0
. "$PWD/.tools/esp-idf-v6.0.1/export.sh"
idf.py -C examples/onx_bsp_orientation_probe build
```

Flash only when this firmware is the assigned validation target, using
`docs/BUILD_FLASH.md` standard flow from
`examples/onx_bsp_orientation_probe/build`.
