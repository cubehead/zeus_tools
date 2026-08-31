# Release checklist

This checklist is intentionally manual. The repository does not use GitHub CI.

The checked state records validation of the current `main` branch through
2026-08-31. Unchecked items require another operating-system version, native
assistive technology, signing credentials, a fresh account, or the final
release upload; they must not be inferred from a cross-build.

## Source and version

- [x] Working tree contains only intended source, documentation and assets.
- [x] `./scripts/check-repository.sh` passes; on native Windows,
      `.\scripts\check-repository.ps1` passes.
- [x] `project(... VERSION ...)`, build number and `CHANGELOG.md` agree.
- [x] Core tests pass from a clean build directory.
- [x] `core-sanitize` ASan + UBSan tests pass on a Clang/GCC development host.
- [x] Synthetic JSON and CSV benchmarks have no material regression.
- [x] `zeus_detection_report` passes its corpus coverage floors with no mismatch.
- [x] Dependency revisions and `THIRD_PARTY_NOTICES.md` are current.

## Functional smoke test

- [x] JSON format/minify/escape and JSON → YAML/XML/CSV.
- [x] XML/YAML format and conversion to JSON.
- [x] Base64, Base64 Data URL and URL auto-decode exactly one layer; verified
  PNG/JPEG Data URLs preview without creating a temporary file.
- [x] Unicode Escape encodes UTF-8, decodes valid `\uXXXX`/surrogate pairs and
      continues exactly one layer at a time.
- [x] JWT header/payload inspection without claiming signature verification.
- [x] CSV auto/manual delimiter, header switch, scroll, search and TSV copy.
- [x] Text selection, double-click word selection, copy and search navigation.
- [x] CRC32/MD5/SHA/HMAC output plus checksum/weak-algorithm warnings.
- [x] System/light/dark themes and all locale menu entries.
- [x] Invalid input displays a concise error and detailed hover card.
- [x] CLI stdin/file input, registered action, stderr diagnostics and 10 MiB limit.

## Performance and platform

- [x] 10 MB input paste, edit, undo, selection, search and clear complete without
      a crash or prolonged UI freeze.
- [x] macOS exact 10 MiB single-line JSON reaches a responsive paged editor
      and formatted result without measuring the complete line in either UI surface.
- [x] Large-input pages preserve UTF-8 boundaries and support full-document
      cross-page selection, replacement and undo/redo without copying the entire
      source into the EUI input model.
- [x] macOS exact 10 MiB CSV reaches the 640-page input preview and virtual table,
      completes analysis once, then returns to idle CPU; stable RSS is recorded.
- [x] macOS Physical footprint and process-lifetime peak are recorded for exact
      10 MiB JSON and CSV scenarios.
- [x] macOS UTF-8 file input, emoji/combining-character rendering, selection,
      copy/cut/paste and undo/redo pass.
- [ ] macOS IME candidate-window composition passes with a system input method.
- [x] Windows 11 IME/emoji/combining characters pass.
- [x] Windows 11 exact 10 MiB JSON/CSV, full-document replacement and undo/redo,
      search, oversized-input guard and explicit continue path pass.
- [ ] Windows 10 IME/emoji/combining characters pass.
- [ ] Minimum supported macOS version starts the app.

## Package

- [x] macOS `.app` and `.dmg` contain the intended icon and version.
- [x] Windows portable ZIP contains `ZeusTools.exe`, required runtime assets,
      `zeus-tools-cli.exe`, icon and VersionInfo; no installer is generated.
- [ ] A fresh user account can launch each artifact.
- [ ] Uninstall/removal leaves no content history; only optional preference files
      may remain.
- [x] SHA-256 checksums are generated and verified for the current Windows
      MinGW-w64 and MSVC portable packages.
- [x] `scripts/check-package.sh` passes against the current macOS DMG and Windows
      Portable ZIP; `scripts/check-package.ps1` passes natively on Windows.
- [x] `scripts/check-package.ps1` passes natively against the current Windows
      MinGW-w64 and MSVC portable packages, including CLI and GUI startup.
- [x] Signing/notarization status is stated accurately in release notes.

## Privacy and security

- [x] No real user content, key, credential or signing identity is in the repo or
      package.
- [x] Application logs do not include input/output/search/HMAC content; crash
      reports remain under operating-system control.
- [x] Offline smoke test passes with no application network sockets.
- [x] XML DTD/entity and constrained YAML security fixtures pass.
- [x] A private reporting route is documented in `SECURITY.md`.

## GitHub release

- [x] README screenshots match the released UI.
- [x] README, privacy, security, contributing and license targets resolve locally.
- [x] Release notes include known limitations and supported platforms.
- [ ] Artifacts and checksum files are attached manually.
- [x] No `.github/workflows` directory is added unless maintainers explicitly
      reverse the current no-CI decision.
