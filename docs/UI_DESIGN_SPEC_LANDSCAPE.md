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
- 不修改 `main/**`、`components/**`、`examples/**`、协议、BSP、构建脚本、sdkconfig。
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
| Printer card | `300 x 66` | 点击切换 profile |
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
| printer list | `x=12,y=48,w=300,h=228` | transparent | 垂直滚动 |
| card 1 | `x=12,y=48,w=300,h=66` | bg card | 点击 |
| card 2 | `x=12,y=122,w=300,h=66` | bg card | 点击 |
| card 3 | `x=12,y=196,w=300,h=66` | bg card | 点击 |
| active stripe | card 内 `x=0,y=0,w=5,h=card_h` | `#4ADE80` | active 时显示 |
| card name | card 内 `x=10,y=8,w=210,h=20` | `17/#F7FAFC/bold` | 单行省略 |
| card pill | card 内右上 `h=22,min_w=32` | `11/#C8D1DC` | active/index |
| card model/host | card 内 `x=10,y=31,w=260,h=14` | `12/#8B98A8` | 单行省略 |
| card state | card 内 `x=10,y=48,w=260,h=14` | `12`, 状态色 | online/wait/offline |
| detail panel | `x=324,y=48,w=144,h=228` | card | 展示 active printer |
| bottom hint | `x=12,y=292,w=456,h=20` | `12/#8B98A8` | 无 |

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
| job title | `x=220,y=50,w=248,h=44` | `19/#F7FAFC/bold` | 最多 2 行 |
| layer | `x=220,y=98,w=248,h=20` | `15/#C8D1DC/bold` | 无 |
| metric grid | `x=220,y=132,w=248,h=84` | 2 col, gap `8` | 无 |
| metric card 1 | `x=220,y=132,w=120,h=38` | card | no-op |
| metric card 2 | `x=348,y=132,w=120,h=38` | card | no-op |
| metric card 3 | `x=220,y=178,w=120,h=38` | card | no-op |
| metric card 4 | `x=348,y=178,w=120,h=38` | card | no-op |
| metric label | card 内 `x=8,y=5,w=104,h=12` | `11/#8B98A8/bold` | 无 |
| metric value | card 内 `x=8,y=20,w=104,h=18` | `20/#F7FAFC/bold` | 无 |
| detail/error card | `x=220,y=222,w=248,h=54` | card; error border/background red | no-op |
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
| layer title | `x=336,y=60,w=120,h=28` | `23/#F7FAFC/bold` | 无 |
| layer value | `x=336,y=90,w=120,h=28` | `23/#F7FAFC/bold` | `122 / 350` |
| info lines | `x=336,y=130,w=120,h=84` | `13/#8B98A8` | auto refresh/source |
| tap hint | `x=336,y=226,w=120,h=18` | `13/#87CEEB` | refresh request only |
| bottom hint | `x=12,y=292,w=456,h=20` | `12/#8B98A8` | 可隐藏 |

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
| 列表 | `x=12,y=48,w=300,h=228` | 打印机卡片，垂直滚动 |
| 详情/空态面板 | `x=324,y=48,w=144,h=228` | active printer 状态、host、Web Config 提示 |
| 底部 hint | `x=12,y=292,w=456,h=20` | `Hold for PIN` 或 `Open <ip>` |

卡片：

- 尺寸 `w=300,h=62..70`，间距 `8`。
- 整卡点击，触发 `pending_printer_switch_`。
- active 使用左色条 `5 px #4ADE80`。
- connected 用文字 + 小点；不能只靠颜色。

草图：

```text
+------------------------------------------------+
| Printers                              Wi-Fi    |
| +------------------------------+ +------------+ |
| | Studio P1S              act  | | Active     | |
| | P1S 192.168.1.24 Online     | | 192.168... | |
| +------------------------------+ | Hold PIN   | |
| | A1 Mini                 2    | | Web Config | |
| | Waiting for LAN              | +------------+ |
| +------------------------------+                |
| Hold for Web Config PIN                         |
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

右信息区：

- 任务名：`x=220,y=50,w=248,h=42`，最多 2 行。
- Layer：`x=220,y=98,w=248,h=24`。
- 指标网格：`x=220,y=132,w=248,h=78`，2 列 x 2 行，卡片 `119 x 36` 或 `119 x 38`。
- detail/error：`x=220,y=220,w=248,h=56`，错误态红边，普通态灰字。

草图：

```text
+------------------------------------------------+
| 87% Printing                         BAT 92%  |
| +------------------+ +------------------------+ |
| | [logo] 87%       | | Bambu_P1S_gearbox...  | |
| |        Printing  | | Layer 122 / 350       | |
| | [progress bar]   | | Nozzle 220C  Bed 65C  | |
| | Remaining 1h24m  | | Aux L219C    Ch 38C   | |
| +------------------+ | Detail / error text    | |
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
| info panel | `x=324,y=48,w=144,h=224` | lifecycle、layer、refresh hint、source |
| bottom hint | `x=12,y=292,w=456,h=20` | `Tap image to refresh` 或错误 |

图片策略：

- 横屏优先给相机图 `300 x 224`，接近原项目 `400 x 224` 的横向快照比例。
- 仅保留双 buffer 等效策略，不增加相机缓存。
- 图像区域和相机页小位移 tap 均表达“刷新”；实现仍沿用原 `camera_refresh_requested_`。
- 无图状态保留图像框，显示 `Tap for new image`、`Camera offline` 或 `camera_detail`。

草图：

```text
+------------------------------------------------+
| Camera                         Printing local  |
| +----------------------------+ +-------------+ |
| |                            | | Layer       | |
| |       300 x 224 cam        | | 122 / 350   | |
| |                            | | Auto 2s     | |
| +----------------------------+ | Tap refresh | |
| Tap image to refresh            Source local  |
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
| topbar 标题 | 单行省略 |
| 任务名 | Main/Cover 最多两行；超出省略或短 marquee |
| detail/error | 2 行内优先；错误可短 marquee，但避免大面积滚动 |
| printer host/model | 单行省略 |
| material name | 单行省略；材料类型必须保留 |
| camera/detail hint | 1-2 行，超出省略 |

## 主线程交接与开发分片建议

横屏视觉方向已确认，可交回主线程审查并安排 UI 实现线程。建议开发按以下小切片推进，避免把方向、BSP、协议和页面重排混在一次大改里。

| Slice | 范围 | 文件边界 | 验收 |
|---|---|---|---|
| L1 layout profile | 增加 `480 x 320` landscape layout/profile 选择，确保 pager/root/overlay 使用横屏尺寸 | board/layout config + UI layout constants；不改协议 | 根画布、触摸坐标、overlay 覆盖全屏 |
| L2 Main page | 实现 Main 左状态卡 + 右信息区 | 仅 UI 页面对象与坐标 | 进度、状态、任务名、层数、温度、remaining/ETA、logo/light 都不重叠 |
| L3 Printer + pager | Printer Select 横屏列表和离散左右翻页 | UI 页面对象/事件 | card 切换、禁用页 skip/clamp、首触唤醒/亮度/PIN 不回归 |
| L4 AMS | AMS 横向四槽 row + EXT + error/no-op 边界 | UI AMS 布局/render | 1-4 AMS 按 count 启用；tray 点击 no-op |
| L5 Cover | Cover 左图右文 | UI preview image bounds/text | `preview_blob` contain，no image/offline/loading 文案正确 |
| L6 Camera | Camera 300x224 + info panel | UI camera bounds/text | tap refresh 请求，离开/禁用释放图像，缓存策略不增加 |
| L7 overlays/regression | PIN、brightness、portal hint、错误态、文本溢出总验收 | UI overlay/text only | 按本文档验收用例通过 |

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

可后续：

- 更细 AMS unit-level error 展示。
- 更复杂的横屏页面指示器。
- 小圆弧进度样式。
- 扩展 MDI glyph。
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
- 所有文本在 480 x 320 内不重叠、不越界。
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

## 已确认的横屏设计决策

- Main 页采用“左状态卡 + 右信息区”的横屏主形态。
- Camera 图像区域采用 `300 x 224`，右侧信息面板 `144 x 224`。
- Cover 页采用左侧 `240 x 240` 图片 + 右侧 `204 x 240` 文本面板。
- 横屏第一版保留原项目 8 个逻辑页面，不合并 AMS 为总览页。
- 视觉方向以 `docs/ui_design_landscape_preview.html` 为准；落地参数以本文档为准。
