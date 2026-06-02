# Build and Flash Flow

This file is the project source of truth for building, flashing, and monitoring. Threads must not invent alternate flows.

## Ownership Rules

- This file defines the only approved build, flash, and monitor flow for this workspace.
- Development threads must follow this flow and must not change ESP-IDF version, serial port, flash arguments, partition assumptions, or dependency-management policy on their own.
- If a development thread hits a build-base, serial, flash, reset, monitor, or driver issue, it must report the exact command and error back to the environment/serial/build maintenance thread for diagnosis.
- Business-code migration work and build/flash-base maintenance are separate concerns. Do not patch application logic to work around environment or flashing failures.

## Board Target

Target board:

- ONX3248G035
- ESP32-S3
- 16 MB Flash
- 8 MB PSRAM

ESP-IDF target:

```sh
idf.py set-target esp32s3
```

## Environment Activation

Use the project-local ESP-IDF install:

```sh
cd /Users/alex/Documents/codex_project/Nextion_project_PrintSphere
export IDF_TOOLS_PATH="$PWD/.tools/espressif"
. "$PWD/.tools/esp-idf-v6.0.1/export.sh"
```

Confirm:

```sh
idf.py --version
cmake --version
ninja --version
```

Expected versions in this workspace:

```text
ESP-IDF v6.0.1
cmake version 4.0.3
ninja 1.12.1
```

## Build

Standard project build:

```sh
idf.py build
```

For the blank hardware smoke-test project only, disable Component Manager because the project has no managed dependencies and this avoids Codex sandbox process-inspection limits:

```sh
export IDF_COMPONENT_MANAGER=0
idf.py set-target esp32s3
idf.py build
```

PrintSphere and ONX examples use `idf_component.yml` dependencies, so their normal builds require ESP-IDF Component Manager.

Current verification status:

- ESP-IDF `hello_world` for `esp32s3` built successfully with `IDF_COMPONENT_MANAGER=0`; output included `build/hello_world.bin`, `build/bootloader/bootloader.bin`, and `build/partition_table/partition-table.bin`.
- Blank smoke-test build path used on 2026-06-02: `/private/tmp/onx_blank_flash_smoke_20260602`.
- ONX `01_touch_test` with `IDF_COMPONENT_MANAGER=0` failed as expected because `espressif__esp_lcd_touch` is an external managed component.
- PrintSphere and ONX dependency-managed builds must run with Component Manager enabled. If Codex sandbox denies `psutil` / `sysctl` process enumeration, rerun the same command outside the sandbox through the permission flow.
- PrintSphere full application build is not the baseline for environment acceptance. The baseline for build/flash-base acceptance is the blank ESP-IDF smoke-test project.

Blank smoke-test build command:

```sh
cp -R .tools/esp-idf-v6.0.1/examples/get-started/hello_world /private/tmp/onx_blank_flash_smoke_20260602
cd /private/tmp/onx_blank_flash_smoke_20260602
export IDF_TOOLS_PATH=/Users/alex/Documents/codex_project/Nextion_project_PrintSphere/.tools/espressif
export IDF_COMPONENT_MANAGER=0
. /Users/alex/Documents/codex_project/Nextion_project_PrintSphere/.tools/esp-idf-v6.0.1/export.sh
idf.py set-target esp32s3
idf.py build
```

## Serial Port Detection

```sh
ls /dev/cu.*
```

Expected ONX serial names may include:

```text
/dev/cu.wchusbserial*
/dev/cu.usbserial*
/dev/cu.usbmodem*
```

Current status:

- macOS USB registry sees the board as `USB Serial`, VID/PID `0x1A86:0x7522`.
- WCH DriverKit extension `cn.wch.CH34xVCPDriver` is installed and bound.
- Current ONX serial node: `/dev/cu.wchusbserial10`.
- PySerial reports `USB VID:PID=1A86:7522 LOCATION=0-1`.

## Flash

Do not flash application firmware until:

1. An ONX serial port exists under `/dev/cu.*`.
2. The exact firmware target has been selected and built.
3. The thread running the flash command states which firmware will be flashed.

Current serial port:

```text
/dev/cu.wchusbserial10
```

Replace `<PORT>` with the ONX serial port:

```sh
idf.py -p <PORT> flash
```

Example:

```sh
idf.py -p /dev/cu.wchusbserial10 flash
```

Blank smoke-test flash command verified on 2026-06-02:

```sh
cd /private/tmp/onx_blank_flash_smoke_20260602
export IDF_TOOLS_PATH=/Users/alex/Documents/codex_project/Nextion_project_PrintSphere/.tools/espressif
export IDF_COMPONENT_MANAGER=0
. /Users/alex/Documents/codex_project/Nextion_project_PrintSphere/.tools/esp-idf-v6.0.1/export.sh
idf.py -p /dev/cu.wchusbserial10 flash
```

Verified blank smoke-test flash result:

- esptool connected to ESP32-S3 on `/dev/cu.wchusbserial10`.
- Detected chip: ESP32-S3 QFN56 revision v0.2.
- Detected features: Wi-Fi, BT 5 LE, dual core, 240 MHz, embedded PSRAM 8 MB.
- MAC: `10:51:db:75:07:60`.
- Wrote and verified:
  - `bootloader/bootloader.bin` at `0x0`.
  - `partition_table/partition-table.bin` at `0x8000`.
  - `hello_world.bin` at `0x10000`.
- Final reset: hard reset via RTS pin.

Codex sandbox note:

- Plain sandbox execution can build the blank smoke-test project.
- Plain sandbox execution cannot open `/dev/cu.wchusbserial10`; flash/monitor must run outside the sandbox through the permission flow.

If automatic download fails:

1. Hold BOOT.
2. Press RESET once.
3. Release BOOT.
4. Run the flash command again.

Application firmware flash has not been attempted yet because the intended application image has not been confirmed.

## Monitor

```sh
idf.py -p <PORT> monitor
```

`idf.py monitor` requires an interactive TTY. In non-interactive Codex execution, use a short PySerial read for smoke-test evidence:

```sh
cd /private/tmp/onx_blank_flash_smoke_20260602
export IDF_TOOLS_PATH=/Users/alex/Documents/codex_project/Nextion_project_PrintSphere/.tools/espressif
. /Users/alex/Documents/codex_project/Nextion_project_PrintSphere/.tools/esp-idf-v6.0.1/export.sh
python -c 'import serial,time,sys; ser=serial.Serial("/dev/cu.wchusbserial10",115200,timeout=0.2); end=time.time()+12; data=bytearray();
while time.time()<end:
    chunk=ser.read(4096)
    if chunk:
        data.extend(chunk)
ser.close(); sys.stdout.write(data.decode("utf-8", "replace"))'
```

Verified monitor evidence from the blank smoke-test firmware:

- Bootloader: `ESP-IDF v6.0.1 2nd stage bootloader`.
- App: `Project name: hello_world`.
- App log: `Hello world!`.
- Chip log: `esp32s3 chip with 2 CPU core(s), WiFi/BLE, silicon revision v0.2`.
- Restart countdown was visible on serial output.

Exit monitor:

```text
Ctrl+]
```

## Combined Flash and Monitor

```sh
idf.py -p <PORT> flash monitor
```

## Demo Smoke Test

For the demo project, the default smoke test is:

- Boot without panic.
- Screen shows expected LVGL content.
- Touch works.
- Backlight can be changed.
- Wi-Fi flow starts.
- No reset during a 10 to 30 minute demo run.

24 hour stability testing is not required for demo acceptance.
