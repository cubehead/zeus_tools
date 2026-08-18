# Changelog

All notable user-visible changes will be documented here. This project follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and intends to use
semantic versioning after the first stable release.

## [Unreleased]

### Added

- Added a reproducible Clang/GCC ASan + UBSan core test preset for local
  stability checks without introducing GitHub CI.
- Added SHA-384 digest and HMAC-SHA384 using the native macOS and Windows
  cryptography providers, with standard-vector coverage.
- Added a manual CRC32 checksum with Hex/Base64 output, standard-vector tests
  and an explicit non-cryptographic warning; CRC32 is not offered for HMAC.
- Added a native PowerShell Windows package validator covering SHA-256,
  packaged files, VersionInfo, CLI behavior, Authenticode status and GUI startup.
- Added a native Authenticode signing helper that validates the certificate,
  signs both GUI and CLI with an RFC 3161 timestamp, and verifies each signature.
- Added strict package validation for valid timestamped Authenticode signatures
  on both Windows executables.
- Added a native PowerShell repository publication check for Windows release
  environments.

### Fixed

- Reduced Windows button/dropdown typography and action-strip spacing so
  format-specific and common actions remain visible at the default window width.
- Fixed digest and HMAC results becoming empty on compilers that evaluate the
  output-vector move before the platform hashing call.
- Made Windows CNG hashing allocate the provider-reported hash object and handle
  empty messages without passing a null input pointer.
- Statically linked the MinGW runtime into Windows GUI and CLI executables so the
  Portable ZIP does not require external GCC, C++ or pthread DLLs.
- Compiled all MSVC sources as UTF-8 so localized strings are independent of the
  active Windows system code page.
- Preserved active Windows IME compositions by ignoring intermediate GLFW
  character callbacks until the selected candidate is committed.

## [0.2.0] - 2026-08-17

### Added

- Added table-driven execution smoke coverage for every registered desktop input
  type and action, including handler applicability, output kind and presentation construction.
- Added a reusable macOS DMG/Windows Portable ZIP validator covering the GUI,
  CLI, runtime assets, metadata, complete license set and forbidden demo files.
- Added an offline `zeus-tools-cli` stdin/file-to-stdout pipeline using the same
  input types, stable action IDs, processing service and 10 MiB boundary as the app.
- Added Unicode Escape/Unescape with explicit `\uXXXX` detection, supplementary
  surrogate-pair support, manual input override and one-layer continuation.
- Added a statically linked processing registry with stable action and input IDs,
  replacing positional UI/dispatch indexes without introducing runtime plugins.
- Registered all explicit core processing modes by functional handler boundary, with
  compile-time completeness checks and a separate ordered auto-detection pipeline.
- Made core processing registrations the single source of stable action IDs, with compile-time
  uniqueness checks and application actions deriving IDs from their processing modes.
- Centralized content labels, compact labels, result syntax, and export extensions in
  complete registered metadata for every content kind.
- Separated detected source kinds from result output kinds so decoded JSON and JWT inspection
  use JSON highlighting and export metadata without losing their source classification.
- Removed redundant structured/tabular result flags; registered output kinds now determine
  syntax highlighting and CSV table presentation without conflicting state combinations.
- Avoided running automatic detection before an explicitly selected input processor, removing
  a redundant full parse for large manually typed inputs.
- Added bilingual README release/download/star/license/platform badges, quick
  links, project support links and direct open-source component acknowledgements.
- Added a versioned automatic-detection regression corpus covering valid,
  invalid, ambiguous and false-positive inputs.
- Added a reproducible detection-quality report with coverage floors and expanded
  the corpus to 55 cases, including common developer-text false-positive boundaries.
- Added `Cmd/Ctrl+F` to focus result search and `Esc` to dismiss the topmost
  open dialog, dropdown or crypto panel.
- Added `Cmd/Ctrl+O` for native file open and `Cmd/Ctrl+S` for result export.
- Added a 10 MiB application-processing benchmark covering detection, formatting,
  presentation-model construction and search for JSON and CSV.
- Added TOML detection, formatting, highlighting, section folding and JSON
  conversion in both directions.
- Added conservative INI/Properties detection, formatting and JSON conversion
  that preserves all values as strings and rejects duplicate keys.
- Added HTML Entity and Hex encode/decode tools plus guarded Unix timestamp
  conversion for exact 10/13 digit candidates.
- Added native file open/export actions and single-file drag-and-drop with a
  10 MB text boundary, UTF-8 BOM handling and no recent-file history.

### Fixed

- Prevented unexpected parser exceptions from terminating the CLI, while keeping
  its fallback diagnostic free of input fragments; added malformed-input coverage across all processors
  and a subprocess regression for formerly fatal TOML table headers.
- Made package validation reject artifacts without their SHA-256 sidecar and
  expanded repository publication checks to cover current docs, screenshots and licenses.
- Cleared and repainted the complete retained UI surface after layered state
  changes, preventing stale vertical blocks after closing selectors or switching formats.
- Kept the compiled Mach-O deployment target aligned with the declared macOS
  12.0 minimum instead of producing a current-SDK-only binary with a misleading plist.
- Hardened in-memory HMAC key cleanup with non-elidable zeroing across key
  replacement, encoded-key temporaries, input composition and undo/redo snapshots;
  the global Clear action no longer discards the key length before wiping it.
- Prevented CSV table results from being mistaken for an uninitialized view and
  launching another full analysis on every composed frame.
- Parsed explicitly selected CSV input/delimiters only once, omitted the unused
  desktop TSV duplicate and replaced stream-based TSV generation with reserved
  linear output construction.
- Removed a full-input temporary allocation from timestamp action availability
  checks by parsing bounded `string_view` data directly.
- Avoided building a redundant full highlighted text document for CSV results;
  the virtualized table is now the only presentation model retained.
- Reduced large-result peak memory by moving processing/CSV models and removing
  the duplicate result string retained beside the read-only document.
- Prevented synchronous clipboard and file operations from propagating allocation,
  dialog or I/O exceptions through the desktop main loop.
- Avoided copying the complete read-only document before clipboard copy or export.
- Paused automatic processing above 10 MiB, replacing the expensive editable input
  surface with a lightweight size notice and an explicit "Process anyway" action.
- Kept 1-10 MiB desktop input responsive with a UTF-8-safe 16 KiB paged editor;
  each page retains native selection, IME and undo while processing uses all bytes.
- Added a dedicated full-input copy action to the paged editor, keeping ordinary
  selection copy explicitly page-local.
- Bounded rich-text measurement and drawing for pathological multi-megabyte lines;
  the underlying document, search model and clipboard/export content remain complete.
- Preserved source content kinds across format conversions, including JSON-to-CSV
  table presentation, instead of overwriting them with the output kind.
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

[Unreleased]: https://github.com/cubehead/zeus_tools/compare/v0.2.0...HEAD
[0.2.0]: https://github.com/cubehead/zeus_tools/releases/tag/v0.2.0
[0.1.0]: https://github.com/cubehead/zeus_tools/releases/tag/v0.1.0
