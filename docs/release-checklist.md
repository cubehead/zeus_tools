# Release checklist

This checklist is intentionally manual. The repository does not use GitHub CI.

## Source and version

- [ ] Working tree contains only intended source, documentation and assets.
- [ ] `./scripts/check-repository.sh` passes.
- [ ] `project(... VERSION ...)`, build number and `CHANGELOG.md` agree.
- [ ] Core tests pass from a clean build directory.
- [ ] Synthetic JSON and CSV benchmarks have no material regression.
- [ ] Dependency revisions and `THIRD_PARTY_NOTICES.md` are current.

## Functional smoke test

- [ ] JSON format/minify/escape and JSON → YAML/XML/CSV.
- [ ] XML/YAML format and conversion to JSON.
- [ ] Base64 and URL auto-decode exactly one layer.
- [ ] Unicode Escape encodes UTF-8, decodes valid `\uXXXX`/surrogate pairs and
      continues exactly one layer at a time.
- [ ] JWT header/payload inspection without claiming signature verification.
- [ ] CSV auto/manual delimiter, header switch, scroll, search and TSV copy.
- [ ] Text selection, double-click word selection, copy and search navigation.
- [ ] MD5/SHA/HMAC output and weak-algorithm warning.
- [ ] System/light/dark themes and all locale menu entries.
- [ ] Invalid input displays a concise error and detailed hover card.
- [ ] CLI stdin/file input, registered action, stderr diagnostics and 10 MiB limit.

## Performance and platform

- [ ] 10 MB input paste, edit, undo, selection, search and clear complete without
      a crash or prolonged UI freeze.
- [ ] Peak memory is recorded for 10 MB JSON and CSV.
- [ ] macOS IME/emoji/combining characters pass.
- [ ] Windows 10 and Windows 11 IME/emoji/combining characters pass.
- [ ] Minimum supported macOS version starts the app.

## Package

- [ ] macOS `.app` and `.dmg` contain the intended icon and version.
- [ ] Windows portable ZIP contains `ZeusTools.exe`, required runtime assets,
      `zeus-tools-cli.exe`, icon and VersionInfo; no installer is generated.
- [ ] A fresh user account can launch each artifact.
- [ ] Uninstall/removal leaves no content history; only optional preference files
      may remain.
- [ ] SHA-256 checksums are generated and verified.
- [ ] Signing/notarization status is stated accurately in release notes.

## Privacy and security

- [ ] No real user content, key, credential or signing identity is in the repo or
      package.
- [ ] Application logs and crash output do not include input/output/search/HMAC.
- [ ] Offline smoke test passes with network access disabled.
- [ ] XML DTD/entity and constrained YAML security fixtures pass.
- [ ] Public security reporting is enabled or a private contact route exists.

## GitHub release

- [ ] README screenshots match the released UI.
- [ ] README, privacy, security, contributing and license links render correctly.
- [ ] Release notes include known limitations and supported platforms.
- [ ] Artifacts and checksum files are attached manually.
- [ ] No `.github/workflows` directory is added unless maintainers explicitly
      reverse the current no-CI decision.
