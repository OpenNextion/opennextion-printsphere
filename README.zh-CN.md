# OpenNextion-printsphere

[![English](https://img.shields.io/badge/lang-English-blue)](./README.md)
[![中文](https://img.shields.io/badge/lang-中文-red)](./README.zh-CN.md)

OpenNextion-printsphere 是一个将 [PrintSphere](https://github.com/cptkirki/PrintSphere) 移植到 OpenNextion ESP32 矩形屏幕上的桌面显示屏项目，用于显示 Bambu Lab（拓竹）打印机的打印状态、文件信息、AMS 信息、封面图和摄像头画面等内容。

首个完成适配和验证的屏幕型号是 [ONX3248G035][onx3248g035]。

## 项目背景

我想给自己的 Bambu Lab（拓竹）打印机做一个桌面显示屏，用来在桌面上直接查看当前打印状态。GitHub 上已经有一个很棒的项目 [PrintSphere](https://github.com/cptkirki/PrintSphere)，不过原项目主要适配的是圆形屏幕。

我手上没有对应的圆形屏幕，但有几款矩形的 OpenNextion ESP32 屏幕，所以决定把 PrintSphere 移植到这些矩形屏幕上。当前第一个完成适配的是 [ONX3248G035][onx3248g035]，它可以横向摆放在桌面上，用来显示打印机状态、AMS 信息、封面图和摄像头画面。

我选择 OpenNextion 屏幕，一方面是因为它使用 ESP32 作为主控，带有比较丰富的外设接口，后续也方便继续 DIY 其他项目；另一方面是因为 OpenNextion 提供了比较完整的开源资料和硬件资料，便于二次开发和结构适配。

以 [ONX3248G035][onx3248g035] 为例，官方资料中包含示例代码、原理图、板级资料，以及 2D DWG 和 3D STP 文件。对这个项目来说，作为打印机桌面显示屏暂时不需要额外硬件，只需要给屏幕做一个桌面横屏底座即可。基于官方提供的 3D 文件，我很快完成了一个简易桌面支架，用来把 [ONX3248G035][onx3248g035] 放在桌面上使用。

## 当前移植内容

这个版本基于 PrintSphere 进行 OpenNextion 适配，主要改动包括：

### 1. 新增 [ONX3248G035][onx3248g035] 板级支持

新增了 [ONX3248G035][onx3248g035] 的 BSP 适配，将显示、触摸、背光和 LVGL 初始化收敛到独立板级组件中。公开 `v0.1.0` 源码构建目标是 [ONX3248G035][onx3248g035] 横屏配置。

横屏 UI 已经完成主要适配和实机验证。其他屏幕型号或显示方向不作为公开 `v0.1.0` 构建目标。

### 2. 重做矩形屏幕横屏 UI

原项目主要面向圆形屏幕。本项目针对 [ONX3248G035][onx3248g035] 的矩形屏幕重新整理了横屏 UI，使主状态页、AMS 信息、封面图和摄像头页面更适合桌面横向摆放查看。

横屏版本重点调整了页面信息密度、文本区域、状态信息排布和 AMS 托盘显示，避免圆形屏幕布局直接迁移到矩形屏幕后出现空间浪费或内容溢出。

### 3. 兼容 CN 区 Bambu Cloud 证书链路

针对 CN 区账号和云端资源访问，增加了 `api.bambulab.cn`、`bambulab.cn` 和 `cn.mqtt.bambulab.com` 等 CN 区端点处理，并为 CN 区云端 HTTP、预览图下载和相关 TLS 连接使用匹配的 `GlobalSign Root R3` CA 证书。

这项修改用于兼容 CN 区 Bambu Cloud 登录、设备列表获取、封面图下载和云端资源访问中可能遇到的证书链路问题。非 CN 区路径仍使用 ESP-IDF 默认证书包。

### 4. 新增 CJK 中文字库

新增 `onx_cjk_16` LVGL 字库，基于 Source Han Sans SC 生成，包含 ASCII 和约 2500 个现代汉语常用字。这个字库用于改善中文文件名、中文项目名和提示文字在设备端显示异常的问题。

## 当前验证状态

### 屏幕验证

- [ONX3248G035][onx3248g035] 可以运行 OpenNextion-printsphere
- [ONX3248G035][onx3248g035] 横屏配置已完成主要实机验证
- 横屏 UI 可以正常显示主要打印状态
- 中文文件名显示已通过 CJK 字库改善
- 其他屏幕型号或显示方向不包含在公开 `v0.1.0` 构建目标中
- OTA 固件升级功能尚未完成实机验证，首版建议优先使用完整固件烧录

### 我手上有的打印机和验证情况

图例：✅ 已验证 / ⚠️ 部分验证或环境未覆盖 / ❌ 验证不通过 / ⏳ 未验证

| 打印机 | 连接模式 | 连接/绑定 | 状态信息 | AMS 信息 | 封面图 | 摄像头 | 备注 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Bambu Lab A1 mini | 本地模式 | ✅ 已验证 | ✅ 已验证 | ⚠️ 环境未覆盖 | ❌ 不可用 | ✅ 已验证 | 当前测试环境未接入 AMS；本地模式暂时没有封面图页面 |
| Bambu Lab A1 mini | 混合模式 | ⏳ 未验证 | ⏳ 未验证 | ⏳ 未验证 | ⏳ 未验证 | ⏳ 未验证 | 后续需要单独验证 |
| Bambu Lab H2C | 本地模式 | ⏳ 未验证 | ⏳ 未验证 | ⏳ 未验证 | ⏳ 未验证 | ⏳ 未验证 | 后续需要单独验证 |
| Bambu Lab H2C | 混合/云端模式 | ✅ 已验证 | ✅ 已验证 | ✅ 已验证 | ✅ 已验证 | ❌ 不可用 | 仅测试了 CN 区账号 |
| Bambu Lab P1S | 本地模式 | ⏳ 未验证 | ⏳ 未验证 | ⏳ 未验证 | ⏳ 未验证 | ⏳ 未验证 | 后续需要验证登录、连接、状态显示和摄像头表现 |
| Bambu Lab P1S | 混合模式 | ⏳ 未验证 | ⏳ 未验证 | ⏳ 未验证 | ⏳ 未验证 | ⏳ 未验证 | 后续需要验证登录、连接、状态显示和摄像头表现 |

## 支持的硬件

| 型号 | 尺寸 | 状态 | 说明 |
| --- | --- | --- | --- |
| [ONX3248G035][onx3248g035] | 3.5 inch | 已验证 | 首个适配型号，推荐使用横屏配置 |
| [ONX2432G028][onx2432g028] | 2.8 inch | 计划中 | 后续计划适配，体积更小，价格更低 |

## 固件下载与烧录

首版计划版本号为 `v0.1.0`。正式发布后，可以在 GitHub Release 页面下载首版已发布板型和方向的完整烧录固件。

| 屏幕型号 | 显示方向 | 固件文件 | 版本 | 状态 |
| --- | --- | --- | --- | --- |
| [ONX3248G035][onx3248g035] | 横屏 | `opennextion-printsphere-onx3248g035-landscape-full-v0.1.0.bin` | `v0.1.0` | 待上传至 GitHub Release |

首版建议使用 `full` 固件进行完整烧录。OTA 升级流程尚未完成实机验证，因此首版暂不提供 OTA 固件下载入口。

[ONX3248G035][onx3248g035] 竖屏固件和 [ONX2432G028][onx2432g028] 固件需要单独的公开构建配置和发布验证，因此不是 `v0.1.0` release asset。

## 下一步计划

后续计划包括：

- 计划适配 [ONX2432G028][onx2432g028] 的横屏和竖屏支持
- 尝试解决 Bambu Lab H2C 混合/云端模式下摄像头页面不可用的问题
- 验证 Bambu Lab A1 mini 混合模式下的连接、状态、AMS、封面图和摄像头表现
- 验证 Bambu Lab H2C 本地模式下的连接、状态、AMS、封面图和摄像头表现
- 验证 Bambu Lab P1S 在本地模式和混合模式下的登录、连接、状态显示、AMS、封面图和摄像头表现

## 致谢

本项目基于 PrintSphere 移植而来，感谢原作者和相关开源项目的工作。

- PrintSphere: https://github.com/cptkirki/PrintSphere
- OpenNextion open source projects: https://github.com/OpenNextion

## License

OpenNextion-printsphere 是 [PrintSphere](https://github.com/cptkirki/PrintSphere) 的非商业派生版本，保留原项目的 copyright 和根许可证约束。

本项目整体遵循 `Federation Non-Commercial License (FNCL) v1.1`。你可以在非商业目的下使用、复制、修改和分享本项目；任何商业用途都需要先获得原 copyright holder 的单独书面商业授权。

GitHub Release 中发布的固件文件同样受 FNCL v1.1 的非商业限制约束。第三方组件或字体可能带有各自的许可证说明，详见 `NOTICE.md`。

## 免责声明

本项目不是 Bambu Lab 官方项目，不是 OpenNextion 官方项目，也不是原版 PrintSphere 官方项目。

刷写和使用第三方固件存在风险。请在理解相关风险后自行决定是否使用。本项目不对设备损坏、数据丢失、打印中断、网络连接异常或其他使用后果承担责任。

[onx3248g035]: https://nextion.tech/wiki/onx3248g035/
[onx2432g028]: https://nextion.tech/wiki/onx2432g028/
