# Privacy

Zeus Tools is designed for local-only processing.

## Data handling

- Input text, formatted output, CSV cells, search terms and HMAC keys remain in
  the application process.
- The application does not upload content, create an account, collect
  telemetry or retain processing history.
- Closing the application releases in-memory content and HMAC keys.
- Only theme and language preferences are written to the current user's
  application settings directory.

## Network access

The built application does not require network access. A source build may use
CMake FetchContent during initial configuration to download pinned open-source
dependencies. This build-time activity does not include user content.

## Logs and crash reports

Zeus Tools does not intentionally log input, output, search content or keys.
Official releases must not add such logging. Operating-system crash reports
may contain generic process metadata; users should review reports before
sharing them publicly.

## Security reports

Do not include private payloads, credentials or keys in a public issue. Follow
the private reporting guidance in [SECURITY.md](SECURITY.md).
