# PrintSphere ONX UI 设计规范

状态：视觉方向已确认 / 等待开发实现与实机校准

本文档用于规划将原 PrintSphere 466 x 466 圆形/近圆形 AMOLED LVGL UI，移植到 ONX3248G035 的 320 x 480 竖屏矩形 LCD。`docs/ui_design_preview.html` 已作为视觉方向基准；开发线程应按本文档的尺寸、坐标、颜色、字体和资源约束高保真还原，实机上仅允许为触摸误差、字体渲染差异和 LCD 可读性做小幅校准。

横屏 `480 x 320` 设计另见 `docs/UI_DESIGN_SPEC_LANDSCAPE.md`，静态预览为 `docs/ui_design_landscape_preview.html`。横屏视觉方向已确认，是独立 layout profile 规范，不应通过旋转或等比缩放本文档坐标实现。

## 目标

- 保留原 PrintSphere 的核心信息和交互：打印状态、进度、任务名、温度、层数、剩余时间、封面图、相机图、AMS/耗材、错误提示、打印机选择、Web Config、PIN、亮度手势、滑动翻页。
- 将原 466 x 466 圆屏布局重排为 320 x 480 竖屏矩形布局，而不是简单缩放。
- 给开发线程提供可落地的页面结构、功能分区和资源使用原则。
- 先产出视觉稿供确认，再把本文档升级为正式 UI 开发规范。

## 非目标

- 不实现 LVGL 页面代码。
- 不修改 BSP、硬件驱动、烧录流程或协议解析。
- 不制作品牌营销页或纯视觉展示页。
- 不为了视觉效果大量引入新图片、新图标或复杂动画。
- 第一版不追求 24 小时稳定运行；目标是 demo-first，并保留关键功能可用性。

## 设计原则

- 优先保留原项目行为：ONX UI 应让原 PrintSphere 用户能识别主要功能路径。
- 尽量复用原项目字体和图片资源，减少重新设计和导入图片/图标资源。
- 优先使用现有 Dosis 字体、Material Design Icons 字体子集、`bambuicon_small` 图片。
- 除非现有字体、LVGL 基础图形、文字标签、颜色、Bambu logo 无法表达功能，否则不新增 bitmap/icon 资源。
- ONX 是嵌入式打印监控屏，不是宣传页；布局优先考虑信息密度、可读性、状态优先级和触摸可用性。
- 320 px 宽度是硬约束。所有内容按 320 x 480 竖屏重新组织。
- 动画要克制。优先使用颜色、文字、小范围脉冲、页面滑动，避免高频全屏重绘。

## 可复用资源

现有可直接复用资源：

- `main/include/img/bambuicon.c`
  - `bambuicon_small`，120 x 120，当前作为主状态页 Bambu logo。
  - 新 UI 建议缩放到 48-72 px，用作状态页 logo 或 chamber light 触控入口。
- `main/include/font/dosis_20.c`
- `main/include/font/dosis_32.c`
- `main/include/font/dosis_40.c`
  - 用于数字、百分比、温度、状态标题。
- `main/include/font/mdi_30.c`
  - 现有电池/充电图标子集。
- `main/include/font/mdi_40.c`
  - 现有时钟、喷嘴、热床、同步等图标子集。

不建议作为 UI 图标直接复用的资源：

- `Case/*.jpg` 主要是外壳/实物照片，不是 UI 图标。
- `managed_components/espressif__libpng/libpng/*.png` 是库测试资源，不属于 PrintSphere UI。
- 当前仓库未发现原始 `.svg` 图标或原始 `.ttf` 字体文件；若要新增 MDI 图标，需要后续开发线程确认字体生成流程。

## 页面模型

原 PrintSphere UI 是横向 pager，最多 8 个逻辑页面：

1. 打印机选择页
2. AMS 1 页
3. AMS 2 页
4. AMS 3 页
5. AMS 4 页
6. 主状态页
7. 封面预览页
8. 相机页

ONX UI 保留这 8 个逻辑页面，以便功能归属尽量贴近原代码。不可用页面可以在导航中跳过，但每个逻辑页面都需要有明确的 320 x 480 矩形布局方案。

不计入页面数量的全局覆盖层：

- 亮度覆盖层：竖向拖动触发。
- Web Config PIN 覆盖层：长按触发。
- 可选临时错误/状态 toast：如第一版需要。

## 全局布局规则

- 屏幕尺寸：320 w x 480 h，竖屏。
- 安全内容区域：x 12..308，y 12..468。
- 显式可点击区域最小尺寸：44 x 44 px。
- 页面标题/状态栏高度：24-32 px。
- 正文推荐字体等效尺寸：18-20 px；次要元信息可用 14-16 px。
- 大数字推荐字体等效尺寸：32-40 px，但每页最多突出 1-2 个大数字。
- 横向 pager 仍作为主导航方式，页面顺序保持稳定。
- 页码/页面指示器仅在不干扰状态信息时显示。
- 尽量复用原 PrintSphere 色彩语义：黑色背景、白色主文字、灰蓝色次文字、状态色强调、红/橙代表错误/警告、绿色代表 active/ready。

## 高保真实现参数

本节把 HTML 视觉稿里的布局 token 转成 LVGL 开发约束。开发线程应优先按这些参数实现，不应只凭肉眼重画。

### 画布和坐标系统

- 物理页面：`320 x 480`。
- 每个逻辑页根容器：`x=0, y=0, w=320, h=480`。
- 页面内边距：`12 px`。
- 主内容宽度：`296 px`，即 `320 - 12 * 2`。
- 安全区：`x=12..308, y=12..468`。
- 所有页面背景：纯黑/近黑，使用 `#050607`。
- 普通页面不使用圆角；HTML 原型的外层圆角只是浏览器预览外壳，不应进入 LVGL 页面。
- 页面内容采用自上而下布局，不再使用原圆屏中心绝对环形布局。

### 颜色 token

| 名称 | 色值 | 用途 |
|---|---:|---|
| `bg` | `#050607` | 页面背景 |
| `panel` | `#111418` | 深色面板 |
| `panel_2` | `#171B20` | 次级面板 |
| `line` | `#2D333B` | 边框/分隔线 |
| `muted` | `#8B98A8` | 次要文字 |
| `soft` | `#C8D1DC` | 次强调文字 |
| `text` | `#F7FAFC` | 主文字 |
| `green` | `#4ADE80` | active/ready/printing |
| `blue` | `#60A5FA` | 辅助进度色 |
| `cyan` | `#87CEEB` | 剩余时间/ETA |
| `amber` | `#F0A64B` | paused/PIN/warning |
| `red` | `#EF4444` | error/HMS |

LVGL 实现中应把这些色值集中成常量，避免分散硬编码。

### 字体 token

优先使用现有字体资源，不新增字体文件。

| 语义 | 首选字体 | 等效字号 | 用途 |
|---|---|---:|---|
| 大百分比/大 PIN/亮度 | `dosis_40` | 40-42 | `87%`、`123456`、`80%` |
| 页面标题/状态大字 | `dosis_32` | 24-32 | `Printing`、`Paused`、`AMS 1` |
| 指标数值 | `dosis_32` | 26-28 | `220C`、`65C` |
| 正文/卡片标题 | `dosis_20` 或 LVGL Montserrat 20 | 18-20 | 任务名、打印机名、材料类型 |
| 元信息/说明 | LVGL Montserrat 14/16 或 Dosis 20 缩用 | 12-16 | IP、source、hint、品牌名 |
| 电池图标 | `mdi_30` | 30 | battery/charging |
| 时钟/喷嘴/热床图标 | `mdi_40` | 32-40 | 如空间允许使用 |

注意：HTML 原型用系统字体近似显示，LVGL 落地时以现有 Dosis/MDI 字体为准。若某些中文/符号不在 Dosis 范围内，优先使用 LVGL 内置 Montserrat 或已有项目中更完整的 info 字体路径。

### 通用组件尺寸

| 组件 | 坐标/尺寸 | 说明 |
|---|---|---|
| 页面 `screen` 内边距 | `12` | 所有页面一致 |
| `topbar` | `x=12, y=12, w=296, h=30` | 左标题，右状态/页码 |
| 页标题 | `font 18-20, max w=220` | 单行省略或滚动 |
| 右侧 meta | `font 12-14, max w=68` | BAT、页码、Wi-Fi、ERR |
| 通用卡片圆角 | `8` | 不用大圆角 |
| 通用卡片边框 | `1 px #2D333B` | error 可加粗/变红 |
| pill 标签 | `h=24, pad x=9, radius=12` | active、页码、source |
| 底部 hint | `x=12, y=432..468, w=296, h=32-36` | 顶部分隔线，居中 |
| 明确触控目标 | `min 44 x 44` | 卡片/图片/时间行/logo |
| 触摸可视调试线 | 不进正式 UI | HTML 的蓝虚线仅用于设计说明 |

### 页面级布局参数

#### 页面 1：打印机选择

| 区域 | 坐标/尺寸 | 字体/颜色 | 交互 |
|---|---|---|---|
| 标题栏 | `x=12,y=12,w=296,h=30` | 标题 `dosis_20/#F7FAFC`；右侧 `#8B98A8` | 无 |
| 卡片列表 | `x=12,y=60,w=296,h=348` | 垂直布局，间距 `10` | 可滚动 |
| 打印机卡片 | `w=296,h>=76,pad=10/12` | 名称 `20/#F7FAFC`，详情 `12-14/#8B98A8` | 整卡点击切换 |
| active 色条 | `x=0,w=5,h=card_h,#4ADE80` | active 时显示 | 无 |
| 状态点 | `8 x 8, radius=4` | online 绿、waiting 橙、offline 红 | 配合文字 |
| 底部 Web Config hint | `x=12,y=432,w=296,h=36` | `13/#8B98A8` | 长按由全局处理 |

#### 页面 2-5：AMS

| 区域 | 坐标/尺寸 | 字体/颜色 | 交互 |
|---|---|---|---|
| 标题栏 | `x=12,y=12,w=296,h=30` | `AMS n` + 页码/状态 | 无 |
| 湿度/温度行 | `x=12,y=48,w=296,h=22` | `14/#8B98A8`，左右对齐 | 无 |
| 料槽网格 | `x=12,y=90,w=296,h=214` | 2 列 x 2 行，gap `10` | 可点击预留，不要求第一版动作 |
| 单个料槽 | `w=143,h=102,pad=10,radius=8,border=2` | 材料 `20 bold`，品牌 `12`，余量 `18 bold` | 触控目标同视觉区域 |
| active 标记 | `right=8,bottom=8,w>=34,h=22` | `#4ADE80` 或暗底白字 `>` | 无 |
| error 标记 | 料槽红边 `#EF4444` + `ERR` pill | `ERR` 白字红底 | 无 |
| EXT 行 | `x=12,y=320,w=296,h=44,pad=9/10` | 左 EXT 材料，右 active | 只在 AMS 1 且 EXT active |
| 底部说明 | `x=12,y=432,w=296,h=36` | `13/#8B98A8` | 无 |

若无 AMS：在页面中央 `x=12,y=180,w=296,h=100` 显示 `No AMS connected`，标题 `24/#C8D1DC`，说明 `14/#8B98A8`。实际 pager 可跳过无效 AMS 页。

#### 页面 6：主状态

| 区域 | 坐标/尺寸 | 字体/颜色 | 交互 |
|---|---|---|---|
| 顶部状态栏 | `x=12,y=12,w=296,h=30` | 左 `87% Printing`，右 `BAT 92%` | 无 |
| hero 区 | `x=12,y=52,w=296,h=70` | logo + 任务名/详情 | logo 可点击灯光 |
| logo 按钮 | `x=12,y=57,w=60,h=60,radius=30` | 图像缩到 `52 x 52` | 点击 chamber light toggle |
| 任务名 | `x=90,y=54,w=218,h<=42` | `18-20/#F7FAFC,bold` | 长文本两行或滚动 |
| 任务详情 | `x=90,y=96,w=218,h=18` | `13/#8B98A8` | 无 |
| 进度块 | `x=12,y=140,w=296,h=104,pad=14/12` | 面板 `#111418/#2D333B` | 无 |
| 百分比 | 进度块左 | `dosis_40, 42,#F7FAFC` | 无 |
| 生命周期 | 进度块右 | `dosis_32, 24,#4ADE80`；paused 用 `#F0A64B` | 无 |
| 进度条 | `x=24,y=214,w=272,h=12,radius=6` | 背景 `#20252B`，填充绿到蓝 | 无 |
| 指标网格 | `x=12,y=258,w=296,h=142` | 2 列 x 2 行，gap `10` | 无 |
| 指标卡片 | `w=143,h=66,pad=9/10` | label `13/#8B98A8`，value `28/#F7FAFC` | 无 |
| 时间行 | `x=12,y=412,w=296,h=48` | `26/#87CEEB,bold` | 点击切换 remaining/ETA |
| Web Config hint | `x=12,y=462` 附近，若空间不足可覆盖底部 hint | `13/#8B98A8` | 长按由全局处理 |

错误/暂停态：

- 错误面板放在 hero 下方，建议 `x=12,y=136,w=296,h=78`。
- 错误标题 `28/#EF4444,bold`，说明 `13/#8B98A8`。
- 错误态可以压缩指标网格为 1 行或隐藏 aux 指标，但必须保留主状态、错误详情、进度、至少 nozzle/bed、时间行。

#### 页面 7：封面预览

| 区域 | 坐标/尺寸 | 字体/颜色 | 交互 |
|---|---|---|---|
| 标题栏 | `x=12,y=12,w=296,h=30` | `Cover` + `7/8` | 无 |
| 封面区域 | `x=12,y=60,w=296,h=296,radius=8` | contain 模式 | 无 |
| 无图提示 | 同封面区域中央 | `16/#C8D1DC,bold` | 无 |
| 标题 | `x=12,y=372,w=296,h<=48` | `20/#F7FAFC,bold` | 两行或滚动 |
| 子说明 | `x=12,y=428,w=296,h=20` | `14/#8B98A8` | 无 |
| 底部 hint | `x=12,y=448..468,w=296` | `13/#8B98A8` | 无 |

封面来自运行时 `preview_blob`，不要为 demo 引入静态封面图。

#### 页面 8：相机

| 区域 | 坐标/尺寸 | 字体/颜色 | 交互 |
|---|---|---|---|
| 标题栏 | `x=12,y=12,w=296,h=30` | `Camera` + `8/8` | 无 |
| 状态 pill | `x=12,y=48,w<=180,h=24` | `12/#C8D1DC` | 无 |
| 相机图区域 | `x=12,y=86,w=296,h=166,radius=8` | RGB565 image contain/center | 点击刷新 |
| 图像最大范围 | `296 x 166`，特殊比例可扩到 `296 x 210` | 不裁主画面 | 点击刷新 |
| 信息区 | `x=12,y=270,w=296,h=120` | `18/#C8D1DC`，行距约 `10` | 无 |
| 自动刷新说明 | 信息区第 2 行 | `14/#8B98A8` | 无 |
| source 说明 | 信息区第 3 行 | `14/#8B98A8` | 无 |
| 底部 hint | `x=12,y=432,w=296,h=36` | `13/#8B98A8` | 无 |

无图状态：相机图区域保留同样尺寸，在中央显示 `Tap for new image`、`Camera offline` 或 `camera_detail`。

### 全局覆盖层参数

| 覆盖层 | 坐标/尺寸 | 字体/颜色 | 行为 |
|---|---|---|---|
| 亮度框 | 居中，`w>=104,h≈68,pad=10/18,radius=16` | `dosis_40,#F7FAFC`，背景黑 72% | 竖向拖动时显示，释放隐藏 |
| PIN 卡片 | 居中，`w=280,h=content,pad=18/22,radius=18` | 边框 `2 #F0A64B`，背景 `#071018` 约 94% | 长按请求 PIN 后显示 |
| PIN 标题 | 卡片内顶部 | `17/#F7FAFC,bold` | `WEB CONFIG PIN` |
| PIN 数字 | 标题下 `8 px` | `dosis_40,42,#F0A64B` | 六位最大显示 |
| PIN 说明 | 数字下 `10 px` | `15/#C8D1DC` | 剩余有效时间 |

## 页面 1：打印机选择页

用途：选择已配置打印机，并显示连接状态。

原 UI 内容：

- 标题 `Printers`。
- 垂直滚动打印机卡片列表。
- 空状态：没有配置打印机，引导使用 Web portal。
- 卡片字段：打印机名称、型号、host/IP、active 状态、连接状态。
- 点击卡片请求切换打印机。

ONX 320 x 480 布局草图：

```text
+--------------------------------+
| Printers                  Wi-Fi |
|                                |
| [active] Studio P1S            |
|          P1S  192.168.1.24     |
|          Online                |
|                                |
| [     ] A1 Mini                |
|          A1M  cloud/local      |
|          Waiting               |
|                                |
| ... vertical scroll ...        |
|                                |
| Hold: Web Config PIN           |
+--------------------------------+
```

功能要求：

- 卡片宽度必须适配 296 px 内容宽度。
- 每张卡片触摸目标建议不小于 296 x 68 px。
- 当前 active 打印机使用强调边框或左侧色条。
- 连接状态使用小色点加文字，不能只依赖颜色。
- 空状态保留原 Web portal 引导。

资源要求：

- 使用文字和 LVGL 基础图形即可，不需要新增打印机图标。

## 页面 2-5：AMS 单元页

用途：显示每个 AMS 单元的 4 个耗材槽；第一个 AMS 页可显示 EXT 外置料卷。

原 UI 内容：

- 最多 4 个 AMS 单元，每个单元一页。
- 多 AMS 时显示 `AMS 1..4` 标题。
- 4 个料槽：耗材颜色、材料类型、剩余百分比、active 箭头。
- 空槽状态。
- HMS/AMS 槽位错误覆盖层和红色脉冲提示。
- 湿度和 AMS 温度。
- 第一个 AMS 页额外显示 external spool。

ONX 320 x 480 常规 4 槽布局草图：

```text
+--------------------------------+
| AMS 1                    1/4   |
| Humidity 32%      Temp 28C     |
|                                |
| +----------+ +----------+      |
| | PLA      | | PETG     |      |
| | green    | | orange   |      |
| | 86%  >   | | 42%      |      |
| +----------+ +----------+      |
|                                |
| +----------+ +----------+      |
| | Empty    | | ASA      |      |
| | --       | | black    |      |
| |          | | 18% ERR  |      |
| +----------+ +----------+      |
|                                |
| EXT: PLA white  active         |
+--------------------------------+
```

功能要求：

- 原 4 个竖向 pill 横排超过 320 px，ONX 改为 2 x 2 料槽网格。
- 每个料槽视觉/触控区域建议约 136 x 96 px。
- 每个料槽显示材料类型、颜色色块/背景、剩余百分比、active 状态。
- 错误态使用红色边框加 `ERR` 或警告标记；原菱形覆盖动画第一版可降级。
- EXT 外置料卷只在 AMS 1 页出现，并仅在 `tray_now == 254` 或 `tray_tar == 254` 时显示。
- 无 AMS 时，页面 2 可显示 `No AMS connected`；页面 3-5 在导航中跳过。
- 只有一个 AMS 时，可隐藏 `AMS 1` 编号以节省空间；多个 AMS 时必须显示单元编号。

资源要求：

- 使用 LVGL 矩形、文字和颜色，不新增料卷/料槽 bitmap。
- 保留原 active 绿色、error 红色语义。

## 页面 6：主状态页

用途：主监控页，显示生命周期、进度、温度、层数、剩余时间、电源状态和 Web Config 提示。

原 UI 内容：

- 全屏圆形进度环。
- 进度百分比。
- 电池图标和电池百分比。
- 中央 Bambu logo；支持 chamber light toggle。
- 生命周期状态文本。
- 详情/错误文本。
- 层数。
- 喷嘴和热床温度，使用 MDI 图标。
- 可选辅助温度：第二喷嘴、chamber 等。
- 剩余时间行；点击切换剩余时长和 ETA。
- Web Config hint、IP、PIN 状态。

ONX 320 x 480 布局草图：

```text
+--------------------------------+
| 87% Printing              BAT  |
| Bambu_P1S_gearbox_cover        |
| Layer 122 / 350                |
|                                |
|        [ compact progress ]    |
|        87%                     |
|        Printing                |
|                                |
| Nozzle  220C      Bed   65C    |
| Aux     L:219C    Chamber 38C  |
|                                |
| Remaining  1h 24m              |
| Tap time row for ETA           |
|                                |
| Open 192.168.1.42 | Hold PIN   |
+--------------------------------+
```

功能要求：

- 原 466 x 466 全屏圆环不能直接迁移。第一版推荐使用横向进度条加大百分比；如果视觉确认需要，可评估 180-220 px 小圆弧。
- 生命周期状态和详情文本必须直接可见，不能依赖圆环中心区域。
- 任务名/详情在 296 px 内换行或滚动。
- 温度区域必须支持单喷嘴、双喷嘴、热床、chamber 等变体。
- 剩余时间行保留点击切换 ETA 功能，触控区域不小于 296 x 44 px。
- `bambuicon_small` 可缩小为 48-64 px，用作 chamber light 触控按钮；不再作为主布局中心锚点。
- 错误、离线、待配置状态下，优先显示状态和详情，进度退为次要信息。

资源要求：

- 百分比、温度、时间优先复用 Dosis 40/32/20。
- 时钟、喷嘴、热床图标优先复用 `mdi_40`；如果图标占宽过大，则改用文字标签。
- 电池优先复用 `mdi_30`。
- Bambu 标识复用 `bambuicon_small` 缩放显示。

## 页面 7：封面预览页

用途：显示云端封面图和打印任务标题/上下文。

原 UI 内容：

- 从 cloud cover PNG 解码得到封面图。
- 加载中、无封面、待配置、离线提示。
- 预览标题、任务名、cloud detail 或 hint。
- 有图时，提示隐藏，标题移动到图片下方。

ONX 320 x 480 布局草图：

```text
+--------------------------------+
| Cover                    7/8   |
|                                |
| +----------------------------+ |
| |                            | |
| |       296 x 296 image      | |
| |                            | |
| +----------------------------+ |
|                                |
| Bambu_P1S_gearbox_cover       |
| PLA matte black               |
| Loading cloud cover / detail  |
+--------------------------------+
```

功能要求：

- 图片最大尺寸建议 296 x 296，使用 contain 模式。
- 无图时保留图片区域，在区域中央显示提示，避免整页跳动。
- 长标题使用两行换行或 marquee。
- 图片下方所有文本限制在 296 px 宽度内。
- 不增加复杂装饰边框；简单边框/背景即可。

资源要求：

- 不需要静态封面资源；封面图来自运行时数据。

## 页面 8：相机页

用途：显示本地相机快照，并支持点击刷新。

原 UI 内容：

- RGB565 相机快照。
- 有图时，状态文本在图片上方。
- 层数/进度说明在图片下方。
- 无图时显示 empty/offline/detail 信息。
- 点击页面请求刷新图片。

ONX 320 x 480 布局草图：

```text
+--------------------------------+
| Camera                   8/8   |
| Printing                       |
| +----------------------------+ |
| |                            | |
| |       296 x 166 image      | |
| |                            | |
| +----------------------------+ |
|                                |
| Layer 122 / 350                |
| Tap to refresh                 |
| Auto-refresh while open        |
|                                |
| Camera source: local           |
+--------------------------------+
```

功能要求：

- 图片按宽度约束，建议 296 x 166 适配 16:9 附近快照；若源比例不同，可 contain 在 296 x 210 内。
- 图片区域整体作为刷新触控区域。
- 无图状态显示 `Tap for new image`、`Camera offline` 或 `camera_detail`。
- 层数/进度说明保留在图片下方。
- 相机页是内存风险区域，不应比当前实现缓存更多相机图片。

资源要求：

- 第一版不需要静态相机图标。文字和简单占位框足够。

## 全局覆盖层

### 亮度覆盖层

触发方式：竖向拖动。保留原行为。

```text
+--------------------------------+
|                                |
|                                |
|          +----------+          |
|          |   80%    |          |
|          +----------+          |
|                                |
+--------------------------------+
```

要求：

- 居中显示，高对比。
- 横向翻页时不能误触发亮度调节。
- 复用 Dosis 40。

### Web Config PIN 覆盖层

触发方式：长按屏幕。

```text
+--------------------------------+
|                                |
|      +------------------+      |
|      | WEB CONFIG PIN   |      |
|      |     123456       |      |
|      | Valid for 2m     |      |
|      +------------------+      |
|                                |
+--------------------------------+
```

要求：

- 保留原 PIN overlay 语义。
- 六位 PIN 是最大字号文本。
- 覆盖层最大宽度建议 280 px。

## 交互契约 / Interaction Contract

本节是后续 UI 开发线程必须遵守的交互边界。来源以 `main/src/ui.cpp` 和 `main/include/printsphere/ui.hpp` 的现有事件处理为准；没有现成处理函数的区域一律视为展示或 no-op，不能自行新增打印机控制协议。

当前交互验收以本文档为准。`docs/ui_design_preview.html` 已确认用于视觉方向；Interaction Map/Hotspot 说明层可后续补充，但不得阻塞本文档中的交互契约执行。

### 原项目交互来源

| 交互 | 原源码位置 | 现有语义 |
|---|---|---|
| 横向 pager | `Ui::handle_pager_event()`、`set_active_page()`、`nearest_enabled_page_for_scroll()` | 横向滚动，释放后吸附到最近的已启用页面 |
| 页面可用性 | `update_page_availability_locked()`、`page_enabled()`、`clamp_enabled_page()` | AMS/Preview/Camera 根据 snapshot 状态启用，禁用页隐藏并跳过 |
| 全屏触摸/亮度/PIN/相机 tap | `Ui::handle_screen_event()` | 统一处理首触唤醒、轴锁、亮度覆盖层、长按 PIN、相机页 tap 刷新 |
| 打印机卡片 | `printer_card_click_cb()`、`consume_printer_switch_request()` | 点击卡片只产生切换 profile 请求 |
| Bambu logo | `handle_logo_event()`、`consume_chamber_light_toggle_request()` | 点击 logo 只产生 chamber light toggle 请求 |
| 剩余时间行 | `remaining_row_event_cb()`、`handle_remaining_row_click()` | 点击在 remaining duration 和 ETA 之间切换 |
| Portal/PIN overlay | `set_portal_access_state()`、`compute_portal_texts_locked()`、`update_portal_access_visuals_locked()` | portal 状态由外部 access snapshot 驱动，PIN active 时显示覆盖层 |

### 全局手势优先级

| 优先级 | 条件 | 行为 | 禁用/消费规则 |
|---:|---|---|---|
| 1 | `screen_power_mode == kOff` 且收到 `LV_EVENT_PRESSED` | 首次触摸调用 `note_activity(true)` 唤醒屏幕 | 本次触摸必须被消费，不触发点击、翻页、PIN、亮度或相机刷新 |
| 2 | 已判定为垂直亮度拖动 | 锁定 pager，显示亮度 overlay，按垂直位移调节亮度 | 亮度 overlay 显示期间禁用长按 PIN、tap、相机刷新和页面切换 |
| 3 | 已判定为水平翻页 | 交给横向 pager 处理，释放后吸附到最近启用页 | 水平翻页期间不能触发亮度、PIN、tap、logo click、remaining click |
| 4 | 长按，且未显示 overlay、未 scrolling、未 horizontal swipe | 置位 `portal_unlock_requested_`，由应用层申请 PIN | 只发请求，不直接编造 PIN；overlay 内容必须来自 portal state |
| 5 | 小位移 tap | 根据当前被点区域执行点击语义 | 仅相机页全屏小位移 tap 会请求 camera refresh；普通页面背景 tap 无动作 |

手势判定参数必须沿用原语义，ONX 版可因触摸噪声做小幅校准，但需要主线程确认：

- 横向翻页阈值：`abs_dx >= 24 px`，且 `abs_dx >= abs_dy - 16 px`。
- 垂直亮度阈值：`abs_dy >= 24 px`，`abs_dx <= 18 px`，且 `abs_dy >= abs_dx + 16 px`。
- 亮度变化比例：垂直上滑为正，`250 px` 约等于 `100%` 亮度变化。
- 手动亮度最小值：原项目为 `4%`；`set_brightness_percent()` 内部仍 clamp 到 `0..100`，但手势调节不得低于 `4%`。
- 相机刷新 tap 阈值：释放时 `abs_dx < 12 px` 且 `abs_dy < 12 px`。
- 长按 PIN 只有在 gesture 仍 active、无 overlay、无 scrolling、无 horizontal swipe 时有效。

### 页面可用性与跳页

页面顺序固定为：`Printer Select -> AMS 1 -> AMS 2 -> AMS 3 -> AMS 4 -> Main -> Cover -> Camera`。

| 页面 | 是否总是启用 | 启用条件 | 禁用处理 |
|---|---|---|---|
| Printer Select | 是 | 永远启用 | 不允许隐藏 |
| AMS 1 | 否 | `snapshot.ams != nullptr && snapshot.ams->count >= 1` | pager 跳过；仅在需要解释空 AMS 时可显示占位 |
| AMS 2 | 否 | `snapshot.ams->count >= 2` | pager 跳过 |
| AMS 3 | 否 | `snapshot.ams->count >= 3` | pager 跳过 |
| AMS 4 | 否 | `snapshot.ams->count >= 4` | pager 跳过 |
| Main | 是 | 永远启用 | 不允许隐藏 |
| Cover | 否 | `snapshot.preview_page_available == true` | 隐藏页面、释放 preview 图片缓存、pager clamp 到最近启用页 |
| Camera | 否 | `snapshot.camera_page_available == true` | 隐藏页面、释放 camera 图片缓存、pager clamp 到最近启用页 |

开发线程不得把不可用页硬做成可交互页面。禁用页在导航中应被跳过；如果 active page 因状态变化被禁用，必须调用等效 `clamp_enabled_page()` 逻辑回到启用页。Demo 中的 AMS 空态画面只允许作为说明/占位，不应成为可操作页面。

### 页面逐项交互映射表

| UI 区域 | 触摸区域 | 原项目处理函数/行为 | 前置条件 | 不可用/离线行为 | Demo 第一版 |
|---|---|---|---|---|---|
| Printer card | 整张卡片，建议 `296 x >=76` | `printer_card_click_cb()` 设置 `pending_printer_switch_`；应用层 `consume_printer_switch_request()` 后切 profile | 页面 1 active；卡片来自 `update_printer_cards()` | 没有卡片时显示空状态；点击 active 卡可允许 no-op 或重新请求同 index，但应用层已有同 index 忽略逻辑 | 必须实现 |
| AMS tray | 单槽视觉区域，建议 `143 x 102` | 原项目清除 clickable，无点击回调 | AMS 单元 present | 第一版点击必须 no-op；只能展示材料、余量、active、error | 必须展示；点击 no-op |
| AMS EXT row | `x=12,y=320,w=296,h=44` | 原项目 EXT 仅展示，无点击回调 | AMS 1 且 `tray_now == 254 || tray_tar == 254` | 不满足时隐藏；点击 no-op | 必须展示；点击 no-op |
| Bambu logo | 主状态页 logo 圆形按钮，建议 `60 x 60`，触控可扩到 `68 x 68` | `handle_logo_event()` 设置 `chamber_light_toggle_requested_` | 非 scrolling、`show_logo_ == true`、`snapshot.chamber_light_supported == true` | 离线/未配置/不支持灯光时禁用点击并显示普通 logo 或隐藏；不得新增其他动作 | 必须实现 |
| Remaining/ETA row | 主状态页时间行，`296 x 48` | `handle_remaining_row_click()` 切换 `show_eta_` 并重渲染 snapshot | 非 scrolling | 无 remaining 时在 `--m` 和 `--:--` 间切换；SNTP 未同步时 ETA fallback 到 remaining | 必须实现 |
| Camera preview | 相机页图像区域；原项目实际是全屏小位移 tap，这里视觉上聚焦图像框 | `handle_screen_event()` 在 camera page 小位移 tap 设置 `camera_refresh_requested_`；应用层 `consume_camera_refresh_request()` 后 `camera_client_.request_refresh()` | active page 是 Camera，`camera_page_available == true`，位移 `<12 px` | Camera disabled 时页面不可达；无图/离线仍可请求刷新，但应用层可能不返回图 | 必须实现 |
| Cover image | 封面图区域 `296 x 296` | 原项目无点击回调 | Cover page enabled | 点击 no-op；不得新增打开详情/下载等协议 | 展示必须，点击 no-op |
| Long press PIN | 全屏 | `handle_screen_event()` 长按后置位 `portal_unlock_requested_`；应用层 `setup_portal_.request_unlock_pin()`；overlay 由 `set_portal_access_state()` 驱动 | 非 off 首触、非 overlay、非 scrolling、非 horizontal swipe | provisioning context 可没有 hint，但长按请求仍走原逻辑；不能本地伪造 PIN | 必须实现 |
| Brightness drag | 全屏垂直拖动 | `handle_screen_event()` 轴锁后 `set_pager_scroll_locked(true)`、显示 `brightness_overlay_`、更新背光 | 非 off 首触；达到垂直阈值 | 释放或 press lost 后隐藏 overlay；拖动期间不触发其他点击 | 必须实现 |
| Horizontal pager | 全屏横向拖动 | `handle_pager_event()`，scroll begin 设置 `scrolling_`，scroll end `nearest_enabled_page_for_scroll()` | 未被亮度拖动锁定 | 禁用页跳过；释放后不得停在半页 | 必须实现 |

### 展示、预留和 no-op 边界

- AMS tray、AMS EXT、封面图、普通指标卡片、状态文字、错误文字、topbar、bottom hint 均为展示区域。
- AMS tray 第一版不做选料、退料、加载、RFID、错误确认等任何动作；点击必须 no-op。
- 封面页不做图片放大、打开历史任务、重新下载等动作；点击必须 no-op。
- 相机页只有刷新请求，不新增云端/本地源切换按钮。
- 错误面板只显示已有 `snapshot.detail`、`hms_codes` 解析结果或待确认字段，不新增 resume/cancel/clear error 按钮。
- 设置/Web Config 不新增独立按钮页；沿用 hint + 长按 PIN 入口。若后续主线程决定加入设置页，需另行修改本规范。

### Overlay 行为和互斥关系

| Overlay/提示 | 显示条件 | 消失条件 | 优先级/互斥 |
|---|---|---|---|
| Brightness overlay | 垂直亮度拖动胜出后立即显示，内容为当前亮度百分比 | `LV_EVENT_RELEASED` 或 `LV_EVENT_PRESS_LOST` | 显示期间消费本次手势；禁止 PIN、tap、pager |
| PIN overlay | `portal_pin_active == true` | `portal_pin_active == false` 或 PIN 超时后由 portal state 清除 | 高于页面内容；不可点击；内容只来自 `set_portal_access_state()` |
| Portal hint | 主状态页 settled、`portal_hint_text_` 非空，且 PIN/session 有优先权或 detail 为空 | 离开主状态页、scrolling、hint 为空 | 与 `detail_label_` 互斥；PIN/session hint 优先于 detail |
| Error/detail 文本 | `detail_text(snapshot)` 非空；错误态使用滚动或醒目样式 | snapshot 状态变化后重算 | Portal hint 有优先权时隐藏 detail；brightness/PIN overlay 盖在其上 |

错误/portal/brightness 的显示优先级从高到低为：Brightness overlay、PIN overlay、Portal hint、Error/detail、普通页面内容。Brightness 是手势过程 overlay，不应改写 portal/PIN 状态。

### 数据绑定边界

| UI 元素 | 数据来源 | 备注 |
|---|---|---|
| 页面可用性 | `PrinterSnapshot.ams->count`、`preview_page_available`、`camera_page_available` | 必须驱动 pager 跳页 |
| 打印机卡片 | `Ui::PrinterCardInfo.index/name/model/host/active/connected`，由 `Application` 从 `ConfigStore` 和 local connected 状态生成 | 当前 connected 只表示 active printer local connected；更多状态待前端线程在代码中确认 |
| 生命周期文本 | `lifecycle_label(snapshot)`，基于 `connection`、`ui_status`、`lifecycle`、`has_error`、`wifi_connected` | 不直接显示未清洗 raw status，除非原 resolver 已赋值 |
| 状态详情/错误 | `detail_text(snapshot)`，主要来自 `detail`、`job_name`、setup AP/IP、Wi-Fi 状态 | HMS 摘要如何压缩为一行待前端线程在代码中确认 |
| 进度 | `progress_percent` clamp 到 `0..100` | download 相关可用 `progress_is_download_related` 做辅助文案，待确认 |
| 任务名 | `job_name`，fallback 可用 `gcode_file` 或 `preview_title` | 具体 fallback 顺序待前端线程在代码中确认 |
| 层数 | `current_layer`、`total_layers` | 没有 total 时显示 `Layer: x / --` |
| 剩余/ETA | `remaining_seconds`、本地时间 `time(nullptr)` | 未同步时 ETA fallback 到 remaining |
| 喷嘴/热床 | `nozzle_temp_c/nozzle_temp_known`、`bed_temp_c/bed_temp_known` | 未知显示 `--C` |
| 双喷嘴/辅助温度 | `active_nozzle_index`、`secondary_nozzle_temp_c/known` | H2D 等机型需保留 L/R 前缀 |
| Chamber 温度 | `chamber_temp_c/chamber_temp_known` | 无数据时隐藏 aux |
| 电池 | `battery_present`、`battery_percent`、`charging`、`usb_present` | 图标来自 `mdi_30` 子集 |
| Bambu logo 显隐 | `should_show_logo(snapshot)`；灯光点击由 `chamber_light_supported/state_known/on` 控制 | 灯光 off 时原项目将 logo recolor 灰色，ONX 应保留等效状态 |
| AMS 单元 | `snapshot.ams->units[u].present/humidity_pct/temperature_c/trays[]` | `present`/`count` 决定页面启用 |
| AMS tray | `AmsTrayInfo.present/active/material_type/material_name/color_rgba/remain_pct` | `material_name` 在原 AMS signature 未计入，显示前需确认刷新条件是否覆盖 |
| AMS EXT | `snapshot.ams->external_spool`，显示条件还依赖 `tray_now/tray_tar == 254` | 第一版只展示 |
| AMS error | `snapshot.hms_codes` 中 AMS module `0x07`、slot part `0x20..0x23` | unit-level AMS 错误如何显示待确认 |
| Cover 图 | `preview_blob` 解码后显示；文案来自 `preview_note_text()`、`preview_subnote_text()` | 不引入静态封面 |
| Camera 图 | `camera_blob`、`camera_width`、`camera_height`；文案来自 `camera_note_text()`、`camera_subnote_text()` | 只保留双 buffer 等效策略，不增加缓存 |
| Portal/PIN | `set_portal_access_state(lock_enabled, request_authorized, session_active, pin_active, pin_code, pin_remaining_s, session_remaining_s)` | PIN 码和有效期绝不由 UI 生成 |
| Wi-Fi/Web hint | `wifi_connected`、`wifi_ip`、`setup_ap_active/ssid/password/ip`、portal state | 独立设置页暂缓 |

### 交互验收用例

后续 UI 开发线程实现后，必须至少按以下用例自测并回报结果：

1. 屏幕 off 状态下第一次触摸只唤醒屏幕，不切页、不点 logo、不刷新相机、不弹 PIN。
2. 横向拖动超过 24 px 后释放，pager 吸附到最近已启用页；如果中间 AMS/cover/camera 禁用，必须跳过。
3. 垂直拖动超过 24 px 且横向位移不超过 18 px 时显示亮度百分比；释放后 overlay 消失，页面不发生横向偏移。
4. 对角线拖动若先满足横向条件，应进入 pager；不得同时改变亮度。
5. 长按主屏或其他页面空白处，在无 scrolling/overlay/swipe 时只触发 portal unlock request；PIN overlay 内容由 portal state 显示。
6. 长按过程中如果已触发亮度 overlay 或横向 swipe，不得再触发 PIN。
7. 主状态页点击 Bambu logo：仅当 `chamber_light_supported` 且 logo 可见时产生 light toggle request；离线/不支持时 no-op。
8. 主状态页点击 remaining/ETA 行：在 remaining duration 与 ETA 间切换；scrolling 期间点击无效。
9. 相机页小位移 tap：产生 camera refresh request；拖动、横滑或离开相机页不产生请求。
10. 封面图、AMS tray、AMS EXT、指标卡片点击均 no-op，不产生任何打印机控制请求。
11. `preview_page_available=false` 时 Cover 页不可达，并释放/隐藏 preview 图像；`camera_page_available=false` 时 Camera 页不可达，并释放/隐藏 camera 图像。
12. `snapshot.ams->count` 从 4 变 1 时，AMS 2-4 立即从 pager 中移除；若当前在被移除页，自动 clamp 到启用页。
13. Portal PIN active 时 PIN overlay 高于页面内容；portal hint 与 detail 互斥，PIN/session hint 优先。
14. 错误态显示错误/detail 信息，但不新增 resume/cancel/clear 控制按钮。

## Demo-first 第一版范围

必须实现：

- 8 个逻辑页面结构，页面尺寸为 320 x 480。
- 打印机选择页。
- 主状态页：进度、生命周期、任务名/详情、温度、层数、剩余时间、电池、Web Config hint。
- 封面页：有图/无图/加载/离线状态。
- 相机页：有图/无图/点击刷新状态。
- AMS 1 页：2 x 2 料槽网格、active/error/empty 状态、EXT 行。
- 长按 PIN 覆盖层。
- 亮度覆盖层。
- 上述 Interaction Contract 中标为“必须实现”的交互和 no-op 边界。

可以后续实现：

- 如果 demo 硬件只暴露一个 AMS，AMS 2-4 可后续补全。
- AMS 菱形错误覆盖动画。
- 如果用户确认需要，再实现小圆弧进度样式。
- 扩展更多 MDI 图标。
- 更完整的页面指示器和转场动画。

## 待确认问题

- 主状态页第一版采用横向进度条，还是保留小圆弧进度？
- 不可用 AMS 页面是完全跳过，还是在 demo 中显示空占位页？
- Web Config/设置是否未来需要独立设置页，还是保持原逻辑：打印机选择页提示 + 全局长按 PIN 覆盖层？
- 是否允许后续扩展 MDI 字体 glyph 范围，还是第一版严格只使用当前已编译的图标子集？
