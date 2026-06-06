# NOTICE

## Project Basis

OpenNextion-printsphere is a non-commercial derivative of PrintSphere:

```text
PrintSphere
https://github.com/cptkirki/PrintSphere
Copyright (c) 2026 Cpt_Kirk
License: Federation Non-Commercial License (FNCL) v1.1
```

This repository preserves the original PrintSphere copyright and root license
terms. OpenNextion-printsphere, including source changes and release firmware
assets derived from PrintSphere, is provided under the FNCL v1.1 non-commercial
terms unless a specific third-party component states its own separate license.

Commercial use of PrintSphere-derived source code, binaries, or release firmware
requires separate written commercial permission from the original copyright
holder.

## OpenNextion Adaptation

This derivative adds OpenNextion display support, with the first public target:

```text
ONX3248G035
https://nextion.tech/wiki/onx3248g035/
```

The OpenNextion adaptation includes board support for display, touch, backlight,
LVGL integration, and the public landscape build profile for ONX3248G035
hardware.

OpenNextion project reference:

```text
https://github.com/OpenNextion
```

Review note: before final publication, confirm whether any OpenNextion-provided
source, schematics, CAD files, or sample code copied into this repository has
additional license or attribution requirements.

## CJK Font Subset

OpenNextion-printsphere includes an LVGL CJK font subset:

```text
main/include/font/onx_cjk_16.c
```

The font was generated from Source Han Sans SC using the project font tooling.
It includes ASCII plus about 2,500 commonly used modern Chinese characters, with
additional project symbols where needed.

Generation inputs / tools:

```text
tools/fonts/generate_onx_cjk_16.py
tools/fonts/modern_chinese_common_2500.txt
tools/fonts/onx_extra_symbols.txt
```

Source Han Sans SC is distributed under the SIL Open Font License.

Review note: before final publication, keep the applicable SIL Open Font License
notice and attribution for the exact Source Han Sans SC source font used to
generate the subset.

## Certificates

The project embeds certificates needed by PrintSphere / Bambu Lab connection
paths, including:

```text
main/include/certs/bambu.cert
main/include/certs/bambu_h2c_251122.cert
main/include/certs/bambu_p2s_250626.cert
main/include/certs/globalsign_root_r3.pem
```

The CN region cloud compatibility work uses `GlobalSign Root R3` for selected
CN-region cloud HTTP, preview download, and TLS paths.

## ESP-IDF Managed Dependencies

The application declares ESP-IDF managed dependencies in:

```text
main/idf_component.yml
```

Current declared dependencies include:

```text
idf >= 6.0.0
lvgl/lvgl 9.5.0
espressif/cjson ^1.7.19
espressif/mqtt ^1.0.0
```

These dependencies are not relicensed by OpenNextion-printsphere. They remain
under their respective upstream licenses and notices.

## Trademark / Affiliation Notice

OpenNextion-printsphere is not an official Bambu Lab project, not an official
OpenNextion project, and not the official original PrintSphere project.

Names such as Bambu Lab, PrintSphere, OpenNextion, and product model names are
used for factual identification and compatibility description only.
