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

Current assignment:

- Thread id: `019e828d-285b-7722-906a-f9baf8047316`.
- Active branch: `feature/onx-bsp-bringup`.
- Status: implementation worker for M2 ONX board bring-up.
- Deliverable: a minimal ONX hardware smoke firmware and BSP layer proving LCD, touch, and backlight basics before PrintSphere app integration.

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
- Must follow `docs/DEV_ENV.md` and `docs/BUILD_FLASH.md`.
- Must not change ESP-IDF version, serial port, flash parameters, or build flow without main-thread approval.
- Must state the exact firmware path and purpose before flashing.

## UI / Layout Thread

Scope:

- 320 x 480 ONX layout.
- LVGL page structure.
- Status page, cover page, camera page, AMS page simplification.
- Text fitting and touch interactions.
- HTML visual preview and draft UI design spec before implementation.
- Resource reuse policy: prefer existing PrintSphere fonts, Material Design Icons subsets, and embedded Bambu image before adding new bitmap/icon assets.
- Visual implementation constraints and interaction contract in `docs/UI_DESIGN_SPEC.md`.
- Interaction ownership: page availability, pager behavior, brightness gesture, long-press PIN, printer card switching, camera refresh, chamber-light logo toggle, remaining/ETA toggle, and no-op boundaries for display-only controls.
- Porting architecture boundaries and implementation slices in `docs/PORTING_ARCHITECTURE_BOUNDARIES.md`.

Boundaries:

- Should not change printer protocol parsing.
- Should not modify board drivers.
- Should target demo usability before full feature parity.
- The visual direction has been approved; UI implementation threads must follow `docs/UI_DESIGN_SPEC.md` for layout, dimensions, colors, fonts, page flow, and interaction behavior.
- UI implementation threads must not invent new buttons, printer controls, AMS actions, settings entries, or protocol fields outside the spec.
- If original PrintSphere behavior is unclear, implementation threads must report back to the main thread/UI Layout owner before changing protocol, state binding, or user-visible behavior.
- Spec changes after visual approval or interaction-contract approval require main-thread acceptance.
- UI implementation must proceed in small slices from `docs/PORTING_ARCHITECTURE_BOUNDARIES.md`; broad rewrites of `Application`, printer protocol, cloud/local merge, Web Config, or config storage are out of scope unless the main thread approves a specific exception.

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

## Active Work

- 2026-06-02: M2 ONX board bring-up started on `feature/onx-bsp-bringup`.
- Main thread owns Git integration and acceptance.
- BSP/Hardware thread owns implementation.
- Env/Serial/Flash thread owns build/flash base issues if the standard flow breaks.
