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

Flash with the standard ONX BSP smoke flow:

```sh
cd /Users/alex/Documents/codex_project/Nextion_project_PrintSphere
export IDF_TOOLS_PATH="$PWD/.tools/espressif"
export IDF_COMPONENT_MANAGER=0
. "$PWD/.tools/esp-idf-v6.0.1/export.sh"
idf.py -C examples/onx_bsp_smoke build
cd examples/onx_bsp_smoke/build
python -m esptool --chip esp32s3 \
  -p /dev/tty.wchusbserial10 \
  -b 115200 \
  --before default-reset \
  --after hard-reset \
  write-flash @flash_args
```

Expected serial evidence:

- `I2C ready`
- `PCF8574 ready`
- `Backlight PWM ready`
- `LCD init complete`
- `M2 color acceptance page: verify RED/GREEN/BLUE/WHITE/BLACK labels`
- `LCD labeled color page complete: RED GREEN BLUE WHITE BLACK`
- `Touch init complete`
- `M2 touch acceptance page: verify TOP/BOTTOM/LEFT/RIGHT and tap TL, TR, BR, BL, CENTER`
- `LCD touch target page complete: TOP BOTTOM LEFT RIGHT X Y TL TR BR BL CENTER`
- `Touch sampler still running; waiting for touch`
- optional `Touch: points=... x=... y=... target=...`

Manual acceptance:

1. Confirm the color page shows five labeled blocks: `RED`, `GREEN`, `BLUE`,
   `WHITE`, and `BLACK`.
2. Confirm the labels are readable: black text on bright blocks and white text
   on dark blocks.
3. Confirm the color labels match the visible colors and white/black are not
   inverted.
4. After the firmware switches to the touch page, confirm the direction labels:
   `TOP`, `BOTTOM`, `LEFT`, `RIGHT`, `X`, and `Y`.
5. Tap the marked targets:
   `TL`, `TR`, `BR`, `BL`, and `CENTER`.
6. Check serial output for matching `target=TL/TR/BR/BL/CENTER` touch lines.
