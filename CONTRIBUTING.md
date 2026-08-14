# Contributing

Thanks for helping improve Zeus Tools.

## Before opening an issue

- Search existing issues first.
- Use synthetic input; never post private payloads, credentials or keys.
- Include the operating system, architecture, app version/commit and exact
  reproduction steps.
- For security issues, follow [SECURITY.md](SECURITY.md) instead.

## Development setup

```sh
cmake --preset core
cmake --build --preset core
ctest --preset core
```

For desktop UI work:

```sh
cmake --preset dev
cmake --build --preset dev
```

## Pull requests

- Keep changes focused and explain user-visible behavior.
- Add or update core tests for parsers, converters, detection and crypto.
- Verify dark/light themes and relevant languages for UI changes.
- Do not add telemetry, content history, network processing or GitHub Actions
  without prior maintainer agreement and an architecture decision record.
- Do not commit `build/`, credentials, signing identities or real user data.
- Update documentation when behavior, packaging or privacy boundaries change.

Before submitting:

```sh
./scripts/check-repository.sh
cmake --build --preset core
ctest --preset core
```

Platform-specific changes should also be built on the affected native OS.
