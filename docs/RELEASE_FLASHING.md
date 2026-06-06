# Flashing Release Firmware

This document is for users flashing a firmware image downloaded from a GitHub
Release.

`v0.1.0` provides a full initial flashing image for
[ONX3248G035](https://nextion.tech/wiki/onx3248g035/) in landscape orientation.
OTA firmware is not provided in `v0.1.0`.

## Release Asset

Download this file from the `v0.1.0` GitHub Release page:

```text
opennextion-printsphere-onx3248g035-landscape-full-v0.1.0.bin
```

Expected SHA256:

```text
130285c013db209ecd4b0e375c1346df36ea3d1fc0db9fb58b8812b764f11e18
```

Expected MD5:

```text
7b14550fea6b31431421e7d903041a1c
```

Expected size:

```text
3811536 bytes
```

Firmware files are release assets only. They are not committed to git.

## Verify The Download

Before flashing, verify the SHA256 of the downloaded file.

macOS / Linux:

```sh
shasum -a 256 opennextion-printsphere-onx3248g035-landscape-full-v0.1.0.bin
```

Windows PowerShell:

```powershell
Get-FileHash .\opennextion-printsphere-onx3248g035-landscape-full-v0.1.0.bin -Algorithm SHA256
```

The result must match:

```text
130285c013db209ecd4b0e375c1346df36ea3d1fc0db9fb58b8812b764f11e18
```

## Flash The Full Initial Image

The `v0.1.0` asset is a full initial flashing image. Write it at address `0x0`.

```sh
python -m esptool --chip esp32s3 \
  -p <SERIAL_PORT> \
  -b 115200 \
  --before default-reset \
  --after hard-reset \
  write-flash 0x0 opennextion-printsphere-onx3248g035-landscape-full-v0.1.0.bin
```

Replace `<SERIAL_PORT>` with the serial device for your computer.

Do not use this firmware on unsupported boards. `v0.1.0` is only for
[ONX3248G035](https://nextion.tech/wiki/onx3248g035/) landscape.

## OTA Status

OTA firmware is not provided in `v0.1.0`.

Use the full initial flashing image for this release.

## License And Risk Notice

OpenNextion-printsphere is a non-commercial derivative of PrintSphere. The
release firmware is subject to the repository `LICENSE`, the Federation
Non-Commercial License (FNCL) v1.1.

Flashing third-party firmware involves risk. Use this firmware only after
understanding the risks and only on hardware you are prepared to recover or
reflash yourself.
