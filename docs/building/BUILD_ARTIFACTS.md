# Windows build artifacts

For each release build, keep the normal x86 and x64 executables and generate
one packed copy of each:

| Architecture | Additional artifact | Packer |
| --- | --- | --- |
| x86 | `<name>-upack.exe` | `tools\Upack.exe` |
| x64 | `<name>-upx.exe` | `upx.exe --ultra-brute` |

Run the pack step after both uncompressed executables have been built:

```powershell
./scripts/pack-release-variants.ps1 `
  -X86Exe ./dist/<version>/<name>-x86.exe `
  -X64Exe ./dist/<version>/<name>-x64.exe
```

The script packs copies, leaving the original executables unchanged. It checks
the UPX output with `upx -t` and prints byte sizes and SHA-256 hashes for all
four files. The x86 UPack output is preserved as a separate artifact; UPack
0.39 does not provide an equivalent integrity-test command in its displayed
options.

Keep both compressed and uncompressed binaries in the version snapshot. Do
not use the packed binaries as inputs to subsequent development builds.
