# M3 Build/Profile Plan

## Scope

This document is the Build/Release review for the M3 milestone:
`PrintSphere Starts on ONX BSP`.

It does not change business logic, flash flow, Bambu protocol, Web Config, OTA
behavior, or production UI. It only defines the minimum build/profile path for
selecting the ONX3248G035 board without accidentally linking the Waveshare
display, touch, or PMU stack.

Current baseline:

- ESP-IDF environment: project-local `ESP-IDF v6.0.1`.
- Standard build/flash flow: `docs/BUILD_FLASH.md`.
- ONX hardware config: `docs/ONX3248G035_HARDWARE_CONFIG.md`.
- ONX smoke BSP component: `components/onx3248g035_bsp`.
- ONX smoke project: `examples/onx_bsp_smoke`.

## Recommendation

Use a new board profile and a separate ONX component. Do not keep the original
`esp32_s3_touch_amoled_1_75` component name and switch its internals for ONX.

Recommended profiles:

- `printsphere_board_waveshare_amoled_1_75`: current upstream board.
- `printsphere_board_onx3248g035`: ONX3248G035.

Recommended component layout:

- Keep `components/esp32_s3_touch_amoled_1_75` unchanged for the original
  Waveshare board.
- Keep and extend `components/onx3248g035_bsp` for ONX hardware.
- Add a thin compatibility component only if the BSP thread confirms it is
  cleaner than teaching `main` to include a neutral board API directly.

Reasoning:

- The Waveshare component manifest pulls `esp_lcd_co5300`,
  `waveshare/esp_lcd_touch_cst9217`, `esp_lvgl_adapter`, `esp_codec_dev`,
  `esp_lcd_panel_io_additions`, `esp_io_expander_tca9554`, and `espressif/usb`.
  None of those should be required for the ONX start path.
- Reusing the Waveshare component name would hide a hardware swap behind a
  misleading package name and makes future upstream merges harder.
- A distinct ONX component keeps Component Manager resolution predictable:
  selecting ONX must not even load the Waveshare manifest.
- The M2 smoke BSP is already local-only and builds with
  `IDF_COMPONENT_MANAGER=0`, which is the right model for board-private ST7796,
  CST826, and PCF8574 code.

## Minimum CMake Path

The current `main/CMakeLists.txt` hard-requires both board and PMU dependencies:

```cmake
REQUIRES
    XPowersLib
    esp32_s3_touch_amoled_1_75
```

For M3, this should become profile-gated. The smallest safe shape is:

```cmake
set(PRINTSPHERE_BOARD "waveshare_amoled_1_75" CACHE STRING "PrintSphere board profile")
set_property(CACHE PRINTSPHERE_BOARD PROPERTY STRINGS
    waveshare_amoled_1_75
    onx3248g035
)

set(PRINTSPHERE_BOARD_REQUIRES)
set(PRINTSPHERE_BOARD_DEFINES)

if(PRINTSPHERE_BOARD STREQUAL "onx3248g035")
    list(APPEND PRINTSPHERE_BOARD_REQUIRES onx3248g035_bsp)
    list(APPEND PRINTSPHERE_BOARD_DEFINES PRINTSPHERE_BOARD_ONX3248G035=1)
elseif(PRINTSPHERE_BOARD STREQUAL "waveshare_amoled_1_75")
    list(APPEND PRINTSPHERE_BOARD_REQUIRES
        esp32_s3_touch_amoled_1_75
        XPowersLib
    )
    list(APPEND PRINTSPHERE_BOARD_DEFINES PRINTSPHERE_BOARD_WAVESHARE_AMOLED_1_75=1)
else()
    message(FATAL_ERROR "Unsupported PRINTSPHERE_BOARD=${PRINTSPHERE_BOARD}")
endif()
```

Then use `${PRINTSPHERE_BOARD_REQUIRES}` in `idf_component_register(REQUIRES ...)`
and apply `${PRINTSPHERE_BOARD_DEFINES}` with `target_compile_definitions`.

Important: this CMake skeleton alone is not enough to build ONX if
`src/pmu.cpp` still includes `XPowersLib.h` unconditionally. The App/BSP boundary
must make ONX power reporting compile without `XPowersLib`.

## Avoiding CO5300/CST9217/AXP2101 on ONX

ONX profile must avoid these dependency sources:

- Do not include `esp32_s3_touch_amoled_1_75` in `REQUIRES`.
- Do not list `XPowersLib` in `REQUIRES`.
- Do not include the Waveshare BSP manifest through `EXTRA_COMPONENT_DIRS` or
  a compatibility alias.
- Do not include headers from `components/esp32_s3_touch_amoled_1_75/include`.

ONX compile blockers expected until BSP/App compatibility work lands:

- `main/src/ui.cpp` currently calls Waveshare-style APIs such as
  `bsp_display_lock`, `bsp_display_unlock`, `bsp_display_start_with_config`,
  `bsp_display_rotation_set`, and `bsp_display_brightness_set`.
- `main/src/application.cpp` reads `BSP_LCD_TOUCH_INT`.
- `main/src/pmu.cpp` unconditionally defines `XPOWERS_CHIP_AXP2101` and uses
  `XPowersPMU`, `AXP2101_SLAVE_ADDRESS`, and AXP2101 charger configuration.

Required cross-thread input:

- BSP thread: define the ONX compatibility surface for display lock/unlock,
  display start, input device registration, brightness, rotation, and touch wake
  behavior.
- App thread: replace hard AXP2101 dependency with a board-neutral power provider
  or profile-gated PMU implementation.
- UI thread: confirm whether ONX starts with the existing LVGL 9 API surface
  through a compatibility adapter, or whether UI should move to a neutral
  `printsphere_board_*` API.

## ONX BSP and Component Manager Boundary

Recommended boundary:

- Keep ST7796, CST826, and PCF8574 as local code inside
  `components/onx3248g035_bsp` for the M3 start path.
- Do not add ONX ST7796/CST826/PCF8574 as managed dependencies unless there is a
  maintained upstream component with known ESP-IDF 6 and LVGL 9 compatibility.
- Continue using Component Manager for application-level PrintSphere
  dependencies already declared in `main/idf_component.yml`: `lvgl/lvgl`,
  `espressif/cjson`, and `espressif/mqtt`.

Build boundary:

- `examples/onx_bsp_smoke` remains local-only and should keep
  `IDF_COMPONENT_MANAGER=0`.
- Full PrintSphere builds require Component Manager enabled because
  `main/idf_component.yml` declares managed dependencies.
- If sandboxed builds fail due to Component Manager network/cache/process
  access, rerun the exact same build command through the approved permission
  flow. Do not replace managed dependencies with ad hoc vendored copies as a
  build workaround.

## Manifest Strategy

Current `main/idf_component.yml`:

```yaml
dependencies:
  idf:
    version: ">=6.0.0"
  lvgl/lvgl:
    version: "9.5.0"
  espressif/cjson:
    version: "^1.7.19"
  espressif/mqtt:
    version: "^1.0.0"
```

Recommended for M3:

- Keep `idf >=6.0.0` because `docs/DEV_ENV.md` standardizes on
  ESP-IDF v6.0.1 and current PrintSphere already declares this.
- Keep LVGL at `9.5.0` for the full application.
- Do not add ONX board-private dependencies here.
- If `esp_new_jpeg` and `libpng` are resolved only through transitive or
  undeclared dependencies, make them explicit before release hardening. This can
  be delayed until the first ONX full-app build attempt classifies dependency
  resolution.

The Waveshare component manifest should remain attached only to the Waveshare
profile. ONX profile must not consume it.

## sdkconfig and Partition Strategy

Current root `sdkconfig.defaults` is mostly valid for ONX:

- Target is `esp32s3`.
- Flash size is `16MB`.
- PSRAM is enabled in octal mode at `80MHz`.
- CPU is `240MHz`.
- Custom partition table is enabled.
- LVGL examples/demos are disabled.

Keep `partitions.csv` unchanged for M3:

- PrintSphere needs OTA slots.
- Current table has two `0x500000` OTA app slots.
- ONX official example factory-only partition tables are not suitable for
  PrintSphere OTA.

Recommended ONX-specific defaults:

- Do not fork partition layout for ONX in M3.
- Add ONX-specific sdkconfig defaults only if the full app needs board-specific
  LCD/LVGL settings after BSP compatibility is wired.
- Candidate future file:
  `sdkconfig.defaults.onx3248g035`.
- Candidate use:
  pass `SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.onx3248g035"`
  only after `docs/BUILD_FLASH.md` is updated by the owning flow thread.

Current `CONFIG_BSP_LCD_TRANS_QUEUE_DEPTH=1` comes from the Waveshare BSP
Kconfig. ONX may not define that Kconfig symbol. It is harmless only if no code
expects it to exist for ONX. If ONX LVGL integration needs its own transaction
queue depth, define an ONX-specific Kconfig symbol in `onx3248g035_bsp` rather
than reusing the Waveshare name.

## Release Packaging

The existing `release_initial_flash` target depends on `app`, `bootloader`,
`flasher_args.json`, and `tools/package_initial_flash.py`. This remains the
right packaging model.

M3 should not change release layout yet. For release hardening after ONX app
boot works, split artifacts by board to prevent cross-flashing:

- `release/waveshare_amoled_1_75/...`
- `release/onx3248g035/...`

Current packaging path mismatch to track:

- Top-level CMake sends full image output to `release/firmware.bin`.
- README and existing release files refer to
  `release/initial/printsphere_full.bin`.

This is not an M3 blocker for "starts on ONX BSP", but it should be fixed before
publishing ONX artifacts.

## Verification Plan

Build-only commands. Do not flash in this M3 Build/Release task.

Baseline ONX BSP smoke build:

```sh
cd /Users/alex/Documents/codex_project/Nextion_project_PrintSphere
export IDF_TOOLS_PATH="$PWD/.tools/espressif"
export IDF_COMPONENT_MANAGER=0
. "$PWD/.tools/esp-idf-v6.0.1/export.sh"
idf.py -C examples/onx_bsp_smoke build
```

Expected classification:

- Success confirms the local ONX BSP remains buildable without managed
  dependencies.
- Failure is BSP/environment scope, not PrintSphere app scope.

Full PrintSphere default build:

```sh
cd /Users/alex/Documents/codex_project/Nextion_project_PrintSphere
export IDF_TOOLS_PATH="$PWD/.tools/espressif"
. "$PWD/.tools/esp-idf-v6.0.1/export.sh"
idf.py build
```

Expected classification:

- Success confirms current Waveshare/default app still builds under standard
  environment.
- Dependency or sandbox failure should be retried with the same command through
  the permission flow.

Future ONX profile build after CMake skeleton and BSP/App compatibility land:

```sh
cd /Users/alex/Documents/codex_project/Nextion_project_PrintSphere
export IDF_TOOLS_PATH="$PWD/.tools/espressif"
. "$PWD/.tools/esp-idf-v6.0.1/export.sh"
idf.py -DPRINTSPHERE_BOARD=onx3248g035 build
```

Expected early failures before compatibility work:

- Undefined Waveshare BSP display symbols if no ONX compatibility API exists.
- `XPowersLib.h` / AXP2101 symbols if PMU remains unconditional.
- LVGL adapter/port mismatch if ONX BSP has not exposed the expected LVGL lock
  and display lifecycle.

## Build Attempts in This Review

No build was executed in this review.

Reason:

- The task asked for build/profile strategy and allowed only documentation or a
  very small CMake/profile skeleton.
- The working tree already contains uncommitted BSP-thread edits in
  `components/onx3248g035_bsp`. Running a build now would classify someone
  else's in-progress BSP state rather than the committed M2 baseline.
- The standard build commands and expected classifications are recorded above
  for the next coordinated build attempt.

## Required Changes for M3

Must change:

- Add profile-gated board dependencies in `main/CMakeLists.txt`.
- Ensure ONX profile requires `onx3248g035_bsp`, not
  `esp32_s3_touch_amoled_1_75`.
- Remove `XPowersLib` from ONX profile dependency closure.
- Provide an ONX-compatible display/touch/brightness/lock API or a neutral board
  API that lets `ui.cpp` compile without the Waveshare BSP.
- Provide an ONX-compatible power path that lets the app compile without
  `XPowersLib` and AXP2101 symbols.

Can defer:

- Board-specific release artifact directories.
- ONX-specific `sdkconfig.defaults` overlay.
- Explicit manifest entries for `esp_new_jpeg` and `libpng` if the current
  dependency resolution remains stable.
- Full release packaging rename cleanup.
- SD card, camera, microphone, speaker, and RTC integrations.

Do not change for M3:

- `docs/BUILD_FLASH.md` standard flash flow.
- `partitions.csv`.
- Project-local ESP-IDF version.
- ONX smoke project `IDF_COMPONENT_MANAGER=0` policy.

## Inputs for Other Threads

BSP/Hardware thread:

- Decide whether `onx3248g035_bsp` should export the Waveshare-compatible
  function names or a neutral PrintSphere board API.
- Include LVGL 9 display registration and lock/unlock behavior in the API
  compatibility boundary.
- Define ONX Kconfig symbols for LCD transaction queue and buffer strategy if
  needed; do not rely on Waveshare `CONFIG_BSP_*` symbols.

App/Protocol thread:

- Make power reporting board-neutral.
- Keep Bambu protocol and MQTT independent of board selection.
- Ensure OTA code still sees the same partition layout and app image size
  constraints.

UI thread:

- Treat `320 x 480` ONX portrait as the target once the app starts.
- Confirm rotation/touch assumptions from
  `docs/ONX3248G035_HARDWARE_CONFIG.md` before changing UI layout.
- Avoid direct includes of Waveshare BSP headers outside the selected board
  profile.

Env/Serial/Flash thread:

- No change requested to `docs/BUILD_FLASH.md`.
- Future ONX full-app flash must be coordinated separately and use the standard
  flow.
