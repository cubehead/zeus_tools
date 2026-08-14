# Changelog

All notable user-visible changes will be documented here. This project follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and intends to use
semantic versioning after the first stable release.

## [Unreleased]

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

### Security

- XML DTD/entity rejection, constrained YAML parsing and local-only processing.
