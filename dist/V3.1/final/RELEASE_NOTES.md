# V3.1 release notes

- Updated the frame pacing to V1.7's two periodic waitable timers: 16 ms for the first two frame waits and 50 ms for the third, re-anchoring the 16 ms timer at the 50 ms boundary.
- Simplified ROM command-line handling to accept one simple, unquoted argument without a general argument parser.
- Added the native Windows Open File dialog for launches without a ROM argument; cancel exits normally.
- Built with Clang/LLVM 23.1.5 size-oriented flags. x86 uses Crinkler; x64 uses UPX.
- Retained the V3 Win32/GDI video and WinMM audio implementation.

ROM paths containing spaces or quoted command-line arguments are not supported. No ROM is bundled. Runtime behavior and actual timer wake precision have not been tested for this release.
