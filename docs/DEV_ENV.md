# Development Environment

## Host

- Host OS: macOS
- Workspace: `/Users/alex/Documents/codex_project/Nextion_project_PrintSphere`
- Shell: zsh
- Git identity:
  - Name: `Alex`
  - Email: `freezing.xie@gmail.com`

## Current Detection

Detected:

- Git: Apple Git 2.50.1
- Git repository status: initialized on 2026-06-02 after approving `git init` outside the Codex sandbox.
- Xcode Command Line Tools: `/Library/Developer/CommandLineTools`
- Homebrew: `/opt/homebrew/bin/brew`
- Python: Python 3.10.8
- ESP-IDF: project-local install at `.tools/esp-idf-v6.0.1`
- ESP-IDF tools path: `.tools/espressif`
- `idf.py --version`: ESP-IDF v6.0.1
- `cmake --version`: 4.0.3
- `ninja --version`: 1.12.1
- `xtensa-esp32s3-elf-gcc --version`: 15.2.0

USB serial currently detected:

- `/dev/cu.Bluetooth-Incoming-Port`
- `/dev/cu.debug-console`
- `/dev/cu.wchusbserial10`

ONX/CH340 serial device is currently visible. Expected device names may include:

- `/dev/cu.wchusbserial*`
- `/dev/cu.usbserial*`
- `/dev/cu.usbmodem*`

USB registry detail:

- macOS sees a USB device named `USB Serial`.
- Vendor ID: `6790` decimal, `0x1A86` hex.
- Product ID: `29986` decimal, `0x7522` hex.
- Driver binding: `cn.wch.CH34xVCPDriver` DriverKit system extension.
- IORegistry serial client: `IOSerialBSDClient`.
- Callout device: `/dev/cu.wchusbserial10`.
- Dial-in device: `/dev/tty.wchusbserial10`.

Interpretation:

- The board or USB-UART bridge is physically visible to macOS.
- The CH34x serial driver is installed and bound.
- The ONX serial port is available as `/dev/cu.wchusbserial10`.
- Plain Codex sandbox commands can list the serial node but cannot open it for flash/monitor; serial flash and monitor commands must run outside the sandbox through the permission flow.

## Required Toolchain

Project toolchain:

- ESP-IDF: v6.0.1.
- LVGL: keep the PrintSphere LVGL 9 direction.
- Python: 3.10+ is acceptable.
- CMake and Ninja: required by ESP-IDF.

Version decision:

- `main/idf_component.yml` requires `idf.version >=6.0.0`.
- `README.md` still mentions ESP-IDF v5.5.x.
- ONX official examples mention ESP-IDF 5.4.1 / 5.4.0+.
- The unified environment is ESP-IDF v6.0.1 because the current PrintSphere component manifest is the strongest source for this workspace.

Activation:

```sh
cd /Users/alex/Documents/codex_project/Nextion_project_PrintSphere
export IDF_TOOLS_PATH="$PWD/.tools/espressif"
. "$PWD/.tools/esp-idf-v6.0.1/export.sh"
```

If Python package downloads fail with a certificate verification error, use the local certifi CA bundle:

```sh
export SSL_CERT_FILE=/Library/Frameworks/Python.framework/Versions/3.10/lib/python3.10/site-packages/certifi/cacert.pem
export REQUESTS_CA_BUNDLE="$SSL_CERT_FILE"
```

## Driver Notes

ONX3248G035 uses a CH340K USB-to-UART path according to the official resources. If no serial device appears:

1. Confirm the USB-C cable supports data.
2. Try another USB port or adapter.
3. Install the CH340/CH341 macOS driver from the official ONX resource package or WCH.
4. Reboot or reconnect the board if macOS asks for driver approval.
5. Check `/dev/cu.*` again.

On macOS, after installing a driver, also check:

- System Settings > Privacy & Security for blocked system extension or driver approval prompts.
- System Settings > Privacy & Security > Driver Extensions, if present.
- Reconnect the board after approval.

Local ONX driver package:

```text
/private/tmp/onx-ch341-driver/CH341SER_MAC/CH34xVCPDriver.pkg
```

Install command, requiring administrator approval:

```sh
sudo installer -pkg /private/tmp/onx-ch341-driver/CH341SER_MAC/CH34xVCPDriver.pkg -target /
```

Status:

- The package was extracted from `/private/tmp/ONX3248G035-readonly/USB-to-Serial Driver/CH341SER_MAC.ZIP`.
- `pkgutil --pkgs='com.wch*'` found no installed WCH package before installation.
- `/Library/Extensions` did not show a WCH CH34x driver before installation.
- The sudo installer request was attempted from Codex but timed out waiting for approval.
- User manually completed installation on 2026-06-02.
- Installed driver path: `/Library/SystemExtensions/381B61F2-4643-4988-9A67-67A6F68F22CE/cn.wch.CH34xVCPDriver.dext`.
- Runtime driver identifier: `cn.wch.CH34xVCPDriver`.
- Current serial node: `/dev/cu.wchusbserial10`.
- `python -m serial.tools.list_ports -v` reports `USB VID:PID=1A86:7522 LOCATION=0-1`.
- `systemextensionsctl list` returns `OSSystemExtensionErrorDomain error 1` in this Codex context, but IORegistry confirms the DriverKit extension is active and bound.

## Homebrew Notes

Codex currently cannot write Homebrew cache files under:

```text
/Users/alex/Library/Caches/Homebrew/api/
```

This can make `brew list` or install commands fail from inside Codex unless permissions are granted or the command is approved outside the sandbox.

## Environment Setup Policy

Only one ESP-IDF installation and one documented build flow should be used for this project. Subthreads must not install alternate ESP-IDF versions or use their own private build commands without updating `docs/BUILD_FLASH.md` through the main thread.

## Approval And Escalation Policy

Development threads may flash firmware for assigned hardware or code
validation, but only through the standard flow in `docs/BUILD_FLASH.md`.

For ONX serial flash, the standard remains:

- Endpoint: `/dev/tty.wchusbserial10`.
- Baud: `115200`.
- Reset flags: `--before default-reset --after hard-reset`.
- Write operation: `write-flash @flash_args`.

Codex `auto_review`, approval pending, or approval timeout means the command was
not executed. It is not a firmware failure, BSP failure, esptool failure, or
serial-driver failure. If approval waits longer than about 2 minutes or times
out, the executing thread should stop and report `approval_not_executed` with
the exact command and preflight evidence. It should not change ports, baud rate,
reset flags, flash arguments, ESP-IDF version, partition layout, macOS privacy
settings, or driver settings.

Flash commands should be written as narrow, approval-friendly steps: activate
ESP-IDF, build, `cd` into the build directory, then run one direct
`python -m esptool ... write-flash @flash_args` command. Persistent approval, if
needed, should be narrow to the standard esptool usage rather than a broad shell
or arbitrary Python command.

Monitor and capture approvals follow the same rule. Capture commands must be
short, have an explicit timeout, close the serial port before exit, and leave
`lsof /dev/cu.wchusbserial10 /dev/tty.wchusbserial10` empty afterward.

## Standard Checks

Run these before building:

```sh
git status --short
python3 --version
export IDF_TOOLS_PATH="$PWD/.tools/espressif"
. "$PWD/.tools/esp-idf-v6.0.1/export.sh"
idf.py --version
cmake --version
ninja --version
ls /dev/cu.*
ls /dev/tty.wchusbserial*
lsof /dev/cu.wchusbserial10 /dev/tty.wchusbserial10
```

Before flashing, `lsof` should return no owner for either ONX serial endpoint.
If a prior monitor or log reader is still attached, close it using the serial
session lifecycle in `docs/BUILD_FLASH.md` before running esptool.

## Blank Build/Flash Smoke Test

The environment baseline was verified on 2026-06-02 with the ESP-IDF `hello_world` example copied to:

```text
/private/tmp/onx_blank_flash_smoke_20260602
```

Verified results:

- Target: `esp32s3`.
- Port: `/dev/cu.wchusbserial10`.
- Build: succeeded with ESP-IDF v6.0.1, CMake 4.0.3, Ninja 1.12.1.
- Firmware image: `/private/tmp/onx_blank_flash_smoke_20260602/build/hello_world.bin`.
- Bootloader image: `/private/tmp/onx_blank_flash_smoke_20260602/build/bootloader/bootloader.bin`.
- Partition table: `/private/tmp/onx_blank_flash_smoke_20260602/build/partition_table/partition-table.bin`.
- Flash: succeeded outside the sandbox via esptool.
- Monitor evidence: serial output showed `ESP-IDF v6.0.1 2nd stage bootloader`, `Project name: hello_world`, and `Hello world!`.

This smoke test verifies the host toolchain, WCH serial driver, serial permissions via approved external execution, ESP32-S3 download/reset flow, and serial output path.

## ONX BSP Smoke Build/Flash Acceptance

The project-local ONX BSP smoke flow was verified on 2026-06-02.

Verified results:

- Project: `/Users/alex/Documents/codex_project/Nextion_project_PrintSphere/examples/onx_bsp_smoke`.
- Branch: `feature/onx-bsp-bringup`.
- Commit: `c42d17f Add labeled M2 smoke acceptance screens`.
- Environment: `.tools/esp-idf-v6.0.1` with
  `IDF_TOOLS_PATH=$PWD/.tools/espressif`.
- Build command: `IDF_COMPONENT_MANAGER=0 idf.py -C examples/onx_bsp_smoke build`.
- Firmware image: `examples/onx_bsp_smoke/build/onx_bsp_smoke.bin`.
- Binary size during repeated flash acceptance: `0x3db80`.
- Main-thread review build after the final LCD configuration change: `0x3e050`.
- Flash endpoint: `/dev/tty.wchusbserial10`.
- Flash baud: `115200`.
- Flash method: direct `python -m esptool ... write-flash @flash_args`.
- Repeated flash result: 2 consecutive standard flashes succeeded, each with
  bootloader, partition table, and app hash verification.
- Log capture: short PySerial read at `115200` returned ONX BSP smoke runtime
  logs, including `onx_bsp: Touch sampler still running; waiting for touch`.

## ONX BSP LVGL Smoke Build/Flash Status

The ONX BSP LVGL smoke build flow was verified on 2026-06-02.

Verified build results:

- Project: `/Users/alex/Documents/codex_project/Nextion_project_PrintSphere/examples/onx_bsp_lvgl_smoke`.
- Environment: `.tools/esp-idf-v6.0.1` with
  `IDF_TOOLS_PATH=$PWD/.tools/espressif`.
- Build command: `IDF_COMPONENT_MANAGER=0 idf.py -C examples/onx_bsp_lvgl_smoke build`.
- Firmware image: `examples/onx_bsp_lvgl_smoke/build/onx_bsp_lvgl_smoke.bin`.
- Latest verified binary size: `0x8c9c0`.
- Generated flash args include `--flash-size 16MB`.

Current flash status:

- Standard flow remains `/dev/tty.wchusbserial10`, `115200`,
  `python -m esptool ... write-flash @flash_args`.
- Latest standard flash completed with bootloader, partition table, and app hash
  verification.
- BSP completed touch capture and reported the LVGL smoke touch mapping passed.
  Touch-coordinate details are owned by
  `docs/ONX3248G035_HARDWARE_CONFIG.md`.
- BSP thread's first standard flash attempt hit `Operation not permitted` on
  `/dev/tty.wchusbserial10`.
- Env/Serial/Flash non-invasive checks confirmed both serial nodes exist. A
  later recheck found a transient Python process holding
  `/dev/cu.wchusbserial10`; it exited before normal cleanup was needed, and the
  final `lsof /dev/cu.wchusbserial10 /dev/tty.wchusbserial10` returned no owner.
- Failure classification order: first clear any `lsof` serial owner; if both
  endpoints are free and esptool still reports `Operation not permitted`,
  classify it as a host permission or Codex approval issue, not a BSP source
  issue and not a reason to change the standard flash command.

Development threads may flash for assigned hardware or code validation, but
they must use the standard flow in `docs/BUILD_FLASH.md`. If the standard flow
fails, collect the required evidence and hand the failure to Env/Serial/Flash
instead of changing ports, baud rate, reset flags, flash arguments, partition
layout, or ESP-IDF version.

## ONX BSP Board Diagnostics Capture Status

The M4 board diagnostics runtime log capture flow was executed on 2026-06-02.

Current constraints:

- Do not flash or monitor until the main thread or user approves overwriting the
  currently active validation firmware on the board.
- Standard diagnostics target:
  `examples/onx_bsp_board_smoke`.
- Standard flash endpoint remains `/dev/tty.wchusbserial10`.
- Standard baud remains `115200`.
- Standard flash method remains direct `python -m esptool ... write-flash
  @flash_args` from `examples/onx_bsp_board_smoke/build`.

Current read-only serial check:

- `/dev/cu.wchusbserial10` exists.
- `/dev/tty.wchusbserial10` exists.
- `lsof /dev/cu.wchusbserial10 /dev/tty.wchusbserial10` returned no owner during
  the capture-plan check.

Runtime log capture must follow `docs/BUILD_FLASH.md` and should collect both
startup evidence and at least one 5 second `onx_board_smoke` loop.
