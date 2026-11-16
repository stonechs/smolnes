# Project documentation

- `plans/` contains development and migration plans.
- `releases/` contains version guides that apply to the repository as a whole.

Version-specific release notes, build inputs, and artifacts stay together under
`../dist/`:

- `../dist/V1/`
- `../dist/V1.2/github-package/`
- `../dist/V2/`
- `../dist/V3/final/`

The V1 historical source, build script, and checksum snapshot are kept in `../archive/V1-release/`.


Source layout: original upstream `.c` files are under `../src/org/`; the current V3 Windows sources and headers are under `../src/`.