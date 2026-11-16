# V3 release notes

V3 is the size-optimized Windows-native build based on the V2-final source.

- Replaced CRT ROM file handling with Win32 file APIs.
- Added a custom process entry and removed MinGW CRT startup/default libraries.
- Linked only the required Windows system libraries and compiler helper library.
- Packed the x64 release EXE with UPX 5.3.0 (`--best --lzma`).
- No SDL runtime or ROMs are bundled.

The release executable is 13,824 bytes (UPX-packed); the uncompressed linked image is 17,920 bytes. `pig.NES` display/close and missing-ROM smoke checks passed. These checks do not establish broad ROM compatibility or subjective audio fidelity.
