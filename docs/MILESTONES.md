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

Current status:

- Started on 2026-06-02.
- Branch: `feature/onx-bsp-bringup`.
- Owner thread: BSP / Hardware, `019e828d-285b-7722-906a-f9baf8047316`.
- Main-thread acceptance requires evidence from build, flash, and serial logs.
- Minimal BSP smoke firmware added at `examples/onx_bsp_smoke`.
- Build and flash passed on 2026-06-02.
- Serial logs prove I2C, PCF8574, GPIO6 backlight PWM, ST7796 init, LCD color fills, and CST826 chip ID read.
- Four-corner touch capture passed on 2026-06-02; no swap or mirror correction is needed.
- Enhanced labeled smoke firmware built and flashed on 2026-06-02 for user-visible color and touch target acceptance.
- Final user visual acceptance passed on 2026-06-02: color labels, white/black
  inversion, readable text orientation, and touch label positions are correct.
- Final LCD hardware configuration is recorded in
  `docs/ONX3248G035_HARDWARE_CONFIG.md`.
- M2 is accepted for demo-first BSP bring-up. Remaining hardware features such
  as SD, battery ADC, charge status, and LVGL integration move to later
  PrintSphere integration milestones.

Tasks:

- Build and flash a minimal ONX test firmware. Done with `examples/onx_bsp_smoke`.
- Initialize ST7796 LCD. Done in BSP smoke logs.
- Fill screen with test colors. Done in BSP smoke logs.
- Show labeled color blocks. Done and visually accepted by the user.
- Initialize CST826 touch. Done; chip ID `0x11` read from address `0x15`.
- Verify four-corner touch coordinates. Done; coordinates map left-to-right and top-to-bottom.
- Show labeled touch targets. Done and visually accepted by the user.
- Verify GPIO6 backlight PWM. Done in BSP smoke logs for 30% and 100% duty settings.

Exit criteria:

- Screen displays correct colors.
- Touch coordinates map correctly.
- Brightness changes visibly.
- Serial logs show stable smoke runtime evidence for demo-first acceptance.

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
