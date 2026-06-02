# ONX BSP LVGL Smoke

This example is a BSP smoke target for the ONX3248G035 LVGL adapter. It does
not include the PrintSphere application, MQTT, Bambu protocol code, Web Config,
or UI layout.

The target verifies that `components/onx3248g035_bsp` can compile and link its
LVGL adapter path against the locally cached `managed_components/lvgl__lvgl`
component. If flashed later through the standard project flow, it shows a
labeled validation screen:

- Color bands labeled `RED`, `GREEN`, `BLUE`, `WHITE`, and `BLACK`.
- Touch targets labeled `TOUCH TOP LEFT`, `TOUCH CENTER`, and
  `TOUCH BOTTOM RIGHT`.
- Serial logs for touch coordinates and inferred target.

This is not the PrintSphere app acceptance firmware.

The example uses the same baseline ONX board defaults as
`examples/onx_bsp_smoke`: ESP32-S3 target, DIO flash mode, 16 MB flash, Octal
PSRAM at 80 MHz, 240 MHz CPU, and 1000 Hz FreeRTOS tick. This keeps generated
flash arguments aligned with the ONX3248G035 standard smoke firmware.

Build:

```sh
cd /Users/alex/Documents/codex_project/Nextion_project_PrintSphere
export IDF_TOOLS_PATH="$PWD/.tools/espressif"
export IDF_COMPONENT_MANAGER=0
. "$PWD/.tools/esp-idf-v6.0.1/export.sh"
idf.py -C examples/onx_bsp_lvgl_smoke fullclean build
```

This example is intended for BSP adapter build validation. Do not flash it
unless a thread explicitly selects it as the current hardware validation
firmware and follows `docs/BUILD_FLASH.md`.
