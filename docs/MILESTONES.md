# Milestones

## M0 - Project Bootstrap

Goal: Establish source baseline, documentation, Git ownership, and operating rules.

Tasks:

- Import PrintSphere upstream source into the project workspace.
- Initialize Git.
- Add upstream remote when a canonical fork/remote target is chosen.
- Create a baseline commit for the imported upstream snapshot.
- Add project context and collaboration documents.
- Define demo-first scope and excluded production work.

Exit criteria:

- `git status` works in the workspace.
- A baseline commit exists.
- Project documents exist under `docs/`.

Current status:

- Source imported.
- Git initialized.
- Environment and build/flash rules documented.
- Baseline commit created.

## M1 - Mac Development Environment

Goal: Make this Mac able to build, flash, and monitor ONX3248G035.

Tasks:

- Confirm USB serial device appears under `/dev/cu.*`.
- Install CH340/CH341 driver if needed.
- Install or activate ESP-IDF.
- Confirm `idf.py`, `python3`, `cmake`, and `ninja`.
- Record one standard build/flash/monitor flow.

Exit criteria:

- `idf.py --version` works.
- `cmake --version` works.
- `ninja --version` works.
- ONX serial port is visible.

## M2 - ONX Board Bring-Up

Goal: Validate the hardware before integrating PrintSphere.

Tasks:

- Build and flash a minimal ONX test firmware.
- Initialize ST7796 LCD.
- Fill screen with test colors.
- Initialize CST826 touch.
- Verify four-corner touch coordinates.
- Verify GPIO6 backlight PWM.

Exit criteria:

- Screen displays correct colors.
- Touch coordinates map correctly.
- Brightness changes visibly.
- Serial logs are stable for 10 minutes.

## M3 - PrintSphere Starts on ONX BSP

Goal: Make PrintSphere boot with an ONX board profile.

Tasks:

- Add ONX board component or profile.
- Provide PrintSphere-compatible display and touch APIs.
- Replace or disable AXP2101 PMU dependency for ONX.
- Start LVGL with a minimal page.

Exit criteria:

- Firmware builds for ONX.
- Firmware boots on ONX.
- LVGL page appears.
- Touch events reach the UI.

## M4 - 320 x 480 Demo UI

Goal: Build a usable demo UI layout for ONX.

Tasks:

- Replace 466 x 466 layout assumptions.
- Create a 320 x 480 status page.
- Show printer state, progress, temperature, remaining time, Wi-Fi, and power status.
- Keep page navigation simple.

Exit criteria:

- Core demo page has no overlapping text.
- UI remains responsive during normal updates.
- Demo runs for 10 to 30 minutes.

## M5 - Wi-Fi and Web Config

Goal: Restore configuration flow on ONX.

Tasks:

- Verify setup AP.
- Verify Wi-Fi save and reconnect.
- Verify Web Config access on home network.
- Keep sensitive values out of logs.

Exit criteria:

- Device can join home Wi-Fi.
- Web Config loads from device IP.
- Config persists across reboot.

## M6 - Bambu Basic Status Demo

Goal: Display real printer status.

Tasks:

- Validate Cloud only or Local only first.
- Add Hybrid after one path is stable.
- Show live printer status on the ONX UI.

Exit criteria:

- One real Bambu printer can connect.
- Idle, printing, paused, and completed states display correctly.
- Short demo run passes without crash.

## M7 - Demo Packaging

Goal: Produce repeatable demo build and flash instructions.

Tasks:

- Document exact build command.
- Document exact flash command.
- Document serial monitor command.
- Record known limitations.

Exit criteria:

- Another thread can follow the docs without inventing a different flow.
