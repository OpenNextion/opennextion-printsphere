# ONX3248G035 BSP Smoke Firmware

This is a minimal hardware bring-up firmware for ONX3248G035. It does not start
the PrintSphere application, network stack, Bambu protocol, Web Config, or OTA.

Build from the workspace root with the documented project-local ESP-IDF
environment. Component Manager is disabled because this smoke project uses only
ESP-IDF components and the local ONX BSP component.

```sh
cd /Users/alex/Documents/codex_project/Nextion_project_PrintSphere
export IDF_TOOLS_PATH="$PWD/.tools/espressif"
export IDF_COMPONENT_MANAGER=0
. "$PWD/.tools/esp-idf-v6.0.1/export.sh"
idf.py -C examples/onx_bsp_smoke build
```

Firmware path after a successful build:

```text
examples/onx_bsp_smoke/build/onx_bsp_smoke.bin
```

Expected serial evidence:

- `I2C ready`
- `PCF8574 ready`
- `Backlight PWM ready`
- `LCD init complete`
- `LCD fill complete`
- `LCD color bars complete`
- `Touch init complete`
- `Touch sampler still running; waiting for touch`
- optional `Touch: points=... x=... y=...`
