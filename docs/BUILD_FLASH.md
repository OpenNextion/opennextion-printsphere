# Build and Flash Flow

This file is the project source of truth for building, flashing, and monitoring. Threads must not invent alternate flows.

## Ownership Rules

- This file defines the only approved build, flash, and monitor flow for this workspace.
- Development threads must follow this flow and must not change ESP-IDF version, serial port, flash arguments, partition assumptions, or dependency-management policy on their own.
- If a development thread hits a build-base, serial, flash, reset, monitor, or driver issue, it must report the exact command and error back to the environment/serial/build maintenance thread for diagnosis.
- Business-code migration work and build/flash-base maintenance are separate concerns. Do not patch application logic to work around environment or flashing failures.
- Development threads may flash firmware for assigned hardware or code validation, but every flash must use the documented standard flow in this file.
- The Env/Serial/Flash thread owns flash-flow maintenance and fixes build-base, serial, reset, monitor, and flashing failures. The main thread owns cross-thread coordination and may stop or redirect flashing when firmware ownership conflicts.
- BSP, UI, App/Protocol, Build/Release, and QA threads must not invent private flash commands or use alternate ports, baud rates, partition tables, or ESP-IDF versions.
- Changes to this flow, serial endpoint policy, flash arguments, partition layout, or ESP-IDF version require main-thread review and Env/Serial/Flash thread input.
- If the documented flash flow fails, development threads must stop after
  collecting the exact command, port, branch, commit, and error output. They must
  hand the failure to the Env/Serial/Flash thread for diagnosis instead of trying
  alternate ports, baud rates, reset modes, esptool flags, or manual recovery
  sequences on their own.

## Standard Command Policy

Default rule:

- Build commands are allowed for development verification.
- Flash commands are allowed for assigned development validation when the executing thread follows this file exactly.
- Monitor/log capture commands are allowed for assigned validation, but must use short captures unless the Env/Serial/Flash thread explicitly approves an interactive monitor.
- Every flash attempt must identify the firmware target, purpose, branch, commit, project path, binary path, endpoint, and baud rate.
- Flash troubleshooting belongs to the Env/Serial/Flash thread. Other
  development threads should report failures; they should not experiment with
  alternate flash procedures.

## Approval / Escalation Policy For Flash And Monitor

Development threads may flash firmware for their assigned hardware or code
validation, but the permission to flash does not include permission to invent a
private flow. The standard ONX flash parameters are:

- Endpoint: `/dev/tty.wchusbserial10`.
- Baud rate: `115200`.
- Reset flags: `--before default-reset --after hard-reset`.
- Write operation: `write-flash @flash_args`.
- `@flash_args` must come from the just-built target build directory.

Approval pending, `auto_review` timeout, or a Codex escalation request that was
not executed is not a firmware, BSP, esptool, or serial-port failure. If a
standard flash or monitor escalation waits longer than about 2 minutes, or the
tool reports that automatic approval did not finish before its deadline, the
executing thread must stop and report:

- `approval_not_executed` or `approval_timeout`.
- Firmware target and build directory.
- Exact command that was waiting for approval.
- Preflight output, including branch, commit, serial node list, and `lsof`.

Do not respond to approval timeout by changing endpoint, baud rate, reset flags,
esptool arguments, `@flash_args`, partition layout, ESP-IDF version, macOS
settings, driver settings, or by repeatedly retrying the request. One clean
retry is acceptable only when the approval tool explicitly says a retry is
reasonable; after that, hand the case to Env/Serial/Flash or ask the user to
approve the pending action.

Only these classes belong in Env/Serial/Flash debugging:

- Serial owner shown by `lsof`.
- macOS permission denial such as `Operation not permitted`.
- Missing `/dev/cu.*` or `/dev/tty.*` node.
- WCH driver or DriverKit binding problems.
- Actual esptool connection, reset, write, verify, or chip-response errors.
- Monitor/capture commands that open the port but return no device logs.

For approval-friendly execution, split build and flash into separate shell
steps. First activate the environment and build. Then change to the build
directory. Then run a single direct esptool command:

```sh
cd /Users/alex/Documents/codex_project/Nextion_project_PrintSphere
export IDF_TOOLS_PATH="$PWD/.tools/espressif"
export IDF_COMPONENT_MANAGER=0
. "$PWD/.tools/esp-idf-v6.0.1/export.sh"
idf.py -C examples/onx_bsp_lvgl_smoke build
cd examples/onx_bsp_lvgl_smoke/build
```

```sh
python -m esptool --chip esp32s3 \
  -p /dev/tty.wchusbserial10 \
  -b 115200 \
  --before default-reset \
  --after hard-reset \
  write-flash @flash_args
```

If persistent approval is needed, ask for the narrowest practical command shape
for the standard esptool invocation. Prefer approving the standard
`python -m esptool ... write-flash @flash_args` usage, not a broad shell,
arbitrary Python script, arbitrary `python -c`, or unrestricted `/bin/zsh -lc`.
Do not request system-level or persistent approval from a development thread
unless Env/Serial/Flash or the user has explicitly asked for it.

Monitor and capture follow the same policy. Captures must be short, must include
an explicit timeout in the command, must close the serial object before exit,
and must be followed by `lsof /dev/cu.wchusbserial10
/dev/tty.wchusbserial10`. A capture approval timeout is also
`approval_not_executed`, not a firmware logging failure.

Before any flash attempt, record these facts in the executing thread:

```sh
git status --short
git branch --show-current
git log --oneline -1
ls -l /dev/cu.wchusbserial10 /dev/tty.wchusbserial10
lsof /dev/cu.wchusbserial10 /dev/tty.wchusbserial10
```

The `lsof` command must return no owner before flashing. If a serial monitor or
log reader is still attached, close it using the serial-session cleanup flow in
this document before running esptool.

Before flashing, the executing thread must state:

- Firmware project path.
- Firmware binary path.
- Purpose of the flash.
- Whether this is a smoke firmware, PrintSphere app firmware, or another image.
- Which serial endpoint and baud rate will be used.
- Expected post-flash evidence.

If environment facts are missing, stop before flashing and ask the
Env/Serial/Flash thread to resolve them. If firmware ownership is ambiguous
because another thread may be relying on the currently flashed image, ask the
main thread to coordinate ownership before flashing.

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

Standard project build from the workspace root:

```sh
cd /Users/alex/Documents/codex_project/Nextion_project_PrintSphere
export IDF_TOOLS_PATH="$PWD/.tools/espressif"
. "$PWD/.tools/esp-idf-v6.0.1/export.sh"
idf.py build
```

Standard ONX portrait app build:

```sh
cd /Users/alex/Documents/codex_project/Nextion_project_PrintSphere
export IDF_TOOLS_PATH="$PWD/.tools/espressif"
. "$PWD/.tools/esp-idf-v6.0.1/export.sh"
idf.py -DPRINTSPHERE_BOARD=onx3248g035 -DPRINTSPHERE_ONX_ORIENTATION=portrait build
```

ONX landscape app build uses the same board profile with an explicit
orientation profile. This build selects the `480 x 320` UI canvas, ST7796
`MADCTL=0xE8`, and the matching CST826 touch transform:

```sh
cd /Users/alex/Documents/codex_project/Nextion_project_PrintSphere
export IDF_TOOLS_PATH="$PWD/.tools/espressif"
. "$PWD/.tools/esp-idf-v6.0.1/export.sh"
idf.py -DPRINTSPHERE_BOARD=onx3248g035 -DPRINTSPHERE_ONX_ORIENTATION=landscape build
```

Always pass the orientation explicitly when switching between portrait and
landscape in the same checkout. CMake caches cache-string options inside the
current build directory, so relying on the implicit default after a previous
landscape build can reuse the cached landscape value.

## Release Build Version Gate

Public release firmware must not be published until the Build/Release or
Env/Serial/Flash owner has confirmed the standard release-version flow.

Before any public GitHub Release asset is uploaded, the executing thread must
record and verify:

- Intended public release tag, for example `v0.1.0`.
- Firmware asset filename and board/orientation, for example
  `opennextion-printsphere-onx3248g035-landscape-full-v0.1.0.bin`.
- CMake `PRINTSPHERE_RELEASE_VERSION`.
- CMake `PROJECT_VER`.
- Packager version argument and generated archive name.
- Any device-visible or log-visible firmware version text, if present.
- SHA256 of the final release asset.

All version fields must agree. A candidate firmware named `v0.1.0` while the
build, package metadata, or device-visible version still reports another value
such as `v1.6-beta1` is not releasable. Treat this as a Build/Release or
Env/Serial/Flash blocker and do not solve it by changing README or Release
notes text.

The Release/README planning thread may draft asset names and release notes, but
it must not change CMake release plumbing or declare a candidate binary ready
for public release. The main thread must review the version evidence before any
release asset is uploaded.

### Current v0.1.0 Release Recommendation

For the first OpenNextion-printsphere public release, use an explicit release
version build rather than renaming a candidate binary by hand.

The current root `CMakeLists.txt` defines:

```cmake
set(PRINTSPHERE_RELEASE_VERSION "v1.6-beta1")
set(PROJECT_VER "${PRINTSPHERE_RELEASE_VERSION}")
```

That ordinary `set(...)` assignment overrides a command-line cache value such
as `-DPRINTSPHERE_RELEASE_VERSION=v0.1.0`. Therefore `v0.1.0` release builds are
blocked until release-version plumbing is changed to allow an explicit release
version override, for example:

```cmake
set(PRINTSPHERE_RELEASE_VERSION "v1.6-beta1" CACHE STRING "PrintSphere release version")
set(PROJECT_VER "${PRINTSPHERE_RELEASE_VERSION}")
message(STATUS "PrintSphere release version: ${PRINTSPHERE_RELEASE_VERSION}")
```

After that plumbing is fixed, the standard ONX3248G035 landscape release build
should use an isolated build directory to avoid stale CMake cache values:

```sh
cd /Users/alex/Documents/codex_project/Nextion_project_PrintSphere
export IDF_TOOLS_PATH="$PWD/.tools/espressif"
. "$PWD/.tools/esp-idf-v6.0.1/export.sh"

idf.py -B build-release/onx3248g035-landscape-v0.1.0 \
  -DPRINTSPHERE_BOARD=onx3248g035 \
  -DPRINTSPHERE_ONX_ORIENTATION=landscape \
  -DPRINTSPHERE_RELEASE_VERSION=v0.1.0 \
  build release_initial_flash
```

The existing candidate file
`release/candidates/opennextion-printsphere-onx3248g035-landscape-full-v0.1.0.bin`
is not a final public release asset until this gate passes. Once the release
version is correctly wired, regenerate the final binary and record a fresh
SHA256 because the embedded project version changes the firmware image.

For the ONX BSP smoke project, Component Manager must be disabled because the
project uses only ESP-IDF components plus the local ONX BSP component:

```sh
cd /Users/alex/Documents/codex_project/Nextion_project_PrintSphere
export IDF_TOOLS_PATH="$PWD/.tools/espressif"
export IDF_COMPONENT_MANAGER=0
. "$PWD/.tools/esp-idf-v6.0.1/export.sh"
idf.py -C examples/onx_bsp_smoke build
```

For the ONX BSP LVGL smoke project, use the same project-local ESP-IDF
environment and disable Component Manager:

```sh
cd /Users/alex/Documents/codex_project/Nextion_project_PrintSphere
export IDF_TOOLS_PATH="$PWD/.tools/espressif"
export IDF_COMPONENT_MANAGER=0
. "$PWD/.tools/esp-idf-v6.0.1/export.sh"
idf.py -C examples/onx_bsp_lvgl_smoke build
```

For the ONX BSP board diagnostics project, use the same project-local ESP-IDF
environment and disable Component Manager:

```sh
cd /Users/alex/Documents/codex_project/Nextion_project_PrintSphere
export IDF_TOOLS_PATH="$PWD/.tools/espressif"
export IDF_COMPONENT_MANAGER=0
. "$PWD/.tools/esp-idf-v6.0.1/export.sh"
idf.py -C examples/onx_bsp_board_smoke build
```

PrintSphere and ONX examples use `idf_component.yml` dependencies, so their normal builds require ESP-IDF Component Manager.

Current verification status:

- ESP-IDF `hello_world` for `esp32s3` built successfully with `IDF_COMPONENT_MANAGER=0`; output included `build/hello_world.bin`, `build/bootloader/bootloader.bin`, and `build/partition_table/partition-table.bin`.
- Blank smoke-test build path used on 2026-06-02: `/private/tmp/onx_blank_flash_smoke_20260602`.
- ONX `01_touch_test` with `IDF_COMPONENT_MANAGER=0` failed as expected because `espressif__esp_lcd_touch` is an external managed component.
- PrintSphere and ONX dependency-managed builds must run with Component Manager enabled. If Codex sandbox denies `psutil` / `sysctl` process enumeration, rerun the same command outside the sandbox through the permission flow.
- PrintSphere full application build is not the baseline for environment
  acceptance. The baseline for build/flash-base acceptance is the blank
  ESP-IDF smoke-test project plus the ONX BSP smoke project.
- ONX BSP smoke build succeeded on 2026-06-02 with
  `IDF_COMPONENT_MANAGER=0`. Main-thread review after the final LCD
  configuration change produced
  `examples/onx_bsp_smoke/build/onx_bsp_smoke.bin`, size `0x3e050`.
- ONX BSP LVGL smoke build succeeded on 2026-06-02 with
  `IDF_COMPONENT_MANAGER=0`. Output binary:
  `examples/onx_bsp_lvgl_smoke/build/onx_bsp_lvgl_smoke.bin`, latest verified
  size `0x8c9c0`.
  Its generated `flash_args` includes `--flash-size 16MB`.

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
ls /dev/tty.wchusbserial*
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
- Current ONX dial-in node: `/dev/tty.wchusbserial10`.
- PySerial reports `USB VID:PID=1A86:7522 LOCATION=0-1`.

Endpoint policy:

- `/dev/cu.wchusbserial10` is useful for callout-style short serial log reads when it works.
- `/dev/tty.wchusbserial10` is the preferred endpoint for the stable low-speed CH340 esptool flashing path.
- Do not assume `/dev/cu.*` and `/dev/tty.*` behave identically on the macOS WCH DriverKit stack.
- If either endpoint disappears, stop and re-run USB/driver detection before any flash attempt.

## Flash

Do not flash any firmware until:

1. An ONX serial port exists under `/dev/cu.*` and `/dev/tty.*`.
2. The exact firmware target has been selected and built.
3. The thread running the flash command states which firmware will be flashed.
4. The thread confirms it is not unintentionally overwriting another thread's
   active hardware test firmware. If ownership is unclear, ask the main thread
   to coordinate before flashing.
5. `lsof` shows no unrelated process holding the serial port.

Current serial port:

```text
/dev/cu.wchusbserial10
/dev/tty.wchusbserial10
```

Do not use high-speed `idf.py flash` as the default for ONX CH340 operations in
this workspace. It has intermittently failed with:

```text
device reports readiness to read but returned no data
Could not exclusively lock port
The chip stopped responding
```

Stable ONX BSP smoke flash command:

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

Rationale:

- `/dev/cu.wchusbserial10` remains useful for normal callout-style serial reads.
- `/dev/tty.wchusbserial10` was the stable endpoint for low-speed direct esptool flashing after CH340 readiness and lock errors on `/dev/cu.wchusbserial10`.
- `@flash_args` comes from the just-built ESP-IDF project and prevents threads from hand-editing bootloader, partition, app offset, flash mode, flash frequency, or flash size.
- This is the standard ONX BSP smoke flash path. Do not replace it with `idf.py flash`, a higher baud rate, a different endpoint, or hand-written offsets unless this document is updated by Env/Serial/Flash.

Stable ONX BSP LVGL smoke flash command:

Build and enter the target build directory first:

```sh
cd /Users/alex/Documents/codex_project/Nextion_project_PrintSphere
export IDF_TOOLS_PATH="$PWD/.tools/espressif"
export IDF_COMPONENT_MANAGER=0
. "$PWD/.tools/esp-idf-v6.0.1/export.sh"
idf.py -C examples/onx_bsp_lvgl_smoke build
cd examples/onx_bsp_lvgl_smoke/build
```

Then run the single standard flash command:

```sh
python -m esptool --chip esp32s3 \
  -p /dev/tty.wchusbserial10 \
  -b 115200 \
  --before default-reset \
  --after hard-reset \
  write-flash @flash_args
```

Current ONX BSP LVGL smoke flash status:

- Build succeeded on 2026-06-02.
- Firmware: `examples/onx_bsp_lvgl_smoke/build/onx_bsp_lvgl_smoke.bin`.
- Latest verified binary size: `0x8c9c0`.
- Generated `flash_args`: `--flash-mode dio --flash-freq 80m --flash-size
  16MB`, followed by bootloader, partition table, and app offsets.
- Latest standard flash completed with bootloader, partition table, and app hash
  verification.
- BSP completed touch capture and reported the LVGL smoke touch mapping passed.
  Touch-coordinate details are owned by
  `docs/ONX3248G035_HARDWARE_CONFIG.md`.
- BSP thread's first standard flash attempt failed before changing any standard
  flow parameter because macOS/Codex returned `Operation not permitted` for
  `/dev/tty.wchusbserial10`.
- Env/Serial/Flash non-invasive checks after the report found both serial nodes
  present. An initial `lsof` check returned no owners, a later recheck found a
  transient Python process holding `/dev/cu.wchusbserial10`, and the process
  exited before a normal cleanup signal could be applied. Final `lsof
  /dev/cu.wchusbserial10 /dev/tty.wchusbserial10` returned no owners.
- Classify this failure path in order: first clear any serial owner reported by
  `lsof`; if both endpoints are free and esptool still reports `Operation not
  permitted`, treat it as a host permission or approval issue rather than a BSP
  source issue or a reason to change the standard flash command.

Stable ONX BSP board diagnostics flash command:

This command overwrites the currently flashed firmware and requires main-thread
or user approval when another validation image may be active on the board.

Build and enter the target build directory first:

```sh
cd /Users/alex/Documents/codex_project/Nextion_project_PrintSphere
export IDF_TOOLS_PATH="$PWD/.tools/espressif"
export IDF_COMPONENT_MANAGER=0
. "$PWD/.tools/esp-idf-v6.0.1/export.sh"
idf.py -C examples/onx_bsp_board_smoke build
cd examples/onx_bsp_board_smoke/build
```

Then run the single standard flash command:

```sh
python -m esptool --chip esp32s3 \
  -p /dev/tty.wchusbserial10 \
  -b 115200 \
  --before default-reset \
  --after hard-reset \
  write-flash @flash_args
```

Repeated ONX BSP smoke flash acceptance on 2026-06-02:

- Branch: `feature/onx-bsp-bringup`.
- Commit: `c42d17f Add labeled M2 smoke acceptance screens`.
- Working tree during acceptance: `docs/BUILD_FLASH.md` modified for this
  flow update.
- Firmware: `examples/onx_bsp_smoke/build/onx_bsp_smoke.bin`.
- Build result at that acceptance step: succeeded with
  `IDF_COMPONENT_MANAGER=0`; binary size `0x3db80`.
- Serial endpoints: `/dev/cu.wchusbserial10` and `/dev/tty.wchusbserial10`.
- Port ownership before flash: `lsof /dev/cu.wchusbserial10
  /dev/tty.wchusbserial10` returned no owners.
- Flash endpoint and baud: `/dev/tty.wchusbserial10`, `115200`.
- Standard command: `python -m esptool --chip esp32s3 -p
  /dev/tty.wchusbserial10 -b 115200 --before default-reset --after hard-reset
  write-flash @flash_args`.
- Result: 2 consecutive standard flashes succeeded without changing port, baud,
  reset flags, `@flash_args`, partition table, or ESP-IDF version.
- Both runs connected to ESP32-S3 revision v0.2, MAC `10:51:db:75:07:60`,
  wrote bootloader at `0x0`, partition table at `0x8000`, app at `0x10000`,
  verified hashes, and ended with hard reset via RTS.
- Failure or anomaly count: 0 during the two standard flashes.
- Post-flash log capture on `/dev/tty.wchusbserial10` at `115200` returned
  smoke firmware runtime logs, including `I (...) onx_bsp: Touch sampler still
  running; waiting for touch`.

Historical blank smoke-test flash command verified on 2026-06-02:

This records the first environment bring-up result only. It is not the current
standard ONX BSP smoke flash command.

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

If automatic download fails during a development-thread flash attempt, stop
after collecting the required evidence and report the failure to
Env/Serial/Flash. Manual BOOT/RESET recovery belongs to the Env/Serial/Flash
troubleshooting path below, unless that thread gives a specific one-off
instruction.

PrintSphere application firmware flash must state the selected app image,
branch, commit, purpose, expected evidence, and standard command before it runs.
If another thread may be relying on a different image on the board, ask the main
thread to coordinate firmware ownership before flashing. After ownership is
clear, development threads still use the same standard flow rather than a
private flash command.

## Monitor

Do not use long-running interactive monitor sessions as the default in Codex.
Use short PySerial captures for evidence. For the ONX BSP smoke firmware:

```sh
cd /Users/alex/Documents/codex_project/Nextion_project_PrintSphere
export IDF_TOOLS_PATH="$PWD/.tools/espressif"
. "$PWD/.tools/esp-idf-v6.0.1/export.sh"
python -c 'import serial,time,sys; ser=serial.Serial("/dev/tty.wchusbserial10",115200,timeout=0.2); end=time.time()+20; data=bytearray();
while time.time()<end:
    chunk=ser.read(4096)
    if chunk:
        data.extend(chunk)
ser.close(); sys.stdout.write(data.decode("utf-8", "replace"))'
```

If `/dev/tty.wchusbserial10` returns no boot logs, retry the same short capture
with `/dev/cu.wchusbserial10`. Do not leave monitor sessions running in the
background.

## Runtime Diagnostics Log Capture

Use this procedure when the assigned validation target is
`examples/onx_bsp_board_smoke`. It is a runtime log acceptance flow, so do not
run it until the main thread or user approves overwriting any currently active
firmware.

Preflight:

```sh
cd /Users/alex/Documents/codex_project/Nextion_project_PrintSphere
git status --short
git branch --show-current
git log --oneline -1
ls -l /dev/cu.wchusbserial10 /dev/tty.wchusbserial10
lsof /dev/cu.wchusbserial10 /dev/tty.wchusbserial10
```

The `lsof` command must return no owner before flashing or capturing logs. If a
serial owner is present, close it using the serial-session lifecycle below.

After approval, build and flash the board diagnostics firmware using the stable
standard command above. Then capture runtime logs with the standard endpoint and
baud rate:

```sh
cd /Users/alex/Documents/codex_project/Nextion_project_PrintSphere
export IDF_TOOLS_PATH="$PWD/.tools/espressif"
. "$PWD/.tools/esp-idf-v6.0.1/export.sh"
python -c 'import serial,time,sys; ser=serial.Serial("/dev/tty.wchusbserial10",115200,timeout=0.2); end=time.time()+25; data=bytearray();
while time.time()<end:
    chunk=ser.read(4096)
    if chunk:
        data.extend(chunk)
ser.close(); sys.stdout.write(data.decode("utf-8", "replace"))'
```

This capture must include at least one 5 second repeat loop from
`onx_board_smoke`. Expected runtime evidence includes:

- `ONX3248G035 board diagnostics smoke start`.
- `I2C bus ready evidence`.
- `PCF8574 raw state`.
- `PCF8574 pin map`.
- `CHG_N raw`.
- `TP_INT raw`.
- `SDCS raw`.
- `Battery ADC raw-only`.
- `Backlight check`.
- `Board diagnostics smoke complete`.
- `Board diagnostics still running`.
- `Battery ADC repeat raw-only`.

To avoid missing early boot logs, use one of these two standard approaches:

1. Immediately start the short capture command after the standard flash command
   finishes. This usually catches runtime loop evidence, but may miss the first
   boot lines if the board resets before the capture command opens the port.
2. For startup-log evidence, start the same short capture command first, then
   press the physical RESET button once within the first two seconds. This keeps
   the standard endpoint and baud rate unchanged and avoids changing flash
   reset flags or `@flash_args`.

If the output only shows ESP-IDF environment activation text and no serial
firmware logs, classify before changing anything:

- Command-wrapper issue: ESP-IDF activation text came from the host shell before
  Python opened the serial port; rerun the Python capture command alone after
  activation.
- Monitor not connected: Python exits with no firmware bytes and no serial
  exception; verify the command really opens `/dev/tty.wchusbserial10`.
- Serial occupied: `lsof` shows another process owning either ONX endpoint;
  close it using the serial-session lifecycle and retry once.
- Permission issue: Python raises `Operation not permitted`; use the host
  permission/Codex approval path in Flash Troubleshooting.
- Firmware log issue: serial opens cleanly, reset-synchronized capture returns
  bytes from the bootloader but no `onx_board_smoke` lines; hand the evidence to
  the BSP thread as a firmware logging or boot progression issue.
- Firmware not flashed: serial opens cleanly but logs identify another project,
  such as an LVGL smoke image; stop and ask the main thread whether diagnostics
  firmware overwrite is approved.

## Serial Session Lifecycle

Default rule:

- Use short PySerial captures for routine evidence. They must open the port,
  read for a bounded time, then call `ser.close()` before exiting.
- Do not keep `idf.py monitor`, `screen`, `minicom`, custom PySerial scripts, or
  IDE serial monitors open while editing code or before a new build/flash cycle.
- Before every flash, run `lsof` and require both ONX endpoints to be free.

Clean pre-flash close check:

```sh
lsof /dev/cu.wchusbserial10 /dev/tty.wchusbserial10
```

Expected clean result:

```text
<no output>
```

If a monitor is still open, close it from the owning terminal first:

- `idf.py monitor`: press `Ctrl+]`.
- `screen`: press `Ctrl+A`, then `\`, then confirm quit.
- `minicom`: press `Ctrl+A`, then `X`, then confirm exit.
- Python/PySerial script: press `Ctrl+C` if it is foregrounded; the script must
  close the serial object in normal exit paths.
- IDE serial monitor: click its disconnect/stop button before returning to the
  flash flow.

If the owning terminal is gone but `lsof` still shows a process, identify the
exact PID from `lsof` and terminate only that serial owner:

```sh
lsof /dev/cu.wchusbserial10 /dev/tty.wchusbserial10
kill <PID>
sleep 2
lsof /dev/cu.wchusbserial10 /dev/tty.wchusbserial10
```

Do not use `kill -9` as a routine cleanup step. If a normal `kill` does not free
the port, stop and hand the evidence to Env/Serial/Flash instead of changing the
flash command, endpoint, baud rate, or reset flags.

After a failed flash or a forced monitor close, wait a few seconds and re-run
the clean check before retrying:

```sh
sleep 3
lsof /dev/cu.wchusbserial10 /dev/tty.wchusbserial10
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

Do not use `idf.py flash monitor` for this project unless Env/Serial/Flash
explicitly approves it for a one-off diagnosis. It combines a hardware-mutating
operation with an interactive monitor and makes failures harder to classify.

## Flash Troubleshooting

This section is for Env/Serial/Flash recovery. Other development threads must
not improvise recovery steps. If the standard build, flash, or log-capture flow
fails, the development thread must stop after collecting evidence and hand the
failure to Env/Serial/Flash.

Required evidence from the failing thread:

- Branch and commit.
- Target firmware project and binary path.
- Complete command that failed.
- Complete error log.
- Serial port list:

  ```sh
  ls -l /dev/cu.wchusbserial10 /dev/tty.wchusbserial10
  ```

- Port ownership:

  ```sh
  lsof /dev/cu.wchusbserial10 /dev/tty.wchusbserial10
  ```

Known intermittent failures on this board/driver stack:

```text
device reports readiness to read but returned no data
Could not exclusively lock port
The chip stopped responding
Operation not permitted: '/dev/cu.wchusbserial10'
Operation not permitted: '/dev/tty.wchusbserial10'
```

For Env/Serial/Flash diagnosis, use this order:

1. Confirm both nodes exist:

   ```sh
   ls -l /dev/cu.wchusbserial10 /dev/tty.wchusbserial10
   ```

2. Confirm no process owns the port:

   ```sh
   lsof /dev/cu.wchusbserial10 /dev/tty.wchusbserial10
   ```

3. If the failure is `Operation not permitted` and `lsof` shows no owner, treat
   it as a host permission or Codex approval problem, not a flash-flow problem.
   Do not change endpoint, baud rate, reset flags, `@flash_args`, or partition
   layout. Re-run the same standard command outside the sandbox through the
   permission flow, or ask the user to approve terminal/Codex device access.
4. If macOS blocks serial access outside Codex too, open System Settings and
   check:

   - Privacy & Security > Files and Folders.
   - Privacy & Security > Full Disk Access.
   - Privacy & Security > Developer Tools.
   - Privacy & Security for any blocked driver or system extension prompt.

   After changing macOS privacy or driver approval, unplug/replug the ONX board
   and re-run the serial node plus `lsof` checks before flashing.
5. Wait a few seconds and retry once. macOS may hold a short serial lock after a
   failed esptool or PySerial run.
6. Release DTR/RTS if the board appears held in reset:

   ```sh
   python3 -c 'import serial,time; s=serial.Serial("/dev/cu.wchusbserial10",115200,timeout=0.1); s.dtr=False; s.rts=False; time.sleep(0.5); s.close(); print("released DTR/RTS")'
   ```

7. Prefer the stable low-speed `/dev/tty.wchusbserial10` esptool command above.
8. If the board is stuck in download mode, run:

   ```sh
   cd /Users/alex/Documents/codex_project/Nextion_project_PrintSphere
   export IDF_TOOLS_PATH="$PWD/.tools/espressif"
   . "$PWD/.tools/esp-idf-v6.0.1/export.sh"
   python -m esptool --chip esp32s3 -p /dev/tty.wchusbserial10 run
   ```

9. If automatic reset fails:

   - Hold BOOT.
   - Press RESET once.
   - Release BOOT.
   - Run the stable low-speed flash command again.

10. If `The chip stopped responding` occurs during app write, do not leave the
   board in a partially flashed state. Re-run the stable low-speed flash command
   until the app write and verification complete.

11. If both `/dev/cu.*` and `/dev/tty.*` return readiness errors after multiple
   attempts, ask the user to unplug/replug USB or press RESET once, then retry.

12. If failures persist, stop and report branch, commit, target firmware,
    complete command, serial port list, `lsof` output, and the full esptool or
    PySerial error log to the Env/Serial/Flash thread.

Do not fix flashing-base failures by changing firmware code, partition tables,
sdkconfig values, ESP-IDF version, or private flash commands.

## Prohibited Actions

Development threads must not:

- Change `.tools/esp-idf-v6.0.1` or introduce another ESP-IDF install.
- Change `IDF_TOOLS_PATH` away from `$PWD/.tools/espressif`.
- Use private build, flash, or monitor commands that are not documented here.
- Flash without stating firmware target, purpose, branch, commit, project path, binary path, endpoint, and baud rate.
- Use high-speed `idf.py flash` as the default ONX CH340 flashing path.
- Edit `@flash_args`, bootloader offset, partition-table offset, app offset, flash mode, flash frequency, or flash size by hand.
- Change partition tables or sdkconfig values to work around a flashing-base issue.
- Leave monitor, PySerial, screen, minicom, or other serial sessions running in the background.
- Start a new flash while `lsof /dev/cu.wchusbserial10 /dev/tty.wchusbserial10`
  still shows a serial owner.
- Use `kill -9` or broad process-kill commands as routine serial cleanup.
- Flash a firmware image outside the executing thread's assigned hardware or code validation scope.
- Flash when firmware ownership is unclear; ask the main thread to coordinate ownership conflicts first.

## Demo Smoke Test

For the demo project, the default smoke test is:

- Boot without panic.
- Screen shows expected LVGL content.
- Touch works.
- Backlight can be changed.
- Wi-Fi flow starts.
- No reset during a 10 to 30 minute demo run.

24 hour stability testing is not required for demo acceptance.
