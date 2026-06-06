# ONX CJK font generation

`modern_chinese_common_2500.txt` contains the 2500 common characters from
`现代汉语常用字表 · 常用字(2500字)`.

Source used for this demo-stage subset:

- 漢典: https://zdic.net/zd/zb/cc1/
- Retrieved and parsed on 2026-06-06.

`onx_extra_symbols.txt` contains project-specific Chinese characters and sample
job title characters that are not guaranteed by the base table. The generator
merges the common table, extras, and ASCII `0x20-0x7f`, then regenerates
`main/include/font/onx_cjk_16.c` from LVGL's bundled
`SourceHanSansSC-Normal.otf`.

Regenerate with:

```sh
python3 tools/fonts/generate_onx_cjk_16.py
```

