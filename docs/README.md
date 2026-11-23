# Project documentation

- [`project_status.md`](project_status.md) summarizes the current version and recent verification.
- [`environment.md`](environment.md) records the verified Windows compiler setup.
- [`known_failures.md`](known_failures.md) records unresolved failures and reproduction notes.
- [`release-workflow.md`](release-workflow.md) describes release and ROM-loading conventions.
- `building/` contains build and artifact guidance.
- `plans/` contains historical porting, renderer, audio, and size-optimization plans.
- `releases/` contains version-specific release notes and usage guides.
- The current Windows source is under `../src/`; versioned builds and release artifacts are under `../dist/`.

The current baseline is V3.5, in `../dist/V3.5/final/`. It adds x86 support for single-file, single-stream LZMA1 `.7z` ROMs; x64 remains `.nes` only.

Historical snapshots under `../archive/` and experiment binaries, captures, and logs under `../dist/experiments/` are local-only and are not included in this repository. Some historical plans and status entries mention those local paths; their summarized findings remain in these documents.
