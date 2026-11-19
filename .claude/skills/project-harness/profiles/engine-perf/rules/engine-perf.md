# Engine and performance profile

- Benchmark gate: a change to a hot path needs a feature in `docs/features.json` whose `checked_by` is the benchmark, with before/after numbers (same machine and settings) under the todo's `## Evidence`.
- A change that alters results (not just speed) is size L; record the strength or accuracy test used.
- Keep benchmark scripts and reference data under version control; put raw outputs in `docs/reports/`.
