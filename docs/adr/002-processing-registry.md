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

Zeus Tools uses a statically linked processing registry. The core processor definition owns
the stable ID for each processing mode. Each application action references that mode and has:

- its applicable input kind;
- presentation metadata for the compact action bar;
- optional applicability and behavior flags.

The application derives stable IDs such as `json.to_toml` and `base64.encode` from the core
mode registration instead of storing a second copy. Context-sensitive automatic actions share
the reserved `auto` ID and remain distinguishable by input kind.

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
- Core execution groups registered handlers by real behavior boundaries: structured-format
  processing, format conversion, codecs, and text transforms. `process_text()` performs a
  registry lookup instead of maintaining another global action switch.
- Automatic detection remains a separate ordered pipeline because detector priority and
  conservative fall-through are different concerns from explicit action dispatch.
- A manual input override executes its registered processor directly. Automatic detection is
  only run for the `auto` input type, avoiding a redundant full parse of large inputs.
- Compile-time validation requires every non-Auto `ProcessingMode` to be registered exactly
  once with a unique, non-empty ID; a missing or duplicate handler or ID fails the build.
- Application content metadata registers the full name, compact action-bar name, result
  syntax, and default export extension for every `ContentKind`. Input-type labels, result
  document construction, and export suggestions use this single definition.
- Content metadata is also compile-time ordered and complete; adding a `ContentKind` without
  its definition fails the build.
- `ProcessResult` keeps the detected source kind separate from the output kind. This allows a
  Base64 or URL source that decodes to JSON, and a JWT inspection that produces JSON, to retain
  source-aware actions while selecting JSON highlighting and export metadata for the result.
- Result presentation is derived from the registered output kind. Redundant `structured` and
  `tabular` flags are intentionally omitted so a result cannot claim conflicting presentation
  states; JSON/XML/YAML/TOML syntax and CSV tables follow their content definitions.
- CSV controls, decode-again, and digest/HMAC remain explicit UI features because they carry
  state or interaction beyond a single text transformation.

## Rejected alternative

Runtime binary plugins were rejected because they add ABI, dependency, signing, security,
and crash-isolation concerns without helping the current goal of organizing built-in code.
