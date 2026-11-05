# Benchmarks

The benchmark target is intentionally small and separate from the normal test
suite. It provides local sanity checks for regressions in the query and result
pipeline; it is not a CI gate and does not make universal performance claims.

## Building and running

```sh
meson setup build --buildtype=release
meson compile -C build benchmark-core
./build/benchmark-core
```

## Methodology

The benchmark uses fixed synthetic inputs:

- 10 representative queries for query analysis.
- 200,000 iterations for query analysis.
- 200,000 iterations for application matching using a fixed application record.
- 200,000 iterations for file matching using a path containing a space.
- 200,000 iterations for `u_result_display_text()` formatting.
- 50,000 iterations for `u_results_dedup()` over 20 synthetic results.

Application and file matching use the production scoring helpers. The benchmark
does not launch subprocesses, perform network requests, or use the real desktop.

## Reference result

Recorded on:

- Nobara Linux 44 / x86_64
- Linux 7.3.1-201.nobara.fc44.x86_64
- AMD Ryzen 7 5700X, 16 threads
- GCC 16.2.1
- Meson release build (`-O3`, no sanitizers)

One representative run:

```text
query analysis: 200000 iterations in 4779.286 ms (23896.4 ns/op)
app matching:   200000 iterations in 166.544 ms (832.7 ns/op)
file matching:  200000 iterations in 109.293 ms (546.5 ns/op)
display format: 200000 iterations in 8.590 ms (43.0 ns/op)
dedup (20):     50000 iterations in 81.002 ms (1620.0 ns/op)
```

These numbers vary with CPU load, frequency scaling, compiler and build options.

## Interpretation

- Query analysis is the most expensive measured step because it evaluates
  several deterministic query signals.
- Application and file matching are sub-microsecond operations in this dataset.
- Display formatting is allocation-heavy but very small relative to analysis.
- Deduplication is measured for a 20-result set and scales with result count.

No peak-memory measurement is included yet. Memory safety and bounded caches are
covered by the normal tests and sanitizer CI jobs.
