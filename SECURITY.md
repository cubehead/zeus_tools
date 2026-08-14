# Security policy

## Supported versions

The project is currently in alpha. Security fixes are applied to the latest
source revision and the newest published test build only.

## Reporting a vulnerability

Please use GitHub's private vulnerability reporting feature when it is enabled
for the repository. If it is unavailable, contact the repository owner through
their public GitHub profile and request a private reporting channel before
sending technical details.

Do not open a public issue containing:

- credentials, tokens, private keys or HMAC keys;
- private JSON/XML/YAML/CSV payloads;
- a working exploit that exposes local files or sensitive process memory.

Include the affected version/commit, platform, minimal synthetic reproduction,
impact and any suggested mitigation. A maintainer should acknowledge a report
within seven days, but this is a best-effort target while the project is alpha.

## Security boundaries

Zeus Tools processes untrusted text locally. It rejects XML DTD/entity input,
limits YAML complexity, decodes automatically at most one layer, does not
verify JWT signatures and does not persist HMAC keys. MD5 and SHA-1 are exposed
only for compatibility and are labeled as weak algorithms.
