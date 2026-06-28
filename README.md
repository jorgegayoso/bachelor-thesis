# Static Intra-Object Overflow Detection with CodeQL

CodeQL queries for detecting intra-object overflows (IOOs) in C code, developed as part of a BSc thesis at Vrije Universiteit Amsterdam.

## What is an intra-object overflow?

An IOO occurs when a `memcpy` or `memset` writes past the boundary of a struct field while staying within the struct's allocation. Tools like AddressSanitizer miss these because the memory is technically "valid."

## Repository contents

- **`queries/`** — Four CodeQL queries:
  - `pattern-memcpy.ql` — fine-grained: both destination and source must be struct fields
  - `pattern-memcpy-broad.ql` — broad: only one side must be a struct field
  - `pattern-memset.ql` — fine-grained: size expression must contain `sizeof(parent_struct)`
  - `pattern-memset-broad.ql` — broad: any constant size exceeding the field
- **`patches/`** — 15 minimal C reproduction scripts derived from Kees Cook's FORTIFY_SOURCE patch series
- **`results/`** — SARIF output from running the queries against the Linux kernel

## Quick start

```bash
# Build a CodeQL database for any C project
codeql database create my-db --language=cpp --command="make -j$(nproc)"

# Run all four queries
codeql database analyze my-db \
  queries/pattern-memcpy.ql \
  queries/pattern-memcpy-broad.ql \
  queries/pattern-memset.ql \
  queries/pattern-memset-broad.ql \
  --format=sarif-latest \
  --output=results.sarif \
  --rerun
```

## Results on the Linux kernel

Tested against commit [`093a0bea`](https://github.com/torvalds/linux/tree/093a0bea629ac8e4f2ee9a3ffc30817139452937) (pre-Kees Cook patches):

| | Memcpy | Memset | Total |
|---|--------|--------|-------|
| Fine-grained hits | 9 | 2 | 11 |
| Broad-only hits | 7 | 3 | 10 |
| **Total** | **16** | **5** | **21** |

All 21 hits are true positives at the type level. 20 are intentional multi-field operations; 1 is a pre-C99 flexible array member hack (`byte_data[1]`). Full triage details are in the thesis.

## Requirements

- CodeQL CLI (v2.x)
- A C project that compiles with `make` or `gcc`

## License

MIT

## Citation

If you use these queries in your research, please cite the thesis:

> Jorge Gayoso de los Ríos. "Static and LLM-Based Intra-Object Overflow Detection." BSc Thesis, Vrije Universiteit Amsterdam, 2026.
