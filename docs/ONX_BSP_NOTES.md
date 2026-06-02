# ONX3248G035 BSP Notes

## Scope

The initial ONX BSP work is hardware bring-up only. It does not migrate the
PrintSphere application, Bambu protocol, MQTT, Web Config, OTA, or production UI.

## Component

Component path:

```text
components/onx3248g035_bsp
```

Current coverage:

- I2C master on `I2C_NUM_0`, SDA `GPIO8`, SCL `GPIO7`.
- PCF8574 at `0x38`.
- ST7796 LCD over `SPI2_HOST`, SCLK `GPIO5`, MOSI `GPIO1`, DC `GPIO3`, CS `GPIO2`.
- LCD reset through PCF8574 pin `LCD_RST`.
- Backlight PWM on `GPIO6` via LEDC low-speed timer 1, channel 0, 10-bit, 10 kHz.
- CST826 touch controller at `0x15`, with chip ID register `0xAA` and coordinate data at `0x02`.

Deferred interfaces:

- SD card mount: ONX uses SDMMC 1-bit on D0 `GPIO9`, CMD `GPIO10`, CLK `GPIO11`; PCF8574 `SDCS` should be kept high before mount.
- Battery: ONX uses ADC1 channel 3 / GPIO4 for battery voltage and PCF8574 `CHG_N` for charge status.
- Touch interrupt: official examples route `TP_INT` through PCF8574. The smoke firmware polls touch instead of using interrupts.
- LVGL integration: not included in smoke firmware.

## Smoke Firmware

Project path:

```text
examples/onx_bsp_smoke
```

Build command used:

```sh
cd /Users/alex/Documents/codex_project/Nextion_project_PrintSphere
export IDF_TOOLS_PATH="$PWD/.tools/espressif"
export IDF_COMPONENT_MANAGER=0
. "$PWD/.tools/esp-idf-v6.0.1/export.sh"
idf.py -C examples/onx_bsp_smoke build
```

Firmware path:

```text
/Users/alex/Documents/codex_project/Nextion_project_PrintSphere/examples/onx_bsp_smoke/build/onx_bsp_smoke.bin
```

Flash command used:

```sh
idf.py -C examples/onx_bsp_smoke -p /dev/cu.wchusbserial10 flash
```

## Verification Result

Build result on 2026-06-02:

- `onx_bsp_smoke.bin` generated successfully.
- Binary size: `0x3d400` bytes.
- Smallest app partition free space: 76%.

Flash result on 2026-06-02:

- Port: `/dev/cu.wchusbserial10`.
- Chip: ESP32-S3 QFN56 revision v0.2.
- PSRAM: embedded 8 MB detected.
- Bootloader, partition table, and `onx_bsp_smoke.bin` wrote and verified.

Serial evidence:

```text
I (750) onx_bsp: ONX3248G035 BSP smoke start
I (752) onx_bsp: I2C ready: port=0 sda=8 scl=7
I (756) onx_bsp: PCF8574 ready: addr=0x38 state=0xFF
I (761) onx_bsp: Backlight PWM ready: gpio=6 freq=10000Hz
I (765) onx_bsp: Backlight PWM set: 30% duty=306
I (770) onx_bsp: PCF8574 pin 6=0 state=0xDF
I (974) onx_bsp: PCF8574 pin 6=1 state=0xFF
I (1415) onx_bsp: LCD init complete: ST7796 SPI 320x480 pclk=80000000
I (1448) onx_bsp: LCD fill complete: color=0xF800
I (1982) onx_bsp: LCD fill complete: color=0x07E0
I (2515) onx_bsp: LCD fill complete: color=0x001F
I (3040) onx_bsp: LCD color bars complete
I (3040) onx_bsp: Backlight PWM set: 100% duty=1023
I (3041) onx_bsp: Touch init complete: CST826 addr=0x15 chip_id=0x11
I (3043) onx_bsp: ONX3248G035 BSP smoke init complete; touch sampling is running
I (3043) onx_bsp: Tap the four screen corners; coordinates print only when touch is detected
I (13043) onx_bsp: Touch sampler still running; waiting for touch
```

Open validation:

- Four-corner touch coordinate mapping still needs a manual tap test while the smoke firmware keeps sampling.
- The serial log proves LCD transfers completed; visual color order and panel orientation should be confirmed by looking at the device.
