# 开发指南

## 当前阶段

项目处于 0.1 Alpha 发布收尾阶段。当前已实现：

当前实现与完整产品基线之间的差距统一记录在根目录 README 的
“Alpha limitations”及[发布检查清单](./release-checklist.md)，本节只描述已落地能力。

- EUI-NEO v0.5.3 应用骨架。
- 上方多行原始输入。
- 严格 JSON 格式化及行列错误信息。
- XML 自动识别、严格解析、格式化、行列错误和 token 高亮；混合内容不注入语义空白，`DOCTYPE`/`ENTITY` 在解析前拒绝。
- YAML 保守自动识别、规范化、错误行列和 token 高亮；首版采用单文档安全子集，限制 10 MB、128 层和 10 万节点，并拒绝 Directive、Tag、Anchor/Alias。
- JSON → YAML、YAML → JSON 类型安全转换，以及 JSON ↔ XML 转换。JSON → XML 使用固定映射：数组项使用 `<item>`，非法元素键名使用 `<entry key="…">`；XML → JSON 保留根节点，属性使用 `@` 前缀，元素文本使用 `#text`，同名子节点转为数组，混合内容按顺序写入 `#content`。
- JSON Escape/Unescape 动态操作：普通 JSON 提供 Escape；合法 JSON 字符串内嵌 JSON，或 `{\"key\":...}` 原始转义形式会自动识别、仅反转义一层并格式化。
- JSON → CSV：对象数组合并字段为表头，缺失值留空；嵌套结构以紧凑 JSON 单元格保存并按 CSV 规则转义，结果直接进入 Table 预览。
- 下方只读虚拟行结果视图、行号和 JSON token 高亮。
- 文本与 CSV 搜索支持匹配计数、上下跳转、大小写敏感和正则模式；无效正则显示本地化错误。文本结果和 CSV 单元格均按每次命中的精确字符范围绘制高亮，不再整行或整格着色，并以更强颜色区分当前命中。
- 只读结果的鼠标按字符跨行选择、贴合文字的选区高亮、`Cmd/Ctrl+A`、`Cmd/Ctrl+C` 和工具栏复制；左侧行号不劫持为整行选择，大选区字节统计采用零拷贝计算，跨屏拖选按文档绝对滚动位置定位。
- 项目自有 `RichTextView` 只读富文本控件：集中管理 token 分段着色、虚拟行、搜索背景、窗口缩放坐标、选择与复制；支持双击选择英文/中文单词、三击选择整行，页面层不再给每个虚拟行绑定选择手势。
- JSON 对象/数组、XML 多行元素和 YAML 缩进块支持行号槽折叠，使用带边框的小方形 `-`（展开）和 `+`（收起）控件；折叠只改变可见行映射，不修改格式化结果。搜索跳转会展开包含当前命中的折叠区间，全选与复制仍基于完整原文。
- 搜索、匹配跳转、匹配数量和复制集中在窗口底栏；顶部仅保留标题、主题与清空等应用级操作。
- 多语言 i18n 已覆盖简体中文、繁體中文、English、日本語、한국어、Español、Français、Deutsch、Português、Русский；语言下拉项使用各自母语名称，“跟随系统”按当前生效语言显示，选择结果本地持久化。主题图标、语言下拉和清空入口右对齐放置在窗口右上角。
- MD5、SHA-1、SHA-256、SHA-512 与 HMAC 首版完成，支持 Hex/Base64 输出；macOS 使用 CommonCrypto、Windows 使用 CNG/BCrypt，无额外运行时依赖。HMAC 密钥仅保存在当前进程内存，MD5/SHA-1 明确标记为弱算法。
- 自动检测与手动类型覆盖：可强制按 JSON、XML、YAML、CSV、Base64、URL Decode 或纯文本处理，且不修改原文；JSON、Base64、URL Encode 检测及单层解码处理链已接入。
- JWT 三段式 Base64URL 自动识别：只读展示并高亮 Header、Payload 和原始 Signature；明确标记签名未验证，claims 可直接搜索和复制。
- Base64 二进制安全摘要（字节数与 Hex 预览），避免把二进制直接渲染为乱码。
- CSV 自动分隔符检测（逗号、Tab、分号、竖线）、手动分隔符覆盖、RFC 风格引号/多行字段解析、Table 预览和 TSV 复制。
- CSV 专用只读虚拟表格：支持“首行为表头”开关；关闭时生成 Column 1/2… 列名并把首行作为数据。表格仅组合当前可见的行列，支持横纵滚动、单元格点击选择、单元格复制，以及搜索后定位并高亮具体行列。
- 原文和只读结果之间的上下文操作带：类型、默认动作、类型专属动作、通用动作依次排列；Encode/Decode 根据输入检测结果动态切换。
- System/Light/Dark 三态主题：默认跟随系统，手动覆盖会持久化；两套语法和状态配色独立适配。
- 字体分层：macOS UI 通过 CoreText 使用系统苹方，Windows 使用微软雅黑；格式化输入、只读结果和 CSV 数据在 macOS 使用 SF Mono、Windows 使用 Cascadia Mono，并保留平台回退链。UI/代码字号分别调优，文本测量与绘制使用同一代码字体。
- 按钮字号单独放大并通过实际窗口截图复核：顶部设置按钮为 19–20 pt、主要格式操作约 20 pt、摘要入口 19 pt、底部复制 18 pt；同步增加按钮高度与必要宽度，不联动放大状态栏和数据内容，避免压缩编辑区域。
- 中间操作条采用紧凑工具项布局：按钮高度 30 pt、缩短水平留白并增加项间距；摘要入口保持透明普通态，通过 `▸/▾` 表达摘要与 HMAC 面板的收起和展开，避免与转换操作的选中态混淆。
- HMAC 面板支持原始输入/当前结果来源和 UTF-8、Hex、Base64 密钥编码；密钥默认遮罩，关闭 HMAC、摘要面板或清空应用时会擦除组件文本及撤销/重做快照。
- 自动解码仍严格限制为一层；若当前结果仍是明确的 Base64、URL 编码或转义 JSON，操作条显示“再解一层”，并在状态栏保留处理链。
- 操作条类型标签使用 18 pt，摘要/HMAC 展开行的分组标签使用 17 pt；两行左内边距统一为 16 pt，避免标签贴近容器边框。
- 输入变化采用 180 ms 防抖并在后台完成检测、转换、CSV 解析和高亮文档构建；新任务会取消旧任务，避免过期结果覆盖当前输入。
- 结果搜索采用 100 ms 防抖和可取消后台任务；CSV 搜索按行检查取消状态，连续输入不会让旧搜索覆盖新结果。
- CSV 超长单元格只渲染安全截断预览（换行显示为 `↵`），选择复制与整表 TSV 仍使用完整数据；横纵滚动条轨道支持点击跳转。
- 独立于 UI 的核心测试与 10 MB 基准程序。

### 2026-08-13 基准结果

测试设备为当前 Apple Silicon 开发机，Debug 构建，数据为程序生成的扁平对象数组：

| 输入 | JSON 格式化 | 高亮文档构建 | 普通文本搜索 | 合计 |
| ---: | ---: | ---: | ---: | ---: |
| 1 MB | 16 ms | 106 ms | 7 ms | 129 ms |
| 10 MB | 182 ms | 1084 ms | 92 ms | 1358 ms |

10 MB 核心处理已低于 2 秒目标。该数据不包括 EUI 输入控件、GPU 首屏渲染和内存峰值，不能替代完整端到端性能结论。

同一开发机新增 10 MB CSV 基准（98,103 行、7 列）：解析约 46 ms，遍历全部单元格搜索约 10 ms。该结果验证核心解析和搜索余量，仍不包含输入控件、内存峰值与 GPU 交互耗时。

CSV 大数据虚拟滚动、精确搜索定位以及 XML、YAML、摘要/HMAC、Base64、URL Decode 已完成首个可用版本。Windows 原生构建验证、签名/公证和 10 MB UI 端到端性能验收仍按发布清单推进。

## 依赖

- CMake 3.20+
- C++17 编译器
- Ninja（使用 presets 时）
- EUI-NEO v0.5.3（固定提交 `f2a3b721…`）
- pugixml v1.15（固定提交 `ee86beb3…`；MIT）
- yaml-cpp 0.9.0（固定提交 `56e3bb55…`；MIT）
- macOS：Xcode Command Line Tools
- Windows：Visual Studio 2022 C++ Desktop Workload

## 构建核心

核心库不依赖 EUI，可离线构建与测试：

```sh
cmake --preset core
cmake --build --preset core
ctest --preset core
```

运行默认 10 MB 基准：

```sh
./build/core/zeus_core_benchmark
```

也可传入目标字节数：

```sh
./build/core/zeus_core_benchmark 1048576
```

CSV 10 MB 基准：

```sh
./build/core/zeus_csv_benchmark
```

## 构建桌面应用

如果已经有 EUI-NEO v0.5.3 源码：

```sh
cmake --preset dev -DZEUS_EUI_NEO_SOURCE_DIR=/absolute/path/to/EUI-NEO
cmake --build --preset dev
./build/dev/zeus_tools
```

不传 `ZEUS_EUI_NEO_SOURCE_DIR` 时，CMake FetchContent 会下载并锁定到 v0.5.3 对应提交。pugixml 与 yaml-cpp 分别锁定到 v1.15 和 0.9.0。完成依赖下载后，业务核心和运行时处理均不需要网络。

## 目录

```text
include/zeus/       公共 C++ 核心接口
src/core/           格式检测、格式化、高亮文档和搜索
src/app/            应用状态、控制器、纯处理服务和 EUI-NEO 页面/控件
tests/              单元测试与性能基准
docs/               产品、架构、评估、实施和开发文档
resources/          应用图标与平台打包资源
scripts/            可重复执行的资源生成脚本
```

`src/app/app.cpp` 只定义窗口和运行参数；`main_view.cpp` 只负责尺寸计算和页面编排；
`src/app/views/` 分别实现 Header、输入区、操作栏、Crypto、结果区和搜索栏；
`app_controller.cpp` 负责状态变更、异步调度和用户操作；
`processing_service.cpp` 负责不依赖 EUI 的输入分析与单层解码。
自定义富文本和 CSV 控件不包含 EUI 的应用实现头，以避免多翻译单元重复定义。

更新 README 截图时，可通过 CMake 缓存参数 `ZEUS_DOCS_SCENARIO` 启动只使用
合成数据的固定场景：`json`、`csv`、`jwt`、`hmac` 或 `about`。该值未设置时，合成数据
不会编译进应用，也不会改变正常发行行为。

## 隐私约束

- 禁止把原始输入、格式化结果、搜索内容或未来的 HMAC 密钥写入日志。
- 设置文件目前只允许保存主题和语言偏好，不得扩展为输入历史或内容缓存。
- 基准只使用程序生成的合成数据。
- 发布构建不得增加网络、Shell 或任意文件访问能力，除非先更新架构决策和产品范围。
