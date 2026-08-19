# Zeus Tools 当前开发计划

> 状态：v0.2.0 Alpha 已发布，继续跨平台稳定化
>
> 更新日期：2026-08-19
>
> 产品基线：[产品需求](./product-requirements.md)
>
> 当前架构：[ADR-001](./adr/001-ui-framework.md)
>
> 发布门禁：[发布检查清单](./release-checklist.md)

## 1. 已完成

### 核心处理

- JSON、XML、YAML、TOML、INI/Properties、CSV、JWT、常见编码和纯文本检测。
- JSON/XML/YAML/TOML/INI 格式化、高亮、错误信息及结构折叠。
- JSON ↔ YAML/XML/TOML、TOML/INI → JSON、JSON → CSV，以及 JSON Escape/Unescape。
- HTML Entity、Hex、Unicode Escape 编解码和仅对精确 10/13 位输入提供的 Unix 时间转换。
- Base64、Base64 Data URL 和 URL 自动解码一层；Base64 二进制结果显示字节数与 Hex 预览，并可将
  原始解码字节显式导出；常见图片、文档、压缩包、音频和 SQLite 数据库根据魔数
  建议扩展名，未知格式回退为 `.bin`；Data URL 解码且魔数确认为 PNG/JPEG 时，
  解析并显示图片尺寸；仅在尺寸安全且尺寸头可解析时直接以内存图片预览，不创建临时文件。
- CRC32、MD5、SHA-1、SHA-256、SHA-384、SHA-512 与 HMAC 计算，支持 Hex/Base64 输出；
  CRC32 仅作为非密码学校验和提供，不进入 HMAC。
- HMAC 可选择原始输入/当前结果以及 UTF-8/Hex/Base64 密钥；密钥默认遮罩，
  关闭 HMAC 或摘要面板时主动清理输入状态与历史快照。
- 嵌套 Base64、URL 和转义 JSON 可由用户每次继续解码一层，并显示处理链；
  原始输入保持不变。
- XML DTD/ENTITY 拒绝和 YAML 单文档安全子集限制。
- 建立版本化自动检测语料，覆盖合法、非法、冲突和纯文本误判样例；普通
  JSON 数组不会再误判为转义 JSON。

### 桌面界面

- 上方可编辑输入、下方只读 `RichTextView` 和中间上下文操作带。
- JSON/XML/YAML/TOML/INI 使用方形 `-`/`+` 控件折叠；搜索跳转可展开隐藏命中。
- 结果按字符跨行选择、双击选词、三击选行、全选和复制。
- 文本与 CSV 精确范围搜索，支持大小写、正则、匹配计数和上下跳转。
- CSV 虚拟行列、分隔符/表头切换、单元格定位与 TSV 复制。
- System/Light/Dark 主题和十种界面语言；主题与语言偏好本地保存。
- macOS 苹方/SF Mono 与 Windows 微软雅黑/Cascadia Mono 字体回退。
- `Cmd/Ctrl+F` 聚焦底部搜索框，`Cmd/Ctrl+O` 打开文件，`Cmd/Ctrl+S` 导出结果；
  `Esc` 关闭最上层弹窗、下拉或摘要/HMAC 面板。
- 用户主动打开或拖入单个 UTF-8 文件，并可导出当前结果；不保存文件路径、
  最近文件或内容历史。
- 离线命令行管道支持 stdin/单文件输入、stdout 输出、手动类型和稳定操作 ID；
  与桌面应用共用处理注册表和 10 MiB 边界。

### 工程与发布资源

- C++17 核心测试和 1 MB/10 MB JSON、CSV 基准程序；10 MiB 应用处理服务基准覆盖
  检测、格式化、展示模型构建和搜索，当前 Debug 峰值常驻内存约 269 MiB，较优化前
  下降约 10%。
- 提供 Clang/GCC AddressSanitizer + UndefinedBehaviorSanitizer 核心测试预设，当前
  四组核心、检测、处理注册表与 CLI 回归均通过；项目仍按约定不使用 GitHub CI。
- 自动检测报告覆盖 57 个版本化样例；当前精确匹配率、可识别内容召回率和纯文本
  特异度均为 100%，并设置语料数量下限防止删减难例。
- EUI-NEO、pugixml、yaml-cpp、toml++ 均固定到不可变提交。
- macOS App/DMG 和 Windows Portable ZIP 配置；不生成 NSIS/MSI。
- 应用图标、README 截图、隐私/安全/贡献文档和 GitHub Issue/PR 模板。
- 项目许可证、第三方声明及 13 份完整第三方许可证随包安装。
- macOS arm64 DMG 已通过 SHA-256、磁盘映像及包内文件检查。
- Windows x64 Portable ZIP 已通过 MinGW-w64 交叉构建、压缩包完整性、
  PE/VersionInfo、运行时资源和系统 DLL 依赖检查。
- Windows 11 已完成 MinGW-w64/MSVC 原生 Debug/Release 构建、CTest、Portable
  ZIP、CLI/GUI 启动、中文 IME 预编辑与候选提交、emoji/组合字符和 10 MiB
  JSON/CSV 验证；线上 `v0.2.0` Windows 包也已重新下载并通过校验。
- `v0.1.0` 标签、双平台产物、SHA-256 文件及双语发布说明已发布到
  GitHub Releases。
- `v0.2.0` 加入 TOML/INI、Unicode/HTML/Hex、文件工作流、离线 CLI、
  大文本分页编辑、处理器全量执行测试和更严格的双平台包校验。

## 2. 当前未完成

| 优先级 | 工作 | 完成标准 |
| --- | --- | --- |
| P0 | Windows 10 原生运行验证 | Windows 11 已通过；仍需在 Windows 10 验证 Portable ZIP 启动、图标、VersionInfo、字体和核心交互。 |
| P0 | 剩余输入与 IME 验证 | Windows 11 中文 IME、emoji 与组合字符已通过；仍需完成 macOS 与 Windows 10 的剪贴板、选择、撤销和 IME 验收。 |
| P0 | 10 MB UI 端到端验收 | 后台基准、超限保护、macOS/Windows 11 精确 10 MiB JSON/CSV、macOS RSS/整机峰值和 16 KiB 分页编辑已完成；仍需跨页连续选择和 Windows 10 复验。 |
| P1 | 键盘与可访问性 | 已完成搜索聚焦和 Esc 关闭；已确认当前 EUI 自绘控件未暴露到 macOS AX 树，需先补框架辅助功能桥接，再完成全键盘导航、焦点顺序和屏幕阅读器检查。 |
| P1 | 检测质量外部验证 | 仓库内 57 例基线与报告已完成；仍需收集经授权脱敏、未用于规则开发的真实样例，输出独立准确率与高置信度误判率。 |
| P1 | 签名与公证 | v0.2.0 已明确采用未签名 Alpha 包；后续版本评估 macOS notarization 与 Windows Authenticode。 |

## 3. 当前里程碑

| 里程碑 | 状态 | 退出条件 |
| --- | --- | --- |
| 0.1 Alpha 功能闭环 | 已完成 | 核心格式、编码、CSV、搜索、选择、主题、语言和 crypto 可用。 |
| v0.1.0 公开 Alpha | 已发布 | `v0.1.0` 标签和 GitHub Release 包含 macOS arm64 DMG、Windows x64 Portable ZIP、校验值与发布说明。 |
| v0.2.0 扩展 Alpha | 已发布 | 新格式、编码、文件工作流、CLI、大文本保护和异常隔离进入双平台候选包。 |
| 0.2.x 稳定化 | 进行中 | 完成 Windows 10、macOS/Windows 10 IME、跨页选择及键盘可访问性验收。 |

## 4. 后续版本发布门禁

- `./scripts/check-repository.sh` 通过。
- `cmake --build --preset core` 与 `ctest --preset core` 通过。
- macOS 和 Windows 分别在原生环境完成 Release 构建。
- JSON/XML/YAML/TOML/INI/CSV/JWT、编码工具、文件工作流、搜索、选择、折叠、复制和 HMAC 完成人工冒烟。
- 10 MB JSON/CSV 不崩溃、不显示过期后台任务结果，并记录峰值内存。
- 发布包不包含用户内容、开发索引、构建缓存、旧图标或 EUI 示例资源。
- 发布包包含项目文档与 `licenses/` 完整许可证目录。

## 5. 当前明确不做

- GitHub CI；构建、测试和发布按文档手工执行。
- NSIS/MSI 安装包；Windows 仅提供 Portable ZIP。
- 服务端、账号、遥测、云同步、内容历史和剪贴板监听。
- 可编辑结果、多标签页、项目空间、SQL、JSON Tree、Diff、Linux 和 Web 版。
- 无限递归自动解码、JWT 签名验证或摘要“解密”。
