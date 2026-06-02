# ONX3248G035 Hardware Configuration

This document records the BSP hardware configuration used by the ONX BSP smoke
firmware.

## LCD

- Controller: ST7796/ST7796U compatible SPI LCD.
- Resolution: `320 x 480`.
- Smoke coordinate system: portrait, origin at top-left, `x` increasing to the
  right, `y` increasing downward.
- SPI host: `SPI2_HOST`.
- SPI clock: `80 MHz`.
- SPI pins:
  - SCLK: GPIO5.
  - MOSI: GPIO1.
  - DC: GPIO3.
  - CS: GPIO2.
- Backlight: GPIO6 PWM through LEDC.
- Reset: PCF8574 pin 6 controls LCD reset.

## LCD Init State

The BSP sets the ST7796U state explicitly in
`components/onx3248g035_bsp/src/onx3248g035_bsp.c`.

- `COLMOD`: `0x55`, 16-bit RGB565 pixels.
- `MADCTL`: `LCD_CMD_MX_BIT | LCD_CMD_BGR_BIT`, value `0x48`.
- `MX`: enabled. The smoke firmware draws directly to GRAM, so this applies
  the horizontal mirror that the official LVGL example applies in its display
  rotation configuration.
- `MY`: disabled.
- `MV`: disabled.
- `BGR`: enabled, matching the official ONX examples that configure
  `LCD_RGB_ELEMENT_ORDER_BGR`.
- Display inversion: final state is `LCD_CMD_INVOFF`.

The official ST7796U component's default vendor init sequence sends `INVON`, but
the official board LCD initialization then calls `esp_lcd_panel_invert_color`
with `false`, which sends `INVOFF`. The BSP sends the final `INVOFF` command
directly at the end of the init sequence.

## RGB565 Bus Byte Order

Logical colors remain standard RGB565 values:

- Red: `0xF800`.
- Green: `0x07E0`.
- Blue: `0x001F`.
- White: `0xFFFF`.
- Black: `0x0000`.

Before writing pixels through `esp_lcd_panel_io_tx_color`, the BSP byte-swaps
each RGB565 value in one helper, `lcd_pack_rgb565()`. This matches the official
sample's use of swapped 16-bit color data for SPI LCD transfers.

## Address Window

The BSP uses standard ST7796 address windows:

- `CASET`: `x_start` to `x_end - 1`.
- `RASET`: `y_start` to `y_end - 1`.
- `RAMWR`: pixel payload for the selected window.

No x/y gap is currently applied. The smoke firmware validates the full
`0..319`, `0..479` logical coordinate range.

## Touch

- Controller: CST826 at I2C address `0x15`.
- I2C bus: `I2C_NUM_0`.
- I2C pins:
  - SDA: GPIO8.
  - SCL: GPIO7.
- PCF8574 pin 5 is the touch interrupt input path in the board mapping, but the
  smoke firmware currently polls CST826 data registers.

Accepted touch evidence from M2 showed:

- `swap_xy`: no.
- `mirror_x`: no.
- `mirror_y`: no.
- Observed x range: roughly `23..290`.
- Observed y range: roughly `28..458`.
- Coordinates increase left-to-right and top-to-bottom in portrait orientation.

If LCD scan direction is changed later, touch mapping must be revalidated
against the displayed `TL`, `TR`, `BR`, `BL`, and `CENTER` targets.

## Official ONX Source Comparison

Reference sources:

- `Example Programs/ESP-IDF/01_touch_test/components/esp_lcd_st7796u/esp_lcd_st7796u.c`
  in the OpenNextion ONX3248G035 repository.
- `Example Programs/ESP-IDF/01_touch_test/main/touch_test.c` in the same
  repository.
- `Example Programs/ESP-IDF/09_outofbox_demo/components/board/lcd_st7796u.c`
  in the same repository.

Matches:

- ST7796U vendor power/gamma sequence.
- SPI mode 0, 8-bit commands, 8-bit parameters, 80 MHz pixel clock.
- `COLMOD=0x55`.
- BGR element order.
- Final display inversion disabled.
- No XY swap for touch.

Intentional differences:

- The smoke firmware does not use LVGL, so it applies the official LVGL
  horizontal mirror as ST7796 `MADCTL.MX`.
- The smoke firmware sends final `INVOFF` directly instead of sending `INVON`
  first and then calling the panel API to disable inversion.
- The smoke firmware packs RGB565 bus byte order in a single helper because it
  writes raw `uint16_t` buffers directly.

## Visual Acceptance

The smoke firmware has two screen pages:

1. A color page with labeled `RED`, `GREEN`, `BLUE`, `WHITE`, and `BLACK`
   regions.
2. A touch and direction page with `TOP`, `BOTTOM`, `LEFT`, `RIGHT`, `X`, `Y`,
   `TL`, `TR`, `BR`, `BL`, and `CENTER`.

Visual pass requires:

- Labels are not mirrored.
- `TOP` appears at the physical top.
- `BOTTOM` appears at the physical bottom.
- `LEFT` appears on the physical left.
- `RIGHT` appears on the physical right.
- White and black are not inverted.
- Red, green, and blue labels match their visible color blocks.

## Visual Result

Final user visual confirmation on 2026-06-02:

- Color labels match the visible colors.
- White and black are correct and not inverted.
- Touch page text is readable and not mirrored.
- Touch label positions match expectation.

This confirms the current ST7796U configuration point:

- `MADCTL=0x48` (`MX | BGR`).
- `COLMOD=0x55`.
- Final inversion command: `INVOFF`.
- RGB565 payloads are byte-swapped before SPI color transfer.
