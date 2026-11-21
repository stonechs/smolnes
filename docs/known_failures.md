# 已知失败与复现记录

## V3.3 WinMM 欠载诊断候选启动崩溃

- 日期：2026-11-21
- 复现文件：`dist/experiments/v3.3-winmm-underrun-diagnostics/release/smolnes-v3.3-x86-winmm-diag.exe`
- 步骤：通过普通桌面启动程序，在文件选择框中打开《魂斗罗》ROM。
- 结果：`smolnes` 窗口短暂出现后退出。Windows Application Error 事件 1000 记录 `0xc0000005` 访问冲突，故障模块为 `ntdll.dll`，故障偏移 `0x0005f633`。
- 影响：未找到 `audio-queue-diagnostics.csv`；没有获得 WinMM 完成间隔、队列估计或 `waveOutWrite` 错误数据，不能判断实时欠载是否导致音量变化。
- 分析：此程序使用自定义入口点。首个诊断版本在 WinMM 路径新增 `fopen/fprintf`，可能在标准 CRT 初始化前调用，故此为当前崩溃假设；没有调用栈，尚未证实。
- 后续：已将日志改为 Win32 文件 API、CSV 路径固定为 EXE 同目录并重新构建。新版尚未运行；由用户手动运行并回传 CSV，以同时确认启动和采集。
