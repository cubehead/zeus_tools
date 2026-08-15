# Detection fixtures

`detection-v1.tsv` is the versioned regression corpus for automatic content
detection. Each non-comment row contains:

1. a stable case name;
2. the expected `ContentKind` display name;
3. whether processing should succeed;
4. the input, using `\n`, `\r`, `\t` and `\\` escapes.

Add a focused case whenever a detection false positive, false negative or
format-boundary regression is fixed. Existing rows should not be silently
repurposed; rename the corpus when detection policy changes intentionally.
