# OpenNextion-printsphere v0.1.0 Release Notes

## Summary

`v0.1.0` is the first public release of OpenNextion-printsphere, a
non-commercial OpenNextion adaptation of PrintSphere for rectangular ESP32
display boards.

This release provides one full initial flashing image for the
[ONX3248G035](https://nextion.tech/wiki/onx3248g035/) 3.5 inch display in
landscape orientation.

## Firmware Asset

GitHub Release asset:

```text
opennextion-printsphere-onx3248g035-landscape-full-v0.1.0.bin
```

SHA256:

```text
130285c013db209ecd4b0e375c1346df36ea3d1fc0db9fb58b8812b764f11e18
```

MD5:

```text
7b14550fea6b31431421e7d903041a1c
```

Size:

```text
3811536 bytes
```

This is a full initial flashing image for writing the complete firmware image to
the device. OTA firmware is not provided in `v0.1.0`.

## Supported Display

| Display | Orientation | Status |
| --- | --- | --- |
| [ONX3248G035](https://nextion.tech/wiki/onx3248g035/) | Landscape | Included in `v0.1.0` |

Portrait orientation and other display targets require a separate public build
profile and release validation. They are not `v0.1.0` targets.

## Highlights

- Added [ONX3248G035](https://nextion.tech/wiki/onx3248g035/) board support for
  the OpenNextion ESP32 display.
- Reworked the UI for a rectangular landscape display instead of the original
  round-screen layout.
- Added CN region Bambu Cloud endpoint handling for `api.bambulab.cn`,
  `bambulab.cn`, and `cn.mqtt.bambulab.com`.
- Added `GlobalSign Root R3` CA handling for selected CN region cloud HTTP,
  preview image download, and related TLS paths.
- Added the `onx_cjk_16` LVGL CJK font subset generated from Source Han Sans SC,
  with ASCII plus about 2,500 commonly used modern Chinese characters.
- Improved Chinese file name and project title rendering on the device.

## Validation Scope

Legend: Verified / Partially covered / Failed or unavailable / Not tested

| Printer | Mode | Connection / binding | Status data | AMS data | Cover image | Camera | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Bambu Lab A1 mini | Local mode | Verified | Verified | Partially covered | Failed or unavailable | Verified | Current A1 mini test setup did not include AMS; cover page is unavailable in local mode |
| Bambu Lab A1 mini | Hybrid mode | Not tested | Not tested | Not tested | Not tested | Not tested | Needs separate validation |
| Bambu Lab H2C | Local mode | Not tested | Not tested | Not tested | Not tested | Not tested | Needs separate validation |
| Bambu Lab H2C | Hybrid / cloud mode | Verified | Verified | Verified | Verified | Failed or unavailable | Only tested with a CN region account |
| Bambu Lab P1S | Local mode | Not tested | Not tested | Not tested | Not tested | Not tested | Needs validation |
| Bambu Lab P1S | Hybrid mode | Not tested | Not tested | Not tested | Not tested | Not tested | Needs validation |

## Known Limitations

- `v0.1.0` only includes the
  [ONX3248G035](https://nextion.tech/wiki/onx3248g035/) landscape full firmware
  asset.
- Portrait orientation and other display targets require a separate public build
  profile and release validation; they are not `v0.1.0` targets.
- Bambu Lab A1 mini local mode does not currently provide a cover page.
- Bambu Lab A1 mini AMS behavior has not been tested in an AMS-equipped setup.
- Bambu Lab H2C camera page is unavailable in the tested hybrid / cloud mode.
- Bambu Lab H2C testing only covered a CN region account.
- Bambu Lab P1S has not yet been validated.
- OTA update flow has not yet been validated, so OTA firmware is not included in
  this release.

## License Notice

OpenNextion-printsphere is a non-commercial derivative of
[PrintSphere](https://github.com/cptkirki/PrintSphere).

This project and the release firmware are provided under the
`Federation Non-Commercial License (FNCL) v1.1`. Use, copying, modification, and
sharing are allowed only for non-commercial purposes under the FNCL terms.
Commercial use requires separate written commercial permission from the original
copyright holder.

The repository includes a standalone `LICENSE` file containing the FNCL v1.1
text. Additional attribution, third-party component notices, and risk /
non-affiliation statements are provided in `NOTICE.md` and `DISCLAIMER.md`.
