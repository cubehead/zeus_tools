# Zeus Tools

<p align="center">
  <img src="resources/icons/zeus-tools-1024.png" width="128" alt="Zeus Tools icon">
</p>

<p align="center">
  <strong>English</strong> | <a href="README_zh-CN.md">简体中文</a>
</p>

Zeus Tools is an offline, cross-platform developer toolbox for formatting,
inspecting, converting, encoding and searching structured text. It is written
in C++17 with [EUI-NEO](https://github.com/sudoevolve/EUI-NEO) and targets
macOS and Windows.

> Project status: **Alpha**. The macOS arm64 build is usable for testing.
> Windows packaging is configured but still requires native Windows 10/11
> validation before the first public release.

## Highlights

- Automatically detects JSON, XML, YAML, CSV, JWT, Base64 and URL-encoded text.
- Formats JSON, XML and YAML with read-only syntax highlighting.
- Folds nested JSON containers, XML elements and indented YAML blocks.
- Converts JSON ↔ YAML, JSON ↔ XML and JSON → CSV.
- Escapes/unescapes JSON and decodes Base64 or URL encoding one layer at a
  time, with an explicit continuation action for nested content.
- Shows CSV in a virtualized table with delimiter and header controls.
- Searches text and CSV cells with case-sensitive and regular-expression modes.
- Resizes the input and result workspaces with a bounded horizontal splitter.
- Calculates MD5, SHA-1, SHA-256, SHA-512 and HMAC locally.
- Supports system/light/dark themes and ten interface languages.
- Never uploads or saves the text being processed.

## Screenshots

### Structured-text workflow

![Zeus Tools formatting JSON in the dark theme](docs/assets/overview-dark.jpg)

<table>
  <tr>
    <td width="50%"><img src="docs/assets/csv-table.jpg" alt="Virtual CSV table with delimiter and header controls"></td>
    <td width="50%"><img src="docs/assets/digest-hmac.jpg" alt="Digest and HMAC panel"></td>
  </tr>
  <tr>
    <td align="center">Virtual CSV preview and search</td>
    <td align="center">Digest and HMAC tools</td>
  </tr>
</table>

Screenshots use synthetic data and show the current macOS alpha UI. Platform
font rendering may differ on Windows.

## Main features

| Input or tool | Automatic/default behavior | Additional actions |
| --- | --- | --- |
| JSON | Pretty format, syntax highlight and structural folding | Minify, escape, YAML/XML/CSV conversion |
| Escaped JSON | Detect a JSON string or escaped object | Unescape exactly one layer |
| XML | Strict parse, safe format, highlight and element folding | Convert to JSON; reject DTD/entities |
| YAML | Conservative safe-subset parse, format and indentation folding | Convert to JSON |
| CSV | Detect comma, Tab, semicolon or pipe | Override delimiter/header, virtual table, TSV copy |
| Base64 | Decode one high-confidence layer | Standard and URL-safe input; binary-safe preview |
| URL encoding | Decode one high-confidence layer | Encode arbitrary UTF-8 text |
| JWT | Decode and format header/payload | Search/copy claims; signature is not verified |
| Text | Read-only result and search | Upper/lower case, Base64/URL encode |
| Digest/HMAC | Manual local calculation | Input/result source, UTF-8/Hex/Base64 keys, masked key, Hex/Base64 output |

Automatic detection can always be overridden from the input-type menu without
changing the source text.

## Privacy

All formatting, decoding, searching and hashing happens in the local process.
Zeus Tools has no account, cloud service, telemetry or content history. Only
theme and language preferences are persisted. See [PRIVACY.md](PRIVACY.md).

## Supported platforms

| Platform | Artifact | Current status |
| --- | --- | --- |
| macOS 12+ | `.app` / `.dmg` | arm64 build tested; signing and notarization pending |
| Windows 10/11 x64 | Portable `.zip` | configured; native validation pending |

The project intentionally does not create an NSIS/MSI installer.

## Alpha limitations

- Windows build and portable ZIP configuration have not yet completed native
  Windows 10/11 validation.
- Test packages are unsigned; macOS notarization and Windows code signing are
  not yet configured for a public production release.
- YAML intentionally accepts a single-document safe subset and rejects
  directives, tags, anchors, aliases and multi-document input.
- JWT inspection decodes claims but does not verify the signature.
- Automatic Base64/URL decoding stops after one layer. Use `Decode +1` to
  process another detected layer manually; the source input remains unchanged.
- HMAC keys are kept in process memory only and are actively cleared when the
  HMAC or digest panel closes. Test packages remain Alpha software, so avoid
  using production secrets during evaluation.

## Build prerequisites

- CMake 3.20+
- Ninja
- A C++17 compiler
- macOS: Xcode Command Line Tools
- Windows: Visual Studio 2022 with Desktop development with C++

Dependencies are pinned by CMake FetchContent: EUI-NEO, pugixml and yaml-cpp.
The first configure requires network access unless local dependency sources are
provided; the built application itself does not require a network connection.

## Build and test the core

```sh
cmake --preset core
cmake --build --preset core
ctest --preset core
```

Run the synthetic 10 MB benchmarks:

```sh
./build/core/zeus_core_benchmark
./build/core/zeus_csv_benchmark
```

## Build the desktop app

```sh
cmake --preset dev
cmake --build --preset dev
```

To reuse an existing EUI-NEO checkout:

```sh
cmake --preset dev -DZEUS_EUI_NEO_SOURCE_DIR=/absolute/path/to/EUI-NEO
cmake --build --preset dev
```

On macOS the development executable is generated as `build/dev/zeus_tools`.
Platform release commands and signing notes are in
[docs/packaging.md](docs/packaging.md).

## Package

macOS:

```sh
cmake --preset package-macos
cmake --build --preset package-macos
cmake --build build/package-macos --target package
```

Windows Developer PowerShell:

```powershell
cmake --preset package-windows
cmake --build --preset package-windows
cmake --build build/package-windows --target package
```

## Repository layout

```text
include/zeus/   Public core interfaces
src/core/       Detection, formatting, conversion, crypto and data models
src/app/        App state, controller, processing service, EUI-NEO view and widgets
src/platform/   macOS, Windows and fallback platform adapters
tests/          Unit tests and synthetic performance benchmarks
resources/      Application icons and packaging assets
docs/           Product, architecture, development and release documents
```

## Contributing and security

Read [CONTRIBUTING.md](CONTRIBUTING.md) before opening a pull request. Please
report security issues privately according to [SECURITY.md](SECURITY.md), and
never attach real secrets or private payloads to a public issue.

GitHub Actions are intentionally not included at this stage. Maintainers run
the documented build and test commands locally on macOS and Windows before a
release.

## License

Zeus Tools is released under the [MIT License](LICENSE). Third-party components
retain their own licenses; see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
