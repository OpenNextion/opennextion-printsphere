# Thread Roles

This project uses the current main thread as the coordinator. Subthreads are advisory or implementation workers for specific areas. They do not communicate with each other automatically.

## Main Thread

Responsibilities:

- Own project direction and final decisions.
- Dispatch tasks to subthreads.
- Read and summarize subthread output.
- Maintain project documentation.
- Own Git operations and integration.
- Decide when a change is accepted.

Rules:

- The main thread is the only place where cross-cutting configuration decisions are made.
- The main thread owns branch naming, commits, and merge order.
- Subthreads should not independently change ESP-IDF version, partition layout, board profile strategy, or release structure.

## BSP / Hardware Thread

Scope:

- ONX3248G035 ST7796 display.
- CST826 touch.
- PCF8574.
- Backlight PWM.
- I2C/SPI pinout.
- ADC battery and charge status.

Boundaries:

- Should keep changes inside board component and board config where possible.
- Should not modify Bambu protocol logic.
- Should not redesign UI layout beyond what is needed for hardware testing.

## UI / Layout Thread

Scope:

- 320 x 480 ONX layout.
- LVGL page structure.
- Status page, cover page, camera page, AMS page simplification.
- Text fitting and touch interactions.

Boundaries:

- Should not change printer protocol parsing.
- Should not modify board drivers.
- Should target demo usability before full feature parity.

## Build / Release Thread

Scope:

- ESP-IDF version strategy.
- Component dependencies.
- CMake and sdkconfig.
- Partition table.
- Build, flash, and packaging commands.

Boundaries:

- Should not switch global toolchain version without main-thread approval.
- Should not create alternate build flows.
- Should maintain `docs/BUILD_FLASH.md` as the source of truth.

## App / Protocol Thread

Scope:

- Bambu Cloud.
- Local MQTT.
- Hybrid routing.
- Web Config behavior.
- NVS config schema.
- Memory pressure from TLS, MQTT, HTTP, and image decode.

Boundaries:

- Should not make hardware-specific decisions.
- Should not log credentials or tokens.
- Should preserve existing PrintSphere behavior unless the demo requires a narrower path.

## QA / Hardware Test Thread

Scope:

- Demo smoke test matrix.
- Bring-up validation.
- Serial log requirements.
- Build/flash verification.

Boundaries:

- This is a demo project. Do not require 24 hour tests for first demo milestones.
- Prefer short targeted checks with clear pass/fail criteria.

## Coordination Rule

When subthreads finish, the main thread must explicitly read their outputs and summarize them. Subthread completion does not automatically notify the main thread.
