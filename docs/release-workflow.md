# Release workflow conventions

## ROM loading and automated tests

Keep command-line ROM loading available in future versions: the emulator should accept a ROM path as an argument. This is needed for repeatable automated launch, rendering, and regression checks.

V1.3 is a special-case size experiment that opens the Windows file picker at startup and does not accept a ROM path from the command line. Do not copy that V1.3 behavior into later versions unless the user explicitly requests it.

Automated smoke tests should pass a known test ROM on the command line, verify that the emulator starts and renders, then close it cleanly. Keep runtime validation results separate from static checks such as PE inspection, file hashes, and packer integrity checks.
