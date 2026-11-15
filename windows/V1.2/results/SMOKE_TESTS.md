# V1.2 verification record

- `pig.NES` smoke check: PASS (window created, center pixel `0x8C0000`, normal exit). The commercial ROM is not included.
- `sprite-limit.nes`: PASS (ninth in-range sprite excluded; sampled background pixel `0x636163`).
- `sprite-row-cache.nes`: PASS (sampled sprite pixel `0xFF9A63`, matching the pre-stage-3 rendering).
- Final release executable: 29,184 bytes; SHA-256 is listed in `SHA256SUMS.txt`.
- Import table: no SDL import. Imports Win32 GDI/User32/Kernel32 and UCRT API-set DLLs.
- Performance comparisons use 600 frames per candidate and five groups of 120 frames. See the accompanying CSV files and migration notes.
