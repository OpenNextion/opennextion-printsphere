# Project Context

## Goal

Port PrintSphere to the Open Nextion ONX3248G035 board as a demo-first Bambu Lab printer companion display.

This is not a traditional Nextion serial HMI port. ONX3248G035 is an ESP32-S3 development board with an onboard LCD and capacitive touch panel, so the preferred path is to keep the ESP-IDF/LVGL PrintSphere application model and replace the board support layer.

## Source Project

- Upstream project: https://github.com/cptkirki/PrintSphere
- Current local source: imported from the upstream PrintSphere snapshot.
- Original target board: Waveshare ESP32-S3 Touch AMOLED 1.75, 466 x 466, CO5300 QSPI AMOLED, CST9217 touch, AXP2101 PMU.

## Target Hardware

- Board: ONX3248G035
- Official wiki: https://nextion.tech/wiki/onx3248g035/
- Official resources: https://github.com/OpenNextion/OpenNextion-SKU-ONX3248G035
- MCU: ESP32-S3R8
- Flash: 16 MB
- PSRAM: 8 MB
- LCD: 3.5 inch, 320 x 480, ST7796/ST7796U, SPI
- Touch: CST826, capacitive touch
- Backlight: GPIO6 PWM
- I2C: SCL GPIO7, SDA GPIO8
- LCD SPI from official examples: SCLK GPIO5, MOSI GPIO1, DC GPIO3, CS GPIO2
- Battery: ADC1 channel 3 / GPIO4 voltage divider
- Charge status: PCF8574 CHG_N
- IO expander: PCF8574, address 0x38

## Demo Scope

The project is demo-first. Long-duration production validation is intentionally out of scope for the first milestone set.

In scope for demo:

- Buildable ESP-IDF project.
- ONX3248G035 basic board bring-up.
- LCD starts and shows LVGL content.
- Touch works in the correct orientation.
- Backlight can be controlled.
- Wi-Fi provisioning path works.
- Basic Bambu printer status display works.
- Short smoke testing, typically 10 to 30 minutes.

Out of scope for demo:

- 24 hour stability testing.
- Full multi-model long-term regression.
- Production OTA release pipeline.
- High-frame-rate camera preview.
- Polished enclosure or manufacturing deliverables.

## Core Technical Direction

- Keep PrintSphere's Bambu protocol, MQTT, Web Config, OTA, config model, and status parsing as much as possible.
- Add an ONX3248G035 board profile instead of scattering hardware conditionals through app code.
- Preserve a PrintSphere-compatible BSP API where practical.
- Replace the Waveshare hardware implementation with ONX-specific ST7796, CST826, PCF8574, PWM backlight, and ADC battery logic.
- Redesign UI layout for 320 x 480 instead of scaling the 466 x 466 circular display layout.

## Current Constraints

- Git identity is configured as `Alex <freezing.xie@gmail.com>`.
- Git is initialized in this workspace after approving `git init` outside the Codex sandbox.
- ESP-IDF v6.0.1, CMake 4.0.3, and Ninja 1.12.1 are available through the project-local environment documented in `docs/DEV_ENV.md`.
- The ONX board is plugged in over USB and available as `/dev/cu.wchusbserial10`.
