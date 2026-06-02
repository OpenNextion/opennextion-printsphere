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

For the ONX BSP smoke project, Component Manager must be disabled because the
project uses only ESP-IDF components plus the local ONX BSP component:

```sh
cd /Users/alex/Documents/codex_project/Nextion_project_PrintSphere
export IDF_TOOLS_PATH="$PWD/.tools/espressif"
export IDF_COMPONENT_MANAGER=0
. "$PWD/.tools/esp-idf-v6.0.1/export.sh"
idf.py -C examples/onx_bsp_smoke build
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

3. Wait a few seconds and retry once. macOS may hold a short serial lock after a
   failed esptool or PySerial run.
4. Release DTR/RTS if the board appears held in reset:

   ```sh
   python3 -c 'import serial,time; s=serial.Serial("/dev/cu.wchusbserial10",115200,timeout=0.1); s.dtr=False; s.rts=False; time.sleep(0.5); s.close(); print("released DTR/RTS")'
   ```

5. Prefer the stable low-speed `/dev/tty.wchusbserial10` esptool command above.
6. If the board is stuck in download mode, run:

   ```sh
   cd /Users/alex/Documents/codex_project/Nextion_project_PrintSphere
   export IDF_TOOLS_PATH="$PWD/.tools/espressif"
   . "$PWD/.tools/esp-idf-v6.0.1/export.sh"
   python -m esptool --chip esp32s3 -p /dev/tty.wchusbserial10 run
   ```

7. If automatic reset fails:

   - Hold BOOT.
   - Press RESET once.
   - Release BOOT.
   - Run the stable low-speed flash command again.

8. If `The chip stopped responding` occurs during app write, do not leave the
   board in a partially flashed state. Re-run the stable low-speed flash command
   until the app write and verification complete.

9. If both `/dev/cu.*` and `/dev/tty.*` return readiness errors after multiple
   attempts, ask the user to unplug/replug USB or press RESET once, then retry.

10. If failures persist, stop and report branch, commit, target firmware,
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
