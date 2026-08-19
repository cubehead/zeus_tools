# Zeus Tools

<p align="center">
  <img src="resources/icons/zeus-tools-1024.png" width="128" alt="Zeus Tools 图标">
</p>

<p align="center">
  <a href="README.md">English</a> | <strong>简体中文</strong>
</p>

<p align="center">
  <a href="https://github.com/cubehead/zeus_tools/releases/latest"><img alt="最新版本" src="https://img.shields.io/github/v/release/cubehead/zeus_tools?display_name=tag&amp;sort=semver"></a>
  <a href="https://github.com/cubehead/zeus_tools/releases"><img alt="发布下载量" src="https://img.shields.io/github/downloads/cubehead/zeus_tools/total"></a>
  <a href="https://github.com/cubehead/zeus_tools/stargazers"><img alt="GitHub Stars" src="https://img.shields.io/github/stars/cubehead/zeus_tools?logo=github"></a>
  <a href="LICENSE"><img alt="MIT 许可证" src="https://img.shields.io/github/license/cubehead/zeus_tools"></a>
  <img alt="支持 macOS 和 Windows" src="https://img.shields.io/badge/platform-macOS%20%7C%20Windows-4b74d8">
  <img alt="C++17" src="https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&amp;logoColor=white">
</p>

<p align="center">
  <a href="https://github.com/cubehead/zeus_tools/releases/latest">下载</a> ·
  <a href="#功能亮点">功能亮点</a> ·
  <a href="#截图">截图</a> ·
  <a href="#开源组件">开源组件</a> ·
  <a href="#构建前置条件">构建</a> ·
  <a href="docs/releases/v0.2.0.md">发布说明</a>
</p>

Zeus Tools 是一款离线、跨平台的开发者工具箱，用于格式化、检查、转换、
编码和搜索结构化文本。项目使用 C++17 和
[EUI-NEO](https://github.com/sudoevolve/EUI-NEO) 开发，支持 macOS 和
Windows。

> 项目状态：**Alpha**。现已提供可供测试的发布包。Windows x64 版本最初由交叉
> 编译生成；Windows 11 原生 MinGW/MSVC 构建、启动与 IME 已通过，Windows 10 仍待验证。

## 下载

请从 [GitHub Releases](https://github.com/cubehead/zeus_tools/releases/latest)
下载最新的未签名 macOS arm64 DMG 或 Windows x64 便携版 ZIP。Windows 版本
无需安装，且不提供 NSIS/MSI 安装包。

## 功能亮点

- 自动识别 JSON、XML、YAML、TOML、INI/Properties、CSV、JWT 和常见编码文本。
- 格式化 JSON、XML、YAML、TOML 和 INI/Properties，并以只读方式显示语法高亮。
- 支持折叠嵌套的 JSON 容器、XML 元素和按缩进划分的 YAML 块。
- 支持 JSON ↔ YAML/XML/TOML、TOML/INI → JSON 和 JSON → CSV 转换。
- 支持 JSON 与 Unicode 转义/反转义；Base64 或 URL 编码每次只解码一层，
  并可通过明确的继续操作手动处理嵌套内容。
- 对格式与尺寸校验通过的 PNG/JPEG Base64 Data URL 直接进行内存图片预览，
  同时显示图片宽高，并保留可搜索摘要和原始字节导出。
- 使用虚拟化表格显示 CSV，支持分隔符和表头设置。
- 可搜索文本和 CSV 单元格，支持区分大小写和正则表达式模式。
- 可拖动水平分隔条调整输入区和结果区，且保留合理的最小显示比例。
- 1–10 MiB 输入使用 UTF-8 安全的分页编辑，并提供完整输入复制；处理始终基于完整原文。
- 在本地计算 CRC32、MD5、SHA-1、SHA-256、SHA-384、SHA-512 和 HMAC。
- 支持跟随系统、亮色和暗色主题，以及十种界面语言。
- 支持打开或拖入 UTF-8 文件并导出当前结果，不保存最近文件或内容历史。
- 可通过离线 stdin/stdout 命令行工具使用同一组已注册处理能力。
- 不上传也不保存正在处理的文本。

## 截图

### 结构化文本工作流

![Zeus Tools 在暗色主题中格式化 JSON](docs/assets/overview-dark.jpg)

<table>
  <tr>
    <td width="50%"><img src="docs/assets/csv-table.jpg" alt="带分隔符和表头设置的虚拟 CSV 表格"></td>
    <td width="50%"><img src="docs/assets/digest-hmac.jpg" alt="摘要与 HMAC 面板"></td>
  </tr>
  <tr>
    <td align="center">虚拟 CSV 预览与搜索</td>
    <td align="center">摘要与 HMAC 工具</td>
  </tr>
</table>

### JWT 查看

![Zeus Tools 解码 JWT Header 与 Payload](docs/assets/jwt-inspect.jpg)

截图使用合成数据，展示的是当前 macOS Alpha 界面。Windows 上的字体渲染
可能有所不同。

## 主要功能

| 输入或工具 | 自动/默认行为 | 其他操作 |
| --- | --- | --- |
| JSON | 美化格式、语法高亮和结构折叠 | 压缩、转义、转换为 YAML/XML/TOML/CSV |
| 已转义 JSON | 识别 JSON 字符串或已转义对象 | 只反转义一层 |
| XML | 严格解析、安全格式化、高亮和元素折叠 | 转换为 JSON；拒绝 DTD/实体 |
| YAML | 使用保守的安全子集解析、格式化和缩进折叠 | 转换为 JSON |
| TOML | 严格解析、格式化、高亮和章节折叠 | 与 JSON 双向转换 |
| INI/Properties | 保守解析赋值并格式化 | 转为 JSON，不擅自推断值类型 |
| CSV | 识别逗号、Tab、分号或竖线 | 手动设置分隔符/表头、虚拟表格、复制 TSV |
| Base64 | 高置信度时解码一层 | 支持标准、URL 安全及 Data URL；PNG/JPEG 内存预览，二进制按类型导出原始文件 |
| URL 编码 | 高置信度时解码一层 | 编码任意 UTF-8 文本 |
| HTML Entity / Hex | 只自动解码具有明确特征的内容 | 编码任意 UTF-8 文本 |
| Unicode Escape | 每次解码一层合法的 `\uXXXX` 序列 | 转义 UTF-8，并正确生成 emoji 代理对 |
| Unix 时间 | 对精确的 10/13 位数字提供候选操作 | 显示秒、毫秒、UTC 和本地时间 |
| JWT | 解码并格式化 Header/Payload | 搜索/复制声明；不验证签名 |
| 文本 | 只读结果和搜索 | 大小写转换、Base64/URL 编码 |
| 摘要/HMAC | 手动在本地计算 | 输入/结果来源、UTF-8/Hex/Base64 密钥、密钥遮罩、Hex/Base64 输出 |

可以随时通过输入类型菜单覆盖自动识别结果，且不会修改源文本。

## 隐私

所有格式化、解码、搜索和哈希计算均在本地进程中完成。Zeus Tools 不提供
账户、云服务、遥测或内容历史记录，只会持久化主题和语言偏好。详见
[PRIVACY.md](PRIVACY.md)。

## 命令行管道

发布包包含 `zeus-tools-cli`（Windows 为 `zeus-tools-cli.exe`）。它从
一个 UTF-8 文件或 stdin 读取内容，仅将处理结果写入 stdout，诊断信息写入
stderr，并使用与桌面应用相同的 10 MiB 输入上限。

```sh
echo '{"name":"Zeus"}' | zeus-tools-cli
zeus-tools-cli --input json --action json.minify data.json
zeus-tools-cli --list-inputs
zeus-tools-cli --list-actions
```

完整选项请运行 `zeus-tools-cli --help`。输入类型和操作 ID 与桌面应用共用
同一份静态注册表。

## 支持的平台

| 平台 | 产物 | 当前状态 |
| --- | --- | --- |
| macOS 12+ | `.app` / `.dmg` | arm64 版本已测试；签名和公证待完成 |
| Windows 10/11 x64 | 便携版 `.zip` | Windows 11 原生构建/启动/IME 通过；Windows 10 待完成 |

本项目有意不提供 NSIS/MSI 安装包。

## Alpha 版本限制

- Windows 11 原生 MinGW/MSVC 构建、便携 ZIP、启动与 IME 验证已通过；
  Windows 10 验证仍待完成。
- 测试包未经签名；用于公开生产版本的 macOS 公证和 Windows 代码签名尚未
  配置。
- YAML 有意只接受单文档安全子集，并拒绝指令、标签、锚点、别名和多文档
  输入。
- JWT 检查会解码声明，但不会验证签名。
- Base64/URL 自动解码在一层后停止。使用“再解一层”可手动处理下一个检测到
  的编码层，且不会修改原始输入。
- HMAC 密钥只保留在进程内存中；替换密钥或关闭 HMAC/摘要面板时，会主动
  覆盖应用持有的密钥和撤销快照。应用无法保证清除操作系统或输入法产生的副本，
  Alpha 版本评估时请勿使用生产环境密钥。

## 开源组件

| 项目 | 在 Zeus Tools 中的用途 |
| --- | --- |
| [EUI-NEO](https://github.com/sudoevolve/EUI-NEO) | 跨平台桌面 UI 与渲染框架 |
| [pugixml](https://github.com/zeux/pugixml) | 严格 XML 解析与序列化 |
| [yaml-cpp](https://github.com/jbeder/yaml-cpp) | YAML 解析、格式化与转换 |
| [toml++](https://github.com/marzer/tomlplusplus) | TOML 解析、格式化与 JSON 转换 |
| [Font Awesome Free](https://fontawesome.com/) | 通用界面图标 |
| [Primer Octicons](https://github.com/primer/octicons) | GitHub 项目链接图标 |

直接依赖均锁定版本，以保证构建可复现。完整许可证信息及 EUI-NEO 引入的
运行时组件请查看 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。

## 构建前置条件

- CMake 3.20+
- Ninja
- 支持 C++17 的编译器
- macOS：Xcode Command Line Tools
- Windows：安装“使用 C++ 的桌面开发”组件的 Visual Studio 2022

依赖通过 CMake FetchContent 锁定，包括 EUI-NEO、pugixml、yaml-cpp 和 toml++。首次
配置需要网络连接，除非提供本地依赖源码；构建后的应用本身不需要网络连接。

## 构建并测试核心模块

```sh
cmake --preset core
cmake --build --preset core
ctest --preset core
./build/core/zeus_detection_report
```

运行使用 10 MB 合成数据的基准测试：

```sh
./build/core/zeus_core_benchmark
./build/core/zeus_csv_benchmark
./build/core/zeus_app_processing_benchmark
```

同一构建还会生成 `build/core/zeus-tools-cli`。

## 构建桌面应用

```sh
cmake --preset dev
cmake --build --preset dev
```

复用已有的 EUI-NEO 源码目录：

```sh
cmake --preset dev -DZEUS_EUI_NEO_SOURCE_DIR=/absolute/path/to/EUI-NEO
cmake --build --preset dev
```

在 macOS 上，开发版可执行文件生成于 `build/dev/zeus_tools`。各平台发布命令
和签名说明请参阅 [docs/packaging.md](docs/packaging.md)。

## 打包

macOS：

```sh
cmake --preset package-macos
cmake --build --preset package-macos
cmake --build build/package-macos --target package
```

Windows Developer PowerShell：

```powershell
cmake --preset package-windows
cmake --build --preset package-windows
cmake --build build/package-windows --target package
```

## 仓库结构

```text
include/zeus/   核心模块的公开接口
src/core/       检测、格式化、转换、加密和数据模型
src/app/        应用状态、控制器、处理服务、EUI-NEO 页面和自定义控件
src/cli/        离线 stdin/stdout 命令行入口
src/platform/   macOS、Windows 和后备平台适配器
tests/          单元测试和合成数据性能基准
resources/      应用图标和打包资源
docs/           产品、架构、开发和发布文档
```

## 贡献与安全

如果 Zeus Tools 对你有帮助，欢迎为项目点一个
[Star](https://github.com/cubehead/zeus_tools/stargazers)。Bug 与明确的功能建议
可以提交到 [GitHub Issues](https://github.com/cubehead/zeus_tools/issues)。

提交 Pull Request 前请阅读 [CONTRIBUTING.md](CONTRIBUTING.md)。安全问题请按照
[SECURITY.md](SECURITY.md) 私下报告，切勿在公开 Issue 中附上真实密钥或私有
数据。

项目现阶段有意不包含 GitHub Actions。维护者会在发布前，分别在 macOS 和
Windows 上手动运行文档中列出的构建和测试命令。

## 许可证

Zeus Tools 使用 [MIT License](LICENSE) 发布。第三方组件保留各自的许可证，
详见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。
