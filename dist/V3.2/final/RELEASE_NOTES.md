# V3.2 release notes

- Replaced the application-created audio worker thread and event wakeup with WinMM `CALLBACK_WINDOW` notifications. The existing window procedure handles `MM_WOM_DONE`, renders the completed 10 ms block, and requeues it.
- Removed audio-only thread, event, critical-section, and completion/underrun diagnostic state.
- Preserved the 48 kHz stereo output, six-block/60 ms queue, PCM synthesis and mixing, APU behavior, controls, ROM selection, and V1.7-style dual-timer pacing.
- Built with Clang/LLVM 23.1.5 size-oriented options. x86 uses Crinkler; x64 uses UPX.

ROM paths with spaces, quoted command-line arguments, and multiple arguments remain unsupported. No ROM is bundled. Runtime audio scheduling and gameplay have not been tested in this build; the window-message refill path requires runtime listening/verification.
