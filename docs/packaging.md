# Windows 与 macOS 打包

## 产物策略

| 平台 | 产物 | 安装方式 | 当前签名状态 |
| --- | --- | --- | --- |
| macOS arm64/x86_64 | `.app` + CLI + `.dmg` + `.sha256` | 拖入 Applications；CLI 可复制到 PATH | 默认无 Developer ID 签名；支持显式签名 |
| Windows x64 | Portable `.zip` + `.sha256` | 解压后运行 `ZeusTools.exe` 或 CLI | 默认未签名；不生成 NSIS/MSI 安装包 |

版本来自顶层 `project(... VERSION ...)`，平台构建号来自 `ZEUS_BUILD_NUMBER`。正式发布前应同时更新两者，并复核仓库根目录的 `LICENSE` 与 `THIRD_PARTY_NOTICES.md`。
macOS 最低系统版本由 `ZEUS_MACOS_MIN_VERSION` 统一控制，默认 12.0；配置会同时写入
Mach-O deployment target 与 `LSMinimumSystemVersion`，产物校验器会拒绝两者不一致的包。

`v0.2.0` 已发布 macOS arm64 DMG 和 Windows x64 Portable ZIP。macOS 产物在
本机完成构建与磁盘映像验证；Windows 产物使用 MinGW-w64 交叉构建，并完成
ZIP、PE 架构、GUI 子系统、VersionInfo、资源和系统 DLL 依赖检查，但尚未
替代 Windows 10/11 原生运行与 IME 验证。两个产物均未签名。

两个平台的发布包都必须包含项目许可证、隐私说明、第三方声明，以及
`docs/licenses/` 中与锁定依赖和实际运行时资源对应的完整许可证文本。

## 图标资源

- ImageGen 母版：`resources/icons/zeus-tools-master-transparent.png`（仅闪电与花括号，外围透明）
- macOS：`resources/icons/zeus-tools.icns`
- Windows：`resources/icons/zeus-tools.ico`
- 预览：`resources/icons/zeus-tools-1024.png`

桌面运行时显式使用 `assets/zeus-tools-1024.png`，避免 EUI-NEO 默认的 `assets/icon.png` 覆盖 macOS Dock 或 Windows 任务栏图标。

图标设计为深海军蓝圆角底、蓝紫闪电和白色代码花括号，在 16px 下仍保留主要轮廓。重新生成平台尺寸：

```sh
./scripts/generate-icons.sh
```

脚本在 macOS 使用 `sips` 与 `iconutil`。生成后的 ICNS/ICO 已提交到项目，因此 Windows 构建机不需要运行该脚本。

## macOS 打包

首次配置时可指定本地 EUI-NEO v0.5.3 源码，避免重复下载：

```sh
cmake --preset package-macos \
  -DZEUS_EUI_NEO_SOURCE_DIR=/absolute/path/to/EUI-NEO
cmake --build --preset package-macos
cmake --build build/package-macos --target package
```

输出目录：`build/package-macos/packages/`。

生成后验证 DMG 的 SHA-256、应用、CLI、运行时资源、版本信息、许可证和禁止资源：

```sh
./scripts/check-package.sh macos \
  build/package-macos/packages/Zeus-Tools-0.2.0-macOS-arm64.dmg
```

如有 Developer ID Application 证书，在配置阶段显式传入：

```sh
cmake --preset package-macos \
  -DZEUS_CODESIGN_IDENTITY="Developer ID Application: Example (TEAMID)"
```

配置会在生成 DMG 前对 `.app` 启用 Hardened Runtime、时间戳和深度签名。公证凭据不写入仓库；建议使用 Keychain Profile：

```sh
xcrun notarytool submit Zeus-Tools-0.2.0-macOS-arm64.dmg \
  --keychain-profile ZEUS_NOTARY --wait
xcrun stapler staple Zeus-Tools-0.2.0-macOS-arm64.dmg
spctl --assess --type open --context context:primary-signature -v Zeus-Tools-0.2.0-macOS-arm64.dmg
```

若需 Intel 包，在 Intel runner 原生构建；若要 Universal 2，需先确认 EUI-NEO 及全部静态依赖均支持 `arm64;x86_64`，再使用 `-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"`。

## Windows 便携 ZIP

在 Visual Studio 2022 Developer PowerShell 中执行：

```powershell
cmake --preset package-windows
cmake --build --preset package-windows
cmake --build build/package-windows --target package
```

ZIP 包含：

- `ZeusTools.exe`；
- `zeus-tools-cli.exe`，用于离线 stdin/stdout 管道；
- Zeus Tools 图标与实际使用的 Font Awesome 图标字体；不会打包 EUI Gallery 的未使用演示字体；
- `LICENSE`、`PRIVACY.md`、`THIRD_PARTY_NOTICES.md` 与 `licenses/` 下的第三方完整许可证；
- CPack 生成的 SHA-256 校验文件。

Windows 资源脚本会嵌入应用图标、产品名、语义版本、构建号和文件描述。项目明确不配置 NSIS/MSI；发布的是解压即用的 Portable ZIP。

生成后验证 ZIP 的 SHA-256、GUI/CLI、运行时资源、许可证和禁止资源：

```sh
./scripts/check-package.sh windows \
  build/package-windows/packages/Zeus-Tools-0.2.0-Windows-x86_64-portable.zip
```

Windows 原生环境也可以先解压 ZIP，再使用相同命令把第二个参数指向解压根目录。

在不依赖 Bash 的原生 Windows PowerShell 中，可以执行完整的哈希、包内容、
VersionInfo、CLI 和 GUI 启动验收：

```powershell
.\scripts\check-package.ps1 `
  build\package-windows\packages\Zeus-Tools-0.2.0-Windows-AMD64-portable.zip
```

正式签名版本追加 `-RequireSignature`；只做服务器或无桌面环境检查时可追加
`-SkipGui`。该脚本可直接复制到 Windows 10/11 原生验收环境执行。

若后续拥有代码签名证书，应在执行 `package` 前签名：

```powershell
signtool sign /fd SHA256 /td SHA256 /tr https://timestamp.digicert.com `
  /sha1 <CERTIFICATE_THUMBPRINT> build\package-windows\ZeusTools.exe
signtool verify /pa /v build\package-windows\ZeusTools.exe
```

证书指纹及凭据只能通过本机安全存储或受控发布环境的 Secret Store 提供，禁止写入仓库。

## 发布前检查

- [ ] Release 构建和核心测试通过。
- [ ] 包内版本、图标、Bundle ID/Windows VersionInfo 正确。
- [ ] 从全新用户目录启动，字体和图标资源无缺失。
- [ ] JSON、Base64、URL Decode、CSV、搜索、选择与复制冒烟测试通过。
- [ ] macOS Gatekeeper 或 Windows Authenticode 校验符合本次发布策略。
- [ ] SHA-256 与发布页附件一致。
- [x] 项目许可证和第三方许可证清单已确定并加入打包配置。
- [ ] Windows 在 Windows 10、11 各验证一次；macOS 至少验证声明的最低版本和当前版本。
