# ADR-001：桌面 UI 使用 EUI-NEO

- 状态：0.1 Alpha 已接受
- 日期：2026-08-14
- 负责人：Zeus Tools 维护者

## 决策

Zeus Tools 使用 C++17、固定版本的 EUI-NEO 和独立核心库构建 macOS/Windows
桌面应用。EUI-NEO 固定到提交：

`f2a3b72104bd946988f8ebe0a13dda956f3455ae`

格式检测、转换、摘要/HMAC、CSV、文本文档和选择模型不得依赖 EUI；
EUI 相关代码限制在 `src/app/` 和 `src/platform/`。升级框架前必须重新执行
视觉、输入、性能、测试和打包检查。

## 当前结构

- `zeus_core`：格式检测、解析、转换、crypto、CSV 和只读文档模型。
- `src/app/app.cpp`：仅保存应用窗口与运行参数。
- `src/app/main_view.cpp`：页面尺寸计算和区域编排。
- `src/app/views/`：Header、输入区、操作栏、Crypto、结果区和搜索栏。
- `src/app/app_controller.cpp`：状态变更、异步调度、搜索、复制与摘要/HMAC 操作。
- `src/app/processing_service.cpp`：不依赖 EUI 的输入分析、转换结果组织和单层解码。
- `src/app/` 自定义控件：`RichTextView`、CSV 视图与虚拟列表。
- `src/platform/`：系统主题、平台字体、原生文件对话框和 Windows/macOS 差异实现。
- CMake FetchContent：使用不可变提交获取 EUI-NEO、pugixml、yaml-cpp 和 toml++。
- CPack：生成 macOS DMG 和 Windows Portable ZIP，不生成安装器。

## 已有证据

- macOS arm64 Debug/Release、App Bundle 和 DMG 可以构建。
- 只读结果仅组合可见行，支持 UTF-8 选择、复制、精确搜索和结构折叠。
- CSV 视图虚拟化可见行列，核心 10 MB / 98k 行基准通过。
- 10 MB 合成 JSON 的格式化、高亮文档构建和搜索在当前开发机约 1.36 秒。
- 主题、十种语言、平台字体、图标和双平台打包配置已经接入。
- DMG 已通过 SHA-256、磁盘映像和包内资源/许可证检查。

上述证据不替代 Windows 原生验证和完整 UI 端到端性能验收。

## 约束与风险

- 项目自行维护只读富文本与 CSV 虚拟视图。
- EUI-NEO 仍处于 0.x，升级可能影响渲染、命中测试和字体测量。
- Windows IME、键盘、可访问性和打包必须在 Windows 10/11 原生验证。
- 10 MB 输入控件的编辑、撤销、选择、内存峰值和 GPU 首屏仍是发布门槛。
- 发布包只能携带实际使用的字体、图标和完整第三方许可证。
- 文件访问只能由用户主动打开、拖入或导出触发；不得记录内容、路径或最近文件。

## 必须继续执行

- 完成 Windows 10/11 Debug/Release、IME 和 Portable ZIP 验收。
- 记录 macOS/Windows 10 MB 端到端耗时和峰值内存。
- 每次发布前执行 [`scripts/check-repository.sh`](../../scripts/check-repository.sh)
  和[发布检查清单](../release-checklist.md)。
- 不得把 EUI 示例资源、构建缓存、用户内容或开发索引放入仓库或发布包。
