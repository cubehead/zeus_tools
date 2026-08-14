# Zeus Tools 当前开发计划

> 状态：0.1 Alpha 发布收尾
>
> 更新日期：2026-08-14
>
> 产品基线：[产品需求](./product-requirements.md)
>
> 当前架构：[ADR-001](./adr/001-ui-framework.md)
>
> 发布门禁：[发布检查清单](./release-checklist.md)

## 1. 已完成

### 核心处理

- JSON、XML、YAML、CSV、JWT、Base64、URL Encode 和纯文本检测。
- JSON/XML/YAML 格式化、高亮、错误信息及结构折叠。
- JSON ↔ YAML、JSON ↔ XML、JSON → CSV，以及 JSON Escape/Unescape。
- Base64 和 URL 自动解码一层；Base64 二进制结果显示字节数与 Hex 预览。
- MD5、SHA-1、SHA-256、SHA-512 与 HMAC 计算，支持 Hex/Base64 输出。
- XML DTD/ENTITY 拒绝和 YAML 单文档安全子集限制。

### 桌面界面

- 上方可编辑输入、下方只读 `RichTextView` 和中间上下文操作带。
- JSON/XML/YAML 使用方形 `-`/`+` 控件折叠；搜索跳转可展开隐藏命中。
- 结果按字符跨行选择、双击选词、三击选行、全选和复制。
- 文本与 CSV 精确范围搜索，支持大小写、正则、匹配计数和上下跳转。
- CSV 虚拟行列、分隔符/表头切换、单元格定位与 TSV 复制。
- System/Light/Dark 主题和十种界面语言；主题与语言偏好本地保存。
- macOS 苹方/SF Mono 与 Windows 微软雅黑/Cascadia Mono 字体回退。

### 工程与发布资源

- C++17 核心测试和 1 MB/10 MB JSON、CSV 基准程序。
- EUI-NEO、pugixml、yaml-cpp 均固定到不可变提交。
- macOS App/DMG 和 Windows Portable ZIP 配置；不生成 NSIS/MSI。
- 应用图标、README 截图、隐私/安全/贡献文档和 GitHub Issue/PR 模板。
- 项目许可证、第三方声明及 11 份完整第三方许可证随包安装。
- macOS arm64 DMG 已通过 SHA-256、磁盘映像及包内文件检查。

## 2. 当前未完成

| 优先级 | 工作 | 完成标准 |
| --- | --- | --- |
| P0 | Windows 10/11 原生构建与运行 | Debug/Release 构建、Portable ZIP、图标、VersionInfo、字体和核心交互通过。 |
| P0 | 双平台输入与 IME 验证 | 中文 IME、emoji、组合字符、剪贴板、选择和撤销无阻断问题。 |
| P0 | 10 MB UI 端到端验收 | 记录粘贴到首屏、滚动、搜索、清空、内存峰值和稳定性数据。 |
| P0 | HMAC 面板补全 | 增加消息来源、密钥编码、密钥遮罩；关闭面板继续主动清理密钥。 |
| P0 | 手动继续解码 | 自动仍限制一层；为当前结果提供明确的继续解码入口和处理链提示。 |
| P1 | 键盘与可访问性 | 补齐搜索聚焦快捷键并完成键盘导航、焦点和屏幕阅读器检查。 |
| P1 | 检测质量数据集 | 建立可版本化的合法、非法、冲突和误判样例，记录准确率。 |
| P1 | 签名与公证策略 | 明确 macOS notarization 与 Windows 代码签名是否进入首个公开版本。 |

## 3. 当前里程碑

| 里程碑 | 状态 | 退出条件 |
| --- | --- | --- |
| 0.1 Alpha 功能闭环 | 已完成 | 核心格式、编码、CSV、搜索、选择、主题、语言和 crypto 可用。 |
| 0.1 Release Candidate | 进行中 | P0 项全部完成，双平台人工冒烟与发布清单通过。 |
| 0.1 公开发布 | 未开始 | 创建版本提交和标签，手工上传双平台产物、校验值与发布说明。 |

## 4. 发布前验证

- `./scripts/check-repository.sh` 通过。
- `cmake --build --preset core` 与 `ctest --preset core` 通过。
- macOS 和 Windows 分别在原生环境完成 Release 构建。
- JSON/XML/YAML/CSV/JWT/Base64/URL、搜索、选择、折叠、复制和 HMAC 完成人工冒烟。
- 10 MB JSON/CSV 不崩溃、不显示过期后台任务结果，并记录峰值内存。
- 发布包不包含用户内容、开发索引、构建缓存、旧图标或 EUI 示例资源。
- 发布包包含项目文档与 `licenses/` 完整许可证目录。

## 5. 当前明确不做

- GitHub CI；构建、测试和发布按文档手工执行。
- NSIS/MSI 安装包；Windows 仅提供 Portable ZIP。
- 服务端、账号、遥测、云同步、内容历史和剪贴板监听。
- 可编辑结果、多标签页、项目空间、SQL、JSON Tree、Diff、Linux 和 Web 版。
- 无限递归自动解码、JWT 签名验证或摘要“解密”。
