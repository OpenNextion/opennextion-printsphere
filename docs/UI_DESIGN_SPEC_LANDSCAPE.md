# PrintSphere ONX 横屏 UI 设计规范

状态：视觉方向已确认 / 等待主线程审查并安排开发

本文档用于规划 ONX3248G035 在已验证横屏方向下的 PrintSphere UI。目标逻辑分辨率为 `480 x 320`，低层候选来自 BSP 方向探针：ST7796 `MADCTL=0xE8` (`MX|MY|MV|BGR`)，触摸映射 `mapped_x=479-raw_y`, `mapped_y=319-raw_x`。这是硬件方向基线，不代表应用 UI 已完成横屏适配。

配套视觉稿：`docs/ui_design_landscape_preview.html`。用户已确认该视觉方向可接受；后续开发线程必须以本文档的坐标、颜色、字体、触控区域和交互边界为准高保真实现，不得只凭截图自由重画。

## 目标

- 基于原 PrintSphere 466 x 466 圆屏信息结构和现有 ONX 320 x 480 竖屏规范，设计真实的 `480 x 320` 横屏 UI。
- 保留原项目核心页面、数据字段和交互：Printer Select、AMS、Main/Status、Cover Preview、Camera、Web Config PIN、亮度手势、横向翻页、相机刷新、logo 灯光切换、remaining/ETA 切换。
- 横屏不是竖屏旋转或等比缩放；必须重新利用 480 px 宽度，减少纵向堆叠。
- 给后续 UI 实现线程提供页面坐标、触控区域、数据绑定、交互契约和验收标准。

## 非目标

- 不实现 LVGL 页面代码。
- 本设计线程不修改 `main/**`、`components/**`、`examples/**`、协议、BSP、构建脚本、sdkconfig。
- 不烧录、不编译固件、不提交 Git。
- 不新增打印机控制协议、不新增 AMS 操作、不把展示区域变成控制按钮。
- 不要求替代竖屏规范；横屏应通过 board/layout profile 与竖屏并存。

## 设计原则

- 同源数据：横屏 UI 只接现有 `PrinterSnapshot`、`Ui::PrinterCardInfo`、portal state 和已有 request/consume 接口。
- 同交互契约：沿用 `docs/UI_DESIGN_SPEC.md` 的全局手势、页面可用性、no-op 边界、overlay 优先级。
- 横屏优先双栏：Main、Camera、Cover 使用左右分区；AMS 使用四槽横向网格；Printer Select 使用列表 + 详情提示。
- 信息密度适中：320 px 高度有限，状态、错误、时间、温度优先；说明文字压缩为一行或两行。
- 复用资源：继续使用 `bambuicon_small`、Dosis 字体、MDI 字体子集、黑底高对比色彩，不新增 bitmap/icon。

## 硬件与画布约束

| 项 | 约束 |
|---|---|
| 逻辑分辨率 | `480 x 320` |
| 页面根容器 | `x=0,y=0,w=480,h=320` |
| 安全边距 | `12 px` |
| 主内容区域 | `x=12..468,y=12..308` |
| 背景 | `#050607` |
| 最小触控目标 | `44 x 44` |
| 页面切换方向 | 横向 pager，左右滑动 |
| 亮度手势 | 竖向拖动，沿用原阈值 |

横屏实现必须使用 board/layout profile 或编译配置选择 `portrait` / `landscape`，不得在应用协议、云/本地合并、Web Config、NVS 或打印机控制代码里写横屏分支。

## 页面模型

逻辑页面顺序沿用原项目：

1. Printer Select
2. AMS 1
3. AMS 2
4. AMS 3
5. AMS 4
6. Main / Status
7. Cover Preview
8. Camera

启用规则不变：

- Printer Select 和 Main 永远启用。
- AMS 页由 `snapshot.ams->count` 决定。
- Cover 由 `snapshot.preview_page_available` 决定。
- Camera 由 `snapshot.camera_page_available` 决定。
- 禁用页必须跳过并 clamp，不允许做成可操作空页。

## 全局尺寸与样式 Token

### 坐标 Token

| 名称 | 值 | 用途 |
|---|---:|---|
| `screen_w` | `480` | 横屏逻辑宽 |
| `screen_h` | `320` | 横屏逻辑高 |
| `pad` | `12` | 页面边距 |
| `content_w` | `456` | `480 - 24` |
| `content_h` | `296` | `320 - 24` |
| `topbar` | `x=12,y=10,w=456,h=28` | 页面标题/状态 |
| `bottom_hint` | `x=12,y=292,w=456,h=20` | 简短提示；空间紧张时隐藏 |
| `card_radius` | `8` | 卡片圆角 |
| `touch_min` | `44` | 触摸目标 |

### 颜色 Token

沿用竖屏规范：

| 名称 | 色值 |
|---|---:|
| `bg` | `#050607` |
| `panel` | `#111418` |
| `panel_2` | `#171B20` |
| `line` | `#2D333B` |
| `muted` | `#8B98A8` |
| `soft` | `#C8D1DC` |
| `text` | `#F7FAFC` |
| `green` | `#4ADE80` |
| `blue` | `#60A5FA` |
| `cyan` | `#87CEEB` |
| `amber` | `#F0A64B` |
| `red` | `#EF4444` |

### 字体 Token

| 语义 | 首选字体 | 横屏字号 |
|---|---|---:|
| 大百分比/亮度/PIN | `dosis_40` | 36-40 |
| 状态标题 | `dosis_32` | 24-28 |
| 指标数值 | `dosis_32` | 24-28 |
| 卡片标题/任务名 | `dosis_20` 或 Montserrat 20 | 17-20 |
| 用户动态文本 CJK | `onx_cjk_16`；如后续扩字再重新生成子集 | 16 |
| 元信息/说明 | Montserrat 14/16 或 Dosis 20 缩用 | 12-15 |
| MDI 图标 | `mdi_30` / `mdi_40` | 28-36 |

## 高保真实现锁定参数

本节把已确认的 HTML 横屏稿固化为 LVGL 开发约束。开发线程实现时必须优先按本节参数落地；只有实机字体渲染、触摸误差或 LCD 可读性需要时，才允许在主线程确认后小幅校准。

### 全局对象与绘制规则

| 对象 | 坐标/尺寸 | 样式 | 说明 |
|---|---|---|---|
| screen/page root | `x=0,y=0,w=480,h=320` | bg `#050607` | 每个逻辑页根对象 |
| page padding | `left=12,right=12,top=10,bottom=8` | - | 横屏顶边略紧凑 |
| topbar | `x=12,y=10,w=456,h=28` | transparent | 左标题、右 meta |
| topbar title | `x=12,y=10,w<=260,h=28` | `dosis_20` 或 Montserrat 20, `18 px`, `#F7FAFC`, bold | 单行省略 |
| topbar meta | `x=292,y=10,w=176,h=28` | `12 px`, `#8B98A8`, bold, right align | BAT/source/page/IP |
| bottom hint | `x=12,y=292,w=456,h=20` | `12 px`, `#8B98A8`, top border `#2D333B` 30% | 空间紧张时可隐藏，但不应挤压主信息 |
| card background | 任意卡片 | bg `rgba(255,255,255,0.045)` 或 `#111418`, border `1 #2D333B`, radius `8` | 不使用大圆角 |
| pill | `h=22,min_w=32,pad_x=8` | border `#2D333B`, radius `11`, font `11-12` | active/source/page 等 |
| touch debug outline | 不进正式 UI | HTML 蓝色虚线仅表示触摸区 | LVGL 正式 UI 不绘制 |

### 颜色与状态映射

| 状态/用途 | 色值 | 约束 |
|---|---:|---|
| 页面背景 | `#050607` | 全屏统一 |
| 卡片背景 | `#111418` / `#171B20` | 不引入浅色大面积背景 |
| 边框/分隔 | `#2D333B` | 卡片、top/bottom 分隔 |
| 主文字 | `#F7FAFC` | 标题、大数字、重要状态 |
| 次文字 | `#8B98A8` | meta、hint、说明 |
| 软强调 | `#C8D1DC` | layer、非错误说明 |
| printing/active | `#4ADE80` | 打印、active、online |
| progress accent | `#60A5FA` | 进度条渐变右端 |
| remaining/ETA | `#87CEEB` | 时间行 |
| paused/warning/PIN | `#F0A64B` | 暂停、PIN 边框和数字 |
| error/HMS | `#EF4444` | 错误边框、错误标题、ERR pill |

### 触控规则

| 区域 | 触控尺寸 | 行为 |
|---|---|---|
| 全屏 | `480 x 320` | 横向翻页、竖向亮度、长按 PIN、首触唤醒 |
| Printer card | `300 x 92` | 点击切换 profile |
| Bambu logo | 视觉 `54 x 54`，触控 `>=60 x 60` | 支持 chamber light 时点击 toggle |
| Remaining/ETA row | `172 x 50` | 点击切换 remaining/ETA |
| Camera image | `300 x 224` | 小位移 tap 请求刷新 |
| AMS tray | `108 x 128` | 第一版 no-op |
| EXT row | `456 x 44` | 第一版 no-op |
| Cover image | `240 x 240` | no-op |
| Metric card / error card | 视觉区域 | no-op |

开发线程不得把 no-op 区域扩展成控制按钮。

### 逐页坐标锁定

#### Printer Select 坐标

| 元素 | 坐标/尺寸 | 字体/颜色 | 行为 |
|---|---|---|---|
| title | `x=12,y=10,w=180,h=28` | `18/#F7FAFC/bold` | 无 |
| right hint | `x=220,y=10,w=248,h=28` | `12/#8B98A8/right` | 无 |
| printer list | `x=12,y=48,w=300,h=224` | transparent | 垂直滚动；每屏显示 2 张完整卡 |
| card 1 | `x=12,y=48,w=300,h=92` | bg card | 点击 |
| card 2 | `x=12,y=150,w=300,h=92` | bg card | 点击 |
| card 3+ | 滚动后显示，步进 `102` | bg card | 点击 |
| active stripe | card 内 `x=0,y=0,w=5,h=card_h` | `#4ADE80` | active 时显示 |
| card name | card 内 `x=12,y=10,w=208,h=24` | CJK font `16/#F7FAFC` | 单行 DOT |
| card pill | card 内右上 `h=22,min_w=32` | `11/#C8D1DC` | active/index |
| card model/serial | card 内 `x=12,y=40,w=252,h=18` | `13/#8B98A8` | 单行省略 |
| card state/host | card 内 `x=12,y=64,w=252,h=18` | `13`, 状态色 | 单行省略；不得贴底裁切 |
| detail panel | `x=324,y=48,w=144,h=228` | card | 展示 active printer |
| detail title | `x=336,y=60,w=120,h=40` | CJK font `16/#F7FAFC` | 最多 2 行；raw serial 禁止 |
| detail state | `x=336,y=108,w=120,h=20` | `14`, 状态色 | `Online` 等 |
| detail serial | `x=336,y=136,w=120,h=20` | `13/#C8D1DC` | `SN <head6>...<tail4>` |
| detail web label | `x=336,y=168,w=120,h=16` | `12/#8B98A8` | `Web Config` 或 `Open` |
| detail ip | `x=336,y=186,w=120,h=20` | `13/#F7FAFC` | 只显示 IP 或 `192.168.4.1` |
| detail pin hint | `x=336,y=214,w=120,h=18` | `12/#87CEEB` | `Hold PIN` / `PIN active` |
| bottom hint | `x=12,y=296,w=456,h=14` | `11/#8B98A8` | Printer 页默认隐藏；必要时只显示 `Hold PIN` |

#### AMS 坐标

| 元素 | 坐标/尺寸 | 字体/颜色 | 行为 |
|---|---|---|---|
| title | `x=12,y=10,w=120,h=28` | `18/#F7FAFC/bold` | 无 |
| meta row | `x=180,y=10,w=288,h=28` | `12/#8B98A8/right` | humidity/temp/page |
| tray row | `x=12,y=52,w=456,h=132` | grid 4 col, gap `8` | 无 |
| tray 1 | `x=12,y=52,w=108,h=128` | material color bg | no-op |
| tray 2 | `x=128,y=52,w=108,h=128` | material color bg | no-op |
| tray 3 | `x=244,y=52,w=108,h=128` | material color bg | no-op |
| tray 4 | `x=360,y=52,w=108,h=128` | material color bg | no-op |
| tray material | tray 内 `x=8,y=10,w=92,h=24` | `20/bold`, auto black/white | 单行省略 |
| tray brand/name | tray 内 `x=8,y=38,w=92,h=16` | `12`, auto black/white | 单行省略 |
| tray percent | tray 内 `x=8,y=96,w=56,h=24` | `20/bold`, auto black/white | `--` if unknown |
| active/ERR flag | tray 内 `right=7,bottom=7,w>=32,h=22` | active dark/green, ERR red | 展示 |
| EXT row | `x=12,y=196,w=456,h=44` | card, `14/#F7FAFC/bold` | no-op |
| AMS note/error | `x=12,y=248,w=456,h=36` | `13/#8B98A8`; error 用 `#EF4444` | 单行或两行 |

#### Main / Status 坐标

| 元素 | 坐标/尺寸 | 字体/颜色 | 行为 |
|---|---|---|---|
| topbar status | `x=12,y=10,w=260,h=28` | `18/#F7FAFC`; state color for error/warn | 无 |
| topbar meta | `x=292,y=10,w=176,h=28` | `12/#8B98A8/right` | battery/source |
| left panel | `x=12,y=48,w=196,h=228` | card | 容器 |
| logo button | `x=24,y=60,w=54,h=54` | Bambu image `48 x 48` | chamber light |
| logo touch | `x=21,y=57,w=60,h=60` | invisible hitbox | 点击 |
| percent | `x=92,y=56,w=104,h=42` | `dosis_40,38,#F7FAFC` | 无 |
| lifecycle | `x=92,y=100,w=104,h=28` | `dosis_32,21-24`, 状态色 | 无 |
| progress bar | `x=24,y=142,w=172,h=12` | bg `#20252B`, fill `#4ADE80 -> #60A5FA` | 无 |
| remaining row | `x=24,y=178,w=172,h=50` | card, `#87CEEB` | 点击切换 |
| remaining label | row 内 `x=10,y=7,w=152,h=13` | `12/#87CEEB` | 文案 |
| remaining value | row 内 `x=10,y=24,w=152,h=25` | `dosis_32/40,25,#87CEEB` | remaining/ETA |
| job title | `x=220,y=48,w=248,h=38` | CJK font `16/#F7FAFC` | 最多 2 行或 DOT；不得遮挡 layer |
| layer | `x=220,y=90,w=248,h=18` | `14/#C8D1DC/bold` | 单行 DOT |
| metric grid | `x=220,y=112,w=248,h=112` | 2 col x 2 row, card `120 x 52`, gap `8` | 无 |
| metric card 1 | `x=220,y=112,w=120,h=52` | card | no-op |
| metric card 2 | `x=348,y=112,w=120,h=52` | card | no-op |
| metric card 3 | `x=220,y=172,w=120,h=52` | card | no-op |
| metric card 4 | `x=348,y=172,w=120,h=52` | card | no-op |
| metric label | card 内 `x=8,y=5,w=104,h=14` | `12-14/#8B98A8/bold` | 单行 DOT |
| metric value | card 内 `x=8,y=23,w=104,h=20` | `dosis_20` 或 Montserrat 18, `#F7FAFC/bold` | 单行 DOT |
| metric aux | 默认隐藏；仅当能放入 card 内 `x=66,y=23,w=46,h=20` 时显示 | `12/#8B98A8/right` | 单行 DOT；不得压住 value |
| detail/error card | `x=220,y=232,w=248,h=44` | card; error border/background red | no-op |
| portal hint | `x=12,y=292,w=456,h=20` | `12/#8B98A8` | long press from global |

#### Cover 坐标

| 元素 | 坐标/尺寸 | 字体/颜色 | 行为 |
|---|---|---|---|
| topbar title | `x=12,y=10,w=160,h=28` | `18/#F7FAFC/bold` | 无 |
| topbar meta | `x=300,y=10,w=168,h=28` | `12/#8B98A8/right` | source/page |
| cover image box | `x=12,y=48,w=240,h=240` | border `#2D333B`, radius `8` | no-op |
| image content | box 内 contain | `preview_blob` decoded image | no static image |
| no-image text | box center | `17/#C8D1DC/bold` | loading/offline/no active |
| info panel | `x=264,y=48,w=204,h=240` | card | no-op |
| cover title | `x=276,y=60,w=180,h=72` | `20/#F7FAFC/bold` | 最多 3 行，优先 2 行 |
| status/detail lines | `x=276,y=142,w=180,h=96` | `13/#8B98A8` | 逐行显示 |
| no-op hint | `x=276,y=250,w=180,h=24` | `12/#8B98A8` | 可隐藏 |

#### Camera 坐标

| 元素 | 坐标/尺寸 | 字体/颜色 | 行为 |
|---|---|---|---|
| topbar title | `x=12,y=10,w=160,h=28` | `18/#F7FAFC/bold` | 无 |
| topbar meta | `x=220,y=10,w=248,h=28` | `12/#8B98A8/right` | lifecycle/source/page |
| camera image box | `x=12,y=48,w=300,h=224` | border `#2D333B`, radius `8` | tap refresh |
| image content | box 内 center/contain | RGB565 camera image | 不裁关键画面 |
| no-image text | box center | `16/#C8D1DC/bold` | `Tap for new image` 等 |
| info panel | `x=324,y=48,w=144,h=224` | card | no-op |
| camera status | image mode: `x=336,y=60,w=120,h=24`; no-image mode: image box center `x=34,y=136,w=256,h=48` | image mode `18-20/#F7FAFC/bold`; no-image `16/#C8D1DC/bold` | image mode DOT 1 行；no-image WRAP 最多 2 行 |
| layer value | `x=336,y=90,w=120,h=24` | `14/#C8D1DC` | `Layer -- / --` 或隐藏；DOT 1 行 |
| source line | `x=336,y=124,w=120,h=20` | `13/#8B98A8` | `Source: local/waiting`；DOT 1 行 |
| refresh line | `x=336,y=156,w=120,h=20` | `13/#8B98A8` | `Auto 2s`；DOT 1 行 |
| tap hint | `x=336,y=198,w=120,h=20` | `13/#87CEEB` | `Tap image`；DOT 1 行 |
| bottom hint | hidden in image mode; optional no-image/error only `x=12,y=286,w=456,h=18` | `11-12/#8B98A8` | DOT 1 行；不得贴底或重复 tap hint |

#### Overlay 坐标

| Overlay | 坐标/尺寸 | 样式 | 行为 |
|---|---|---|---|
| brightness overlay | 居中，`w>=104,h≈60,pad=10/18,radius=16` | bg black 72%, text `dosis_40,40,#F7FAFC` | 垂直拖动显示，释放隐藏 |
| PIN card | 居中，`w=280,h≈132,pad=18/22,radius=18` | bg `#071018` 95%, border `2 #F0A64B` | `portal_pin_active` 时显示 |
| PIN title | card 内顶部 | `17/#F7FAFC/bold` | `WEB CONFIG PIN` |
| PIN value | title 下 `8 px` | `dosis_40,42,#F0A64B` | 来自 portal state |
| PIN detail | value 下 `10 px` | `14/#C8D1DC` | 剩余时间 |

## 页面布局

### 1. Printer Select

用途：选择已配置打印机，保留 Web Config/PIN 入口提示。

布局：

| 区域 | 坐标/尺寸 | 内容 |
|---|---|---|
| topbar | `x=12,y=10,w=456,h=28` | 左 `Printers`，右 Wi-Fi/IP/PIN hint |
| 列表 | `x=12,y=48,w=300,h=224` | 打印机卡片，垂直滚动；每屏 2 张完整卡 |
| 详情/空态面板 | `x=324,y=48,w=144,h=228` | active printer、short SN、Web Config IP、PIN 提示 |
| 底部 hint | `x=12,y=296,w=456,h=14` | Printer 页默认隐藏；必要时只显示短 `Hold PIN` |

卡片：

- 尺寸固定 `w=300,h=92`，间距 `10`；不得再使用 `62..70` 高度。
- 卡片内最多三行：title、model/short serial、state/host。三行都必须在 `h=92` 内完整可见，第三行底部至少留 `8 px` 内边距。
- 整卡点击，触发 `pending_printer_switch_`。
- active 使用左色条 `5 px #4ADE80`。
- connected 用文字 + 小点；不能只靠颜色。
- 如果实现线程选择两行卡片策略，必须把 state 合并为右上小点/pill，不能把第三行画到卡片底部之外；但本规范默认使用 `92 px` 三行卡片。
- 右侧详情面板不得把 `Open <ip> | Hold for PIN` 作为一个 120 px 宽 label；必须拆成 `Web Config`、`<ip>`、`Hold PIN` 三个短行。
- Printer 页底部不再显示大号 `Hold for Web Config PIN`。Web Config discoverability 放在右侧详情面板和 Main 页底部；Printer 底部只允许小号辅助 `Hold PIN`，且默认隐藏。

草图：

```text
+------------------------------------------------+
| Printers                              Wi-Fi    |
| +------------------------------+ +------------+ |
| | H2C                    act  | | H2C        | |
| | H2C 0459                    | | Online     | |
| | Online  192.168.0.139      | | SN 31B...  | |
| +------------------------------+ | Hold PIN   | |
| | A1 Mini                 2    | | 192.168... | |
| | Waiting for LAN              | +------------+ |
| +------------------------------+                |
|                                                |
+------------------------------------------------+
```

### 2-5. AMS Pages

用途：显示每个 AMS 单元四个槽位；AMS 1 可显示 EXT。

横屏重排：

- 不采用竖屏 2 x 2 大卡片；横屏采用一行四槽，保留原项目“四槽横排”的心智模型。
- 480 宽度允许 `4 x 100` 左右槽位；比竖屏更接近原圆屏 AMS 页。

布局：

| 区域 | 坐标/尺寸 | 内容 |
|---|---|---|
| topbar | `x=12,y=10,w=456,h=28` | `AMS n`、湿度、温度、页码 |
| tray row | `x=12,y=52,w=456,h=132` | 4 个槽位横排 |
| 单槽 | `w=105,h=128,gap=8` | 材料、品牌/名称、余量、active/error |
| EXT row | `x=12,y=196,w=456,h=44` | 仅 AMS 1 且外置料卷 active |
| note/error | `x=12,y=248,w=456,h=36` | AMS 错误摘要或说明 |

单槽内容：

- 背景使用 `AmsTrayInfo.color_rgba`。
- 顶部材料类型 `material_type`，中部 `material_name` 单行省略，底部 `remain_pct`。
- active 使用绿色底部 pill 或 `>` 标记。
- slot error 使用红边 + `ERR`，不新增点击动作。

草图：

```text
+------------------------------------------------+
| AMS 1        Humidity 32%      Temp 28C   2/8 |
| +--------+ +--------+ +--------+ +--------+   |
| | PLA    | | PETG   | | Empty  | | ASA ERR|   |
| | Basic  | | Trans  | | --     | | Black  |   |
| | 86% >  | | 42%    | | --     | | 18%    |   |
| +--------+ +--------+ +--------+ +--------+   |
| EXT  PLA white                         active |
| Slot 4 HMS: filament feed failed              |
+------------------------------------------------+
```

无 AMS：

- 若所有 AMS 不存在，pager 应跳过 AMS 1-4。
- 若 demo 需要说明，可在 AMS 1 占位显示 `No AMS connected`，但不得成为可操作页。

### 6. Main / Status

用途：横屏主监控页。横屏宽度优先给状态和指标，减少纵向滚动。

布局采用左右两栏：

| 区域 | 坐标/尺寸 | 内容 |
|---|---|---|
| topbar | `x=12,y=10,w=456,h=28` | 左状态摘要，右电池/source |
| 左主面板 | `x=12,y=48,w=196,h=228` | logo、百分比、生命周期、进度条、remaining/ETA |
| 右信息区 | `x=220,y=48,w=248,h=228` | 任务名、层数、温度指标、detail/error |
| PIN hint | `x=12,y=292,w=456,h=20` | 可隐藏，portal hint 优先 |

左主面板：

- logo/light button：`x=24,y=60,w=54,h=54`，触控扩到 `60 x 60`。
- 大百分比：`x=92,y=56,w=104,h=42`，`dosis_40`。
- 生命周期：`x=92,y=100,w=104,h=28`。
- 进度条：`x=24,y=142,w=172,h=12`。
- Remaining/ETA 行：`x=24,y=178,w=172,h=50`，整行点击切换。

右信息区实机修订：

- 任务名：`x=220,y=48,w=248,h=38`，最多 2 行；无任务时用 `No active job` 单行，`DOT`。
- Layer：`x=220,y=90,w=248,h=18`，单行 `DOT`；显示 `Layer: -- / --` 时不得换行。
- 指标网格：`x=220,y=112,w=248,h=112`，2 列 x 2 行，卡片固定 `120 x 52`，列/行 gap 均 `8`。
- 指标卡片必须容纳两行：label `x=8,y=5,w=104,h=14`，value `x=8,y=23,w=104,h=20`。label/value 都用 `DOT`，不允许 `WRAP`。
- metric value 只显示主值，例如 `--C`、`220C`、`122/350`、`setup`。若已有 aux label 会压住 value，横屏第一版必须隐藏 aux；只有 card 高度和宽度都确认有余量时才允许放到右侧 `x=66,y=23,w=46,h=20`。
- detail/error：`x=220,y=232,w=248,h=44`，普通态最多 2 行 `WRAP`，错误态优先短 HMS/detail 摘要；不得与底部 portal hint 重叠。

根因记录：

- 2026-06-06 实机 Main/Status 截图显示旧规格 `120 x 38` metric card 在真实 Dosis/Montserrat 渲染下高度不足，label、value、aux 互相遮挡。此处属于“设计空间不足 + 实现未限制 aux/long mode”两者都有；修订后实现线程必须先按 `120 x 52` 卡片和隐藏 aux 落地。

草图：

```text
+------------------------------------------------+
| 87% Printing                         BAT 92%  |
| +------------------+ +------------------------+ |
| | [logo] 87%       | | Bambu_P1S_gearbox...  | |
| |        Printing  | | Layer 122 / 350       | |
| | [progress bar]   | | +Nozzle---+ +Bed------+ |
| | Remaining 1h24m  | | |220C     | |65C      | |
| +------------------+ | +Layer----+ +Detail---+ |
| Hold for Web Config PIN                       |
+------------------------------------------------+
```

错误/暂停态：

- topbar 状态色改为 `amber` 或 `red`。
- 右侧 detail/error 面板优先显示 `snapshot.detail`、HMS 摘要或配置提示。
- 不新增 resume/cancel/clear error 按钮。
- 进度和 remaining 仍保留，避免错误态丢失上下文。

### 7. Cover Preview

用途：显示 cloud preview/cover 图和任务标题。

横屏重排为图片左、文字右：

| 区域 | 坐标/尺寸 | 内容 |
|---|---|---|
| topbar | `x=12,y=10,w=456,h=28` | `Cover`，source/页码 |
| cover image | `x=12,y=48,w=240,h=240` | contain，来自 `preview_blob` |
| text panel | `x=264,y=48,w=204,h=240` | 标题、状态、cloud detail、hint |
| bottom hint | `x=264,y=260,w=204,h=24` | 可选 loading/offline |

图片策略：

- 最大 `240 x 240`，横屏下比竖屏 `296 x 296` 小，但右侧可同时显示文本。
- 无图时同框显示 `Loading cloud cover`、`No active print`、`Printer offline`。
- 点击 cover image no-op。

草图：

```text
+------------------------------------------------+
| Cover                                      7/8 |
| +----------------------+ +-------------------+ |
| |                      | | Bambu_P1S_...     | |
| |      240 image       | | Preparing cover   | |
| |                      | | PLA matte black   | |
| +----------------------+ | Source: cloud     | |
|                          | click: no-op      | |
+------------------------------------------------+
```

### 8. Camera

用途：显示本地相机快照，并保留 tap refresh。

横屏重排：

| 区域 | 坐标/尺寸 | 内容 |
|---|---|---|
| topbar | `x=12,y=10,w=456,h=28` | `Camera`、状态/source |
| camera image | `x=12,y=48,w=300,h=224` | RGB565，center/contain |
| info panel | `x=324,y=48,w=144,h=224` | status、layer、source、refresh、tap hint |
| bottom hint | 默认隐藏；仅无图/错误时可用 `x=12,y=286,w=456,h=18` | 简短错误或 `Tap for new image`，不得重复右栏文案 |

图片策略：

- 横屏优先给相机图 `300 x 224`，接近原项目 `400 x 224` 的横向快照比例。
- 仅保留双 buffer 等效策略，不增加相机缓存。
- 图像区域和相机页小位移 tap 均表达“刷新”；实现仍沿用原 `camera_refresh_requested_`。
- 无图状态保留图像框，显示 `Tap for new image`、`Camera offline` 或 `camera_detail`。

右侧信息面板实机修订：

- 保留 `300 x 224` 相机图，不为右栏牺牲图像宽度。问题根因不是图像过宽，而是右栏 `120 px` 内容宽度内塞入重复长句。
- 右栏内容顺序固定为：status/lifecycle -> layer -> source -> refresh cadence -> tap hint。
- 文案必须短化并去重：`Auto-refresh while open` 在右栏最多出现一次，优先显示为 `Auto 2s`；`Tap image area to refresh` 不进入右栏，右栏只用 `Tap image`。
- image mode：status 在 `x=336,y=60,w=120,h=24`，`DOT` 1 行；layer 在 `x=336,y=90,w=120,h=24`，`DOT` 1 行；source 在 `x=336,y=124,w=120,h=20`；refresh 在 `x=336,y=156,w=120,h=20`；tap hint 在 `x=336,y=198,w=120,h=20`。
- no-image mode：图像框中央显示主提示，`x=34,y=136,w=256,h=48`，`WRAP` 最多 2 行；右栏仍可显示 source/refresh/tap hint，但不得重复图像框主提示。
- bottom hint 在 image mode 必须隐藏；无图或错误态如果确需显示，放到 `y=286,h=18`，字号 `11-12`，`DOT` 1 行，离底边至少 `14 px`，不得被屏幕边框遮挡。

根因记录：

- 2026-06-06 实机 Camera 截图显示旧规格允许 `Source: waiting\nAuto-refresh while open`、`Auto-refresh...`、`Tap image to refresh` 和底部 `Tap image area to refresh` 同时出现，造成重复、截断和贴底。此处属于“设计文案策略过满 + 实现把长句直接塞入 120 px label”两者都有；修订后实现线程必须先删重复文案、隐藏 image mode bottom hint。

草图：

```text
+------------------------------------------------+
| Camera                         Printing local  |
| +----------------------------+ +-------------+ |
| |                            | | Printing    | |
| |       300 x 224 cam        | | L122/350    | |
| |                            | | Source local| |
| +----------------------------+ | Auto 2s     | |
|                                | Tap image   | |
+------------------------------------------------+
```

## 全局交互契约

横屏沿用竖屏 `Interaction Contract`，但因为当前 ONX 竖屏实现已把 ONX pager 处理为离散翻页，横屏也应采用离散翻页，不要求半页跟手滚动。

| 交互 | 横屏要求 |
|---|---|
| 首触唤醒 | 屏幕 off 时第一次触摸只唤醒并消费 |
| 水平翻页 | `abs_dx >= 24` 且满足原轴锁条件；向左进入下一个已启用页，向右进入上一个已启用页 |
| 垂直亮度 | `abs_dy >= 24`、`abs_dx <= 18`，显示居中亮度 overlay |
| 长按 PIN | 非 scrolling、非亮度 overlay、非横滑时触发 portal unlock request |
| Printer card | 整卡点击切换 profile |
| Logo | 仅在支持 chamber light 时点击切换灯光 |
| Remaining row | 点击切换 remaining/ETA |
| Camera tap | 相机页小位移 tap 请求刷新 |
| AMS tray / Cover / Metrics / Error | 展示或 no-op，不新增控制 |

Overlay 优先级不变：

1. Brightness overlay
2. PIN overlay
3. Portal hint
4. Error/detail
5. 普通页面内容

## 数据绑定边界

横屏 UI 元素必须来自现有字段：

| UI 元素 | 数据来源 |
|---|---|
| 页面可用性 | `snapshot.ams->count`、`preview_page_available`、`camera_page_available` |
| 打印机列表 | `Ui::PrinterCardInfo` |
| 状态文本 | `lifecycle_label(snapshot)` |
| detail/error | `detail_text(snapshot)`、`has_error`、`hms_codes` |
| 任务名 | `job_name`，fallback 可用 `gcode_file`、`preview_title`，具体顺序由实现确认 |
| 进度 | `progress_percent` |
| 层数 | `current_layer`、`total_layers` |
| remaining/ETA | `remaining_seconds`、本地时钟 |
| 温度 | `nozzle_temp_c`、`bed_temp_c`、`secondary_nozzle_temp_c`、`chamber_temp_c` 与 known flags |
| 电池 | `battery_present`、`battery_percent`、`charging`、`usb_present` |
| chamber light | `chamber_light_supported`、`chamber_light_state_known`、`chamber_light_on` |
| AMS | `AmsSnapshot`、`AmsUnitInfo`、`AmsTrayInfo`、`tray_now`、`tray_tar` |
| Cover | `preview_blob`、`preview_url`、`preview_title`、`preview_hint`、`cloud_detail` |
| Camera | `camera_blob`、`camera_width`、`camera_height`、`camera_detail`、`camera_source` |
| Portal/PIN | `set_portal_access_state(...)` 参数 |

不确定或未直接绑定的内容必须标记“待前端线程在代码中确认”，不能凭空定义协议字段。

## H2C Cloud 实机补充规范

本节来自 H2C Cloud 实机联调后的 UI handoff。App/Protocol 侧已验证 CN Cloud 登录、H2C Cloud MQTT 状态/AMS/进度/层数、Cover PNG 下载/缓存/解码和 Printer 页 cloud fallback card 数据链路可用；以下问题不得再归因到协议层，后续实现线程必须在横屏 UI/layout/text 层处理。

### 根因分类

| 问题 | 分类 | 设计判断 |
|---|---|---|
| CJK 文件名/标题显示方框 | 字体资源/glyph coverage 问题 | 协议数据已是 UTF-8 并到达 UI；当前字体缺少 CJK 字形时不得直接显示 tofu boxes |
| Printer 页 cloud serial 过长 | 布局/文案优先级问题 | cloud-only 无本地 profile 时 serial 可作为数据来源，但不能作为大标题完整显示 |
| Cover title/detail/loading 冲突 | 布局/状态优先级问题 | image blob 已可显示后，loading 文案不得继续占据或覆盖主要视觉状态 |
| Wi-Fi/Web Config 横屏不可发现 | 横屏移植漏项 | 原 UI 已有 AP/IP/PIN 文案语义；横屏必须在 Printer/Main 持续暴露入口，而不是只依赖开机 intro hint |
| 2026-06-06 复验 Main 标题仍有方框 | 字体/fallback 策略不足 | ASCII-safe + `[UTF8]` 仍不满足验收，且当前 Dosis/Montserrat 对标记字符也可能缺 glyph；必须启用 CJK 字体优先显示真实 UTF-8 |
| 2026-06-06 复验 Printer 卡片裁切 | 坐标/高度问题 | `62..70 px` 卡片高度不足以承载三行真实字体；必须改为 `92 px` 或删减为两行 |
| 2026-06-06 复验 Printer 右栏拥挤 | 文案拆分问题 | `Open <ip> | Hold for PIN` 不能塞进 `120 px` label；必须拆成短行 |

### CJK 文本策略

横屏 demo 当前阶段改为 CJK 字体优先策略，目标是“真实显示 UTF-8 用户文本，且不显示方框”：

- 使用项目内生成的 `onx_cjk_16` 作为首选 CJK 字体，用于动态用户文本。字体源为 LVGL 依赖中开源 `SourceHanSansSC-Normal.otf` 生成的 16 px 子集。
- 当前子集必须覆盖 demo 需要的 ASCII、常用标点，以及 `测试中文文件名打印机配置项目模型板载云端本地状态在线离线设置访问码温度进度层数封面图片失败等待刷新自动来源任务名称打开长按详情喷嘴热床剩余时间完成暂停错误材料颜色预览相机网络连接成功`。
- 若后续实机发现新字符缺 glyph，优先扩展并重新生成 `onx_cjk_16` 子集；不得回退到 `[UTF8]` / `[CJK]` 页面标签。
- 固定英文 UI、数值、单位、IP、PIN、serial、温度、进度、层数等机器字段继续使用现有 Dosis/Montserrat/MDI 字体，不扩大字体影响面。
- 不修改 `PrinterSnapshot` 或协议原始字符串，不改文件名来源；只在 UI label helper 和具体 label font 选择中处理显示。
- `[UTF8]` / `[CJK]` 不得作为正常用户页面输出；这类标记只能用于日志或极端 fallback。
- CJK label 必须使用 UTF-8 原文或 UTF-8 原文的安全截断结果；不得先逐字节过滤成 ASCII fragment。
- 如果后续发现某个字符不在 `onx_cjk_16` 覆盖范围内，页面 fallback 只能显示无括号 ASCII 语义文本，例如 `Project name` 或 `Printer name`，不得显示 `[UTF8]` 标记。
- 所有 CJK label 仍必须应用本规范的 width/long mode/行数限制；中文显示不能遮挡 layer、detail、IP/PIN 或 Cover 图片。

必须纳入 CJK 字体策略的文本源：

| UI 文本 | 现有来源 | 横屏显示要求 |
|---|---|---|
| Main job title | `job_name`，fallback `gcode_file`、`preview_title` 待实现线程按现有代码确认 | `x=220,y=48,w=248,h=38`，CJK 16；最多 2 行或 DOT，底部不得越过 `y=86`，不得遮挡 layer |
| Cover title | `preview_subnote_text(snapshot)`：`job_name`、`preview_title`、`cloud_detail`、`preview_hint` | `x=276,y=60,w=180,h=72`，CJK 16 或 14；最多 2-3 行，优先 2 行，不得显示方框 |
| Cover detail | `cloud_detail`、`preview_hint`、source/detail 派生短句 | `x=276,y=142,w=180,h=96`，CJK 14；最多 4 行，不得与 title 重叠 |
| Printer card name | `Ui::PrinterCardInfo.name`、`model`、cloud fallback 名称 | card 内 `x=12,y=10,w=208,h=24`，CJK 16，单行 DOT |
| Printer detail title | active `PrinterCardInfo` name/model/cloud fallback | `x=336,y=60,w=120,h=40`，CJK 16 或 14，最多 2 行；raw serial 禁止 |
| Detail/error 普通文案 | `detail_text(snapshot)`、`cloud_detail`、`preview_hint` | 可 WRAP 但不得超过对应 label 高度；含用户标题时使用 CJK 14，错误/HMS 优先短摘要 |
| Setup SSID | `setup_access_text(snapshot)` 中 `setup_ap_ssid` | SSID 可使用 CJK 14；`PW` 与 `Open <ip>` 必须完整可读 |

风险边界：

- `main/include/font/onx_cjk_16.c` 是项目内 CJK 子集字体源码，约 100 KB；不启用 LVGL 内置全量/大子集 CJK Kconfig 字体。
- 主线程需要 build 验证最终 app size；C 源文件大小不等同最终二进制增量，验收以 portrait/landscape build 产物和 partition report 为准。
- 若 build 余量不可接受，主线程再决定更小子集或 14 px 子集方案。

### Printer 页 cloud serial 策略

cloud-only 无本地 profile 时，允许用 cloud resolved serial 生成卡片，但横屏显示优先级必须固定如下：

1. Primary title：用户昵称/display name 优先；其次 printer model/family；如果只有 serial，显示 `Cloud printer`，不得把完整 serial 放到 primary title。
2. Secondary line：显示 model + shortened serial；如果 model 未知，显示 `SN <head6>...<tail4>`。
3. Detail panel：标题同 primary；state 显示 `Online`、`Waiting for LAN`、`Profile ready` 等短状态；host/serial 行使用 shortened serial 或 local IP，不得换行显示完整 serial。
4. Active badge/pill 固定 `h=22,min_w=32`，不得被 serial 挤压。若空间不足，缩短 serial，不缩小触控卡片。

推荐 shortening 规则：

| 原始字段 | 显示 |
|---|---|
| serial 长度 `<= 12` | 原样显示 |
| serial 长度 `> 12` | `SN ` + 前 6 字符 + `...` + 后 4 字符 |
| 有 model + serial | `<model>  ` + 后 4 字符，或 detail 中 `SN <head6>...<tail4>` |
| 无 nickname/model，仅 serial | title `Cloud printer`，secondary `SN <head6>...<tail4>` |

横屏坐标约束：

| 元素 | 坐标/尺寸 | long mode |
|---|---|---|
| card container | `x=12,y=48 + n*102,w=300,h=92` | card clickable |
| card primary | card 内 `x=12,y=10,w=208,h=24` | `DOT` 1 行 |
| card secondary/model/serial | card 内 `x=12,y=40,w=252,h=18` | `DOT` 1 行 |
| card state/host | card 内 `x=12,y=64,w=252,h=18` | `DOT` 1 行；底部留白 `>=8 px` |
| detail title | `x=336,y=60,w=120,h=40` | `WRAP` 最多 2 行；raw serial 禁止 |
| detail state | `x=336,y=108,w=120,h=20` | `DOT` 1 行 |
| detail serial | `x=336,y=136,w=120,h=20` | `DOT` 1 行 |
| detail web label | `x=336,y=168,w=120,h=16` | `DOT` 1 行，`Web Config` 或 `Open` |
| detail ip | `x=336,y=186,w=120,h=20` | `DOT` 1 行，只放 IP |
| detail pin hint | `x=336,y=214,w=120,h=18` | `DOT` 1 行，`Hold PIN` / `PIN active` |

2026-06-06 复验后强制修订：

- 旧 `h=62..70` 卡片高度禁止继续使用；真实字体下第三行会被卡片底部裁切。
- 如果保留三行卡片，必须使用 `h=92`；如果要改成 `h=76..80`，只能显示 title + secondary 两行，state 必须变成小点/pill，不能再绘制第三行文本。
- 右侧详情面板必须拆出独立 IP 行。禁止显示 `Open 192.168.0.139 | Hold for PIN` 这类组合长句。
- Printer 页底部 `Hold for Web Config PIN` 禁止使用 `dosis_32`/`info20` 等大字号；默认隐藏。确需显示时只用 `Montserrat 11`、`h=14`、文案 `Hold PIN`。

### Cover title/detail/loading 优先级

Cover 页必须按状态优先级渲染，不允许多个状态文案同时竞争主视觉：

| 状态 | image box | cover title | detail lines |
|---|---|---|---|
| `preview_blob` 已解码且 image visible | 显示 contain image | 显示 job/preview title；CJK font 最多 2 行优先 | 不显示 `Loading cloud cover`；只显示 source/material/短 cloud detail |
| 有 `preview_url` 但无 blob | image box 中心显示 `Loading cloud cover` | 若已有 title 可显示在右侧 | detail 不重复 loading；可显示 `Source: cloud` 或短错误 |
| 无图但打印中/准备中 | image box 中心显示 `Preparing cover` | 显示 job/preview title | detail 显示 source/detail |
| offline/setup | image box 中心显示 `Printer offline` 或 `Set up printer` | title 可隐藏 | detail 优先显示 AP/IP/Web Config 访问提示 |
| error/HMS | image box 如有图仍显示图 | title 保留 | error/detail 只用短摘要，不覆盖 title |

实现约束：

- `preview_note_text(snapshot)` 在 `preview_blob` 非空且 image visible 后必须为空；横屏上 `page2_note_` 应隐藏。
- `Loading cloud cover` 只能出现在 image box 无图状态，不能同时出现在右侧 detail。
- `page2_detail_label_` 横屏 `x=276,y=142,w=180,h=96`，只允许短 detail，不得把 title、loading、HMS 长句全部拼接。
- 验收日志出现 `ui preview set_src raw` 或等效 image set_src 后，Cover 页必须显示图片并且屏幕上没有 `Loading cloud cover`。

### Wi-Fi / Web Config 横屏可发现性

横屏必须保留原项目 Web Config/PIN 交互语义：长按 PIN 不变，UI 不生成 PIN，不新增 Web Config API。UI 只展示现有 `setup_access_text(snapshot)`、`station_portal_text(snapshot)`、portal state 和 `wifi_ip` 派生出的提示。

强制显示规则：

| 状态 | Printer 页 | Main 页 | topbar |
|---|---|---|---|
| setup AP active | 空态/详情面板必须显示 `AP: <ssid>`、`PW: <password>`、`Open <setup_ap_ip or 192.168.4.1>` | bottom portal hint 显示 `AP <ssid>  Open <ip>` 或 `Open 192.168.4.1` | 可显示 `Setup AP`，不得替代 IP 行 |
| waiting credentials，AP inactive，Wi-Fi connected | 详情面板显示 `Open <wifi_ip>` 或 `Open the portal on your Wi-Fi IP` | bottom portal hint 持续显示 `Open <wifi_ip>` | 可显示 `Web <wifi_ip>` |
| Wi-Fi connected 但 printer/cloud/source 未完成 | 详情面板分行显示 source 状态、`Web Config`、`<wifi_ip>`、`Hold PIN` | Main bottom portal hint 可显示 `Open <wifi_ip> | Hold PIN`，不能只在开机 intro 时间内显示 | 若无更高优先级错误，右侧 meta 显示 `Web <wifi_ip>` |
| portal unlocked/session active | 详情面板显示 `Web Config unlocked` + `Open <wifi_ip>` | bottom hint 显示 `Web Config unlocked <remaining>` | 可显示 `Web unlocked` |
| PIN active | PIN overlay 显示原 PIN/title/detail | 页面底部 hint 可显示 `Enter the PIN shown` | overlay 优先于 topbar |
| Wi-Fi disconnected 且无 setup AP | 显示 `No Wi-Fi` / `Use Web Config when available` | 可显示 `Hold for PIN`，但不能伪造 IP | `Offline` |

横屏 discoverability 验收重点：用户通过 setup AP 输入 Wi-Fi 密码后设备重启，若已获得 station `wifi_ip`，不需要等待开机 intro，也不需要猜 IP；在 Printer 页空态/详情面板和 Main 页底部都能看到访问 Web Config 的 IP 或 PIN 提示。

Printer 页 Web Config 文案强制拆分：

- `page0_detail_host_` 不应同时承载 serial、IP 和 PIN；若现有实现只有 `host`/`hint` 两个 label，必须调整 label 分布或复用为短行，不得输出组合长句。
- 右侧详情面板优先显示 `Web Config` label + 单独 IP 行 + `Hold PIN`。IP 行必须完整显示 `192.168.x.x`；若 120 px 下仍被 DOT，字体降到 Montserrat `12`，不允许换成组合句。
- Printer 页底部只作为辅助，不承担 IP discoverability 主职责。

### UI Implementation 执行指令草案

交给 UI Implementation 线程时，代码范围应限定为：

- 允许：`main/src/ui.cpp` 中字体引用、动态文本 helper、相关 label font 切换、横屏 label 文案、long mode、Cover/Printer text priority。
- 如确有必要：`main/include/printsphere/ui.hpp` 中增加 UI 内部 helper 声明或 layout/font profile 常量。
- 允许新增项目内字体子集源文件，并在 `main/CMakeLists.txt` 中接入该字体源。
- 禁止：`components/**`、BSP、cloud/local protocol client、Web Config/setup portal、证书、NVS schema、sdkconfig。
- 实现完成后必须由实现线程构建 `portrait` 与 `landscape` 两个 profile；烧录和实机验证交由主线程执行。

实现线程不得新增协议字段或控制命令；所有显示均从现有 snapshot/card/portal state 派生。若某字段来源不明确，必须回报主线程/设计线程确认，不得自行改 App/Protocol。

## 实现边界

- 横屏应作为 layout profile 实现，例如 `ONX portrait` 与 `ONX landscape` 两套坐标/构建 profile。
- 横屏只改变 LVGL 对象尺寸、坐标、可见性、文字截断策略和图片 contain bounds。
- 不改变 `Application -> PrinterSnapshot -> Ui` 的数据流。
- 不改变 `consume_camera_refresh_request()`、`consume_chamber_light_toggle_request()`、`consume_printer_switch_request()`、`consume_portal_unlock_request()` 的语义。
- 不在 UI 层新增 printer control command。
- 不在 UI 层新增 Web Config API。
- 不在 UI 层新增相机解码协议或缓存策略。

## 文本溢出策略

| 类型 | 策略 |
|---|---|
| topbar 标题 | `DOT`，1 行 |
| 任务名 | 横屏 demo 使用 CJK font 真实显示 UTF-8；Main/Cover 最多 2 行；Main 空闲态优先单行 `DOT`；超出省略 |
| detail/error | 普通态最多 2 行 `WRAP`；错误可短 marquee，但避免大面积滚动 |
| Main metric label/value | label/value 均 `DOT` 1 行；value 优先，aux 可隐藏 |
| Main metric aux | 默认隐藏；显示时 `DOT` 1 行且不得与 value 同坐标 |
| printer title/name | 优先 nickname/model；raw serial 不做标题；横屏 demo 使用 CJK font |
| printer host/model/serial | `DOT`，1 行；serial 使用 `SN <head6>...<tail4>` 或 model + tail |
| material name | `DOT`，1 行；材料类型必须保留 |
| Cover title | CJK font 最多 2 行优先；不得显示方框；不得遮挡 detail |
| Cover loading/detail | 有 image blob 时隐藏 loading；detail 只显示短 source/detail，不拼接长 loading |
| Camera image-mode status/layer/source/refresh/tap | 全部 `DOT`，各 1 行；右栏不允许 `WRAP` 长句 |
| Camera no-image 主提示 | 图像框中央 `WRAP`，最多 2 行 |
| Camera bottom hint | image mode 隐藏；无图/错误态 `DOT` 1 行 |
| Web Config/IP hint | AP/IP/PIN 访问提示优先可读；Printer 页拆分 IP/PIN，Main 页才允许短组合 hint；IP、密码、PIN 不得因省略策略丢失关键字段 |

## 主线程交接与开发分片建议

横屏视觉方向已确认，可交回主线程审查并安排 UI 实现线程。建议开发按以下小切片推进，避免把方向、BSP、协议和页面重排混在一次大改里。

| Slice | 范围 | 文件边界 | 验收 |
|---|---|---|---|
| L1 layout profile | 增加 `480 x 320` landscape layout/profile 选择，确保 pager/root/overlay 使用横屏尺寸 | board/layout config + UI layout constants；不改协议 | 根画布、触摸坐标、overlay 覆盖全屏 |
| L2 Main page | 实现 Main 左状态卡 + 右信息区；按实机修订使用 `120 x 52` metric card、隐藏冲突 aux | 仅 UI 页面对象与坐标 | 进度、状态、任务名、层数、温度、remaining/ETA、logo/light 都不重叠 |
| L3 Printer + pager | Printer Select 横屏列表、`92 px` 卡片、右侧 IP/PIN 拆分、离散左右翻页 | UI 页面对象/事件 | card 第三行不裁切；右栏 IP/PIN 可读；card 切换、禁用页 skip/clamp、首触唤醒/亮度/PIN 不回归 |
| L4 AMS | AMS 横向四槽 row + EXT + error/no-op 边界 | UI AMS 布局/render | 1-4 AMS 按 count 启用；tray 点击 no-op |
| L5 Cover | Cover 左图右文、CJK font、loading/detail 优先级 | UI preview image bounds/text | `preview_blob` contain；image visible 后 loading 消失；真实 UTF-8 标题不显示方框 |
| L6 Camera | Camera `300 x 224` + 去重后的 info panel；image mode 隐藏 bottom hint | UI camera bounds/text | tap refresh 请求，离开/禁用释放图像，缓存策略不增加 |
| L7 Cloud text + portal regression | CJK 用户文本、H2C cloud serial、Cover title、Web Config/AP/IP 可发现性 | UI text/layout only | Main/Cover/Printer 使用 CJK font 显示真实 UTF-8；长 serial 不撑爆；Printer/Main 可持续看到 Web Config 入口 |
| L8 overlays/regression | PIN、brightness、portal hint、错误态、文本溢出总验收 | UI overlay/text only | 按本文档验收用例通过 |

开发线程必须在每个 slice 回报：修改文件、是否触及非 UI 代码、截图/实机照片或日志证据、未通过项。主线程再决定是否进入下一 slice。

## Demo-first 范围

必须第一版实现：

- 横屏 480 x 320 根页面、横向 pager 和页面启用/跳过。
- Printer Select 横屏列表。
- Main 横屏左右分栏，包含进度、状态、任务名、层数、温度、remaining/ETA、battery、portal hint、logo/light。
- AMS 1 横屏四槽一行；AMS 2-4 按 count 启用。
- Cover 左图右文。
- Camera 大图 + 右侧信息面板 + tap refresh。
- Brightness overlay、PIN overlay。
- no-op 边界。
- H2C Cloud demo 文本策略：横屏用户动态文本使用项目内 `onx_cjk_16` 子集字体；cloud serial shortened；Printer 页 `92 px` 卡片和拆分 IP/PIN；Cover image visible 后隐藏 loading；横屏 Printer/Main 可发现 Web Config IP/PIN。

可后续：

- 更细 AMS unit-level error 展示。
- 更复杂的横屏页面指示器。
- 小圆弧进度样式。
- 扩展 MDI glyph。
- 按后续真实文件名扩展 `onx_cjk_16` 字体子集。
- 横屏/竖屏运行时切换；第一版可编译期选择。

## 验收标准

### HTML 视觉验收

- `docs/ui_design_landscape_preview.html` 可直接浏览器打开。
- 所有设备 mock 都是 `480 x 320` 横屏比例。
- 至少展示 Printer Select、AMS、Main printing、Main error/paused、Cover、Camera、PIN/brightness overlay。
- 页面不是竖屏缩放；Main、Cover、Camera 必须体现左右分栏。
- 可点击区域在视觉稿中有明确边界或说明。

### 代码实现验收

- 通过 board/layout profile 选择 portrait/landscape，不改协议、不改 App 业务逻辑。
- 横屏根画布、pager、overlay、图片 bounds 使用 `480 x 320`。
- 禁用页 skip/clamp 行为与竖屏一致。
- 所有文本在 480 x 320 内不重叠、不越界；Main metric 和 Camera right panel 必须用实机修订后的 long mode/短文案策略。
- H2C Cloud 场景下 Main job title、Cover title、Printer display name 必须使用 CJK font 显示真实 UTF-8 文本，不得显示 tofu 方框或页面级 `[UTF8]` / `[CJK]` 标记。
- Printer 页 cloud-only serial 不得作为完整大标题换行；卡片和详情面板必须使用 nickname/model 优先和 shortened serial。
- Printer 页卡片必须使用 `w=300,h=92,gap=10` 或两行删减方案；若显示三行，第三行必须完整可见且底部留白 `>=8 px`。
- Printer 页右侧详情必须拆分 `Web Config`、`<ip>`、`Hold PIN`，禁止在 `120 px` label 内显示 `Open <ip> | Hold for PIN`。
- Printer 页底部禁止大号 `Hold for Web Config PIN`；默认隐藏，必要时 `Montserrat 11` + `Hold PIN`。
- Cover 页在 `preview_blob` 已 set_src/visible 后必须隐藏 `Loading cloud cover`；title/detail/source 不重叠。
- setup AP、waiting credentials、Wi-Fi connected but source not ready、portal unlocked/PIN active 等状态下，Printer 页和 Main 页必须可发现 `Open <ip>` 或 PIN 提示。
- AMS tray、Cover image、指标卡片、错误面板点击 no-op。
- 相机页不增加超过现有策略的图片缓存。

### 实机交互验收

- 横屏触摸映射在 UI 中表现为左滑下一页、右滑上一页；触摸目标位置与视觉位置一致。
- 屏幕 off 首触只唤醒。
- 竖向拖动只调亮度，不翻页。
- 长按显示/请求 Web Config PIN。
- Printer card 切换 profile。
- Logo light toggle 只在支持灯光时生效。
- Remaining/ETA 行可切换。
- Camera 页 tap refresh 请求生效。
- 禁用 Cover/Camera/AMS 页不可达。
- AP 配网后重启且拿到 station IP 时，横屏不依赖 intro timeout；在 Printer 页空态/详情面板与 Main 页底部都能读到 Web Config 访问入口。

## 已确认的横屏设计决策

- Main 页采用“左状态卡 + 右信息区”的横屏主形态。
- Camera 图像区域采用 `300 x 224`，右侧信息面板 `144 x 224`。
- Cover 页采用左侧 `240 x 240` 图片 + 右侧 `204 x 240` 文本面板。
- 横屏第一版保留原项目 8 个逻辑页面，不合并 AMS 为总览页。
- 视觉方向以 `docs/ui_design_landscape_preview.html` 为准；落地参数以本文档为准。
