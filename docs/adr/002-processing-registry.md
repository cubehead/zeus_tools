# ADR 002: Static processing registry

- Status: Accepted
- Date: 2026-08-15

## Context

Processing actions were previously coordinated through integer indexes in application state,
UI conditionals, and switch statements in the processing service. Adding a format or codec
required keeping those separate mappings synchronized. Reordering an action could silently
change the operation executed by an existing index.

The project needs a clearer extension boundary, but it does not need runtime-loaded DLL,
dylib, script, or third-party plugins.

## Decision

Zeus Tools uses a statically linked processing registry. Each registered action has:

- a stable string ID, such as `json.to_toml` or `base64.encode`;
- its applicable input kind;
- its processing mode;
- presentation metadata for the compact action bar;
- optional applicability and behavior flags.

Input overrides are also registered under stable IDs. Application state and background
processing requests store these IDs instead of positional indexes. The action bar and the
processing service query the same registry.

Formatters, codecs, inspectors, CSV presentation, and cryptographic UI remain separate by
their existing functional boundaries. The registry coordinates them; it does not allow a
handler to construct arbitrary UI.

## Consequences

- Action ordering can change without changing behavior or persisted state.
- The UI no longer duplicates the mapping between numeric values and processing modes.
- New built-in operations have one discoverable registration point.
- All handlers remain part of the executable and use the normal build, signing, and test flow.
- `ProcessingMode` remains the internal compatibility contract for core execution; it is no
  longer exposed as a positional UI contract.
- CSV controls, decode-again, and digest/HMAC remain explicit UI features because they carry
  state or interaction beyond a single text transformation.

## Rejected alternative

Runtime binary plugins were rejected because they add ABI, dependency, signing, security,
and crash-isolation concerns without helping the current goal of organizing built-in code.
