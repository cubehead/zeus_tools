# Changelog

All notable user-visible changes will be documented here. This project follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and intends to use
semantic versioning after the first stable release.

## [Unreleased]

### Added

- Added a statically linked processing registry with stable action and input IDs,
  replacing positional UI/dispatch indexes without introducing runtime plugins.
- Registered all explicit core processing modes by functional handler boundary, with
  compile-time completeness checks and a separate ordered auto-detection pipeline.
- Centralized content labels, compact labels, result syntax, and export extensions in
  complete registered metadata for every content kind.
- Added bilingual README release/download/star/license/platform badges, quick
  links, project support links and direct open-source component acknowledgements.
- Added a versioned automatic-detection regression corpus covering valid,
  invalid, ambiguous and false-positive inputs.
- Added `Cmd/Ctrl+F` to focus result search and `Esc` to dismiss the topmost
  open dialog, dropdown or crypto panel.
- Added TOML detection, formatting, highlighting, section folding and JSON
  conversion in both directions.
- Added conservative INI/Properties detection, formatting and JSON conversion
  that preserves all values as strings and rejects duplicate keys.
- Added HTML Entity and Hex encode/decode tools plus guarded Unix timestamp
  conversion for exact 10/13 digit candidates.
- Added native file open/export actions and single-file drag-and-drop with a
  10 MB text boundary, UTF-8 BOM handling and no recent-file history.

### Fixed

- Kept the common Base64 Encode action as encoding even when the source already
  looks like Base64; the type-specific Decode action remains separate.
- Prevented ordinary JSON arrays from being misclassified as escaped JSON.

## [0.1.0] - 2026-08-14

### Added

- Structural folding for JSON containers, XML elements and YAML indentation blocks in the read-only result view.
- Folding controls use clear minus/plus indicators for expanded and collapsed regions.
- Search navigation automatically reveals matches hidden inside collapsed regions.
- Dropdown selections close synchronously without popup transitions, and structural result changes invalidate the full-window background to prevent stale partial colors after changing Auto/type, language or CSV delimiter options.
- Offline detection and processing for JSON, XML, YAML, CSV, JWT, Base64 and
  URL-encoded text.
- JSON/YAML/XML conversions, JSON escape/unescape and JSON-to-CSV preview.
- Read-only syntax-highlighted result view with selection, copying and search.
- Virtualized CSV table with delimiter/header controls and exact match ranges.
- MD5, SHA-1, SHA-256, SHA-512 and HMAC tools.
- Manual one-layer continuation for nested Base64, URL encoding and escaped
  JSON, with a visible processing chain and unchanged source input.
- HMAC message-source selection, UTF-8/Hex/Base64 key decoding, masked key
  entry and active in-memory key cleanup when the panel closes.
- System/light/dark themes and ten interface languages.
- macOS DMG and Windows portable ZIP packaging configuration.
- Localized About dialog with project, license and open-source acknowledgement
  links.

### Changed

- Replaced the formatted output layer with a read-only rich text view that
  supports normal character/word selection, copying and exact search matches.
- Added a bounded draggable splitter so the input and result areas can be
  resized without hiding either workspace.
- Reorganized context-sensitive actions by detected input type and kept common
  encoding actions at the end of the action bar.

### Fixed

- Prevented stale partial colors after type, language, theme and CSV option
  changes by invalidating the complete window when required.
- Hardened background processing and CSV parsing so malformed input reports a
  concise error instead of terminating the application.
- Corrected CSV cell-level search highlighting, popup layering and several
  alignment, font-size and first-render issues.

### Security

- XML DTD/entity rejection, constrained YAML parsing and local-only processing.

[Unreleased]: https://github.com/cubehead/zeus_tools/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/cubehead/zeus_tools/releases/tag/v0.1.0
