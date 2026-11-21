# smolnes V2 音频移植计划

## 目标与参考范围

V2 以 jsnes-lite 的 `build-support/upstream/src/papu.js` 为参照，先移植它的“振荡器 + 增益”简化发声方式，不把它描述成完整硬件级 APU。该源码直接创建浏览器 `AudioContext`、`OscillatorNode` 和 `GainNode`，由 CPU 对 `$4000–$4013` 的寄存器写入更新频率或音量。

原始实现当前能听到的声源是两个方波 pulse、一个 triangle、一个基于 `createPeriodicWave` 的 noise。`$4010–$4013` 的 DMC 函数和 `$4017` 帧计数器是空实现；pulse duty、envelope、sweep、length、triangle 控制、noise mode/length 等也没有完整实现。因此 V2 第一目标定为四路振荡器可由游戏寄存器驱动、稳定播放；DMC 和完整 APU 时序列入后续增强，不纳入本轮验收。

## 参考代码检查

- 原始实现使用 pulse 频率 `111860.8/(timer+1)`、triangle 频率 `55930.4/(timer+1)`；noise 使用 16 项 period 表。
- 每个声源有各自的 `AudioContext`/振荡器/增益节点，由浏览器音频引擎连续输出。它没有在 JavaScript 中按 CPU 周期计算 PCM，也没有 NES 的非线性混音器或模拟滤波器。
- 原文件有明显未完成/需修正处：`$4015` 状态值表达式需核对运算符优先级，`a5/o5/g5` 没有实际声源，部分寄存器处理空缺，噪声 PeriodicWave 用随机系数。这些不应原样带入 C。
- `src/papu.js` 是后续桥接完整 `JSNESPAPU` 的另一份实现；其 APU 内核位于 `src/jsnes-apu/`。`publish-repo/src/papu.js` 又是后续简化实现。它们只作对照，本计划以 `build-support/upstream/src/papu.js` 为主。

## Windows C 对应实现

Windows 原生 C 没有浏览器 `OscillatorNode` 这种高层通道对象。对应方式是软件振荡器：每声道保存相位、频率、增益、波形类型和启用状态；按输出采样率推进相位，pulse 生成方波、triangle 生成三角波、noise 生成确定性的 LFSR/周期序列；各声道求和、限幅后形成 PCM。

寄存器写入只更新振荡器参数，不在 CPU 指令执行期间绘制波形。独立音频线程连续生成 PCM 并送往声卡，避免每帧只提交一次短音频块造成卡顿。频率或增益改变时做短斜坡，降低参数突变带来的爆音。

输出后端首选 Windows 自带 `waveOut`（`winmm.dll`），使用 48 kHz、16-bit PCM、双声道。waveOut 只传输 PCM，本身不提供 NES 振荡器。实测初始 4 个 10 ms 的 `WAVEHDR` 缓冲在 60 秒内出现 3 次队列耗尽，所以提高为 6 个缓冲块（约 60 ms 已提交音频）；音频线程等待已完成块并补入 PCM，欠载时补静音并记录计数，不能阻塞模拟主线程。它不需要第三方 DLL、适合现有 GCC/Win32 构建；若之后测得延迟过高，再评估共享模式事件驱动 WASAPI。

当前 V1.2 视频循环按固定 60 Hz 限帧，而 NTSC 实际刷新率约 60.1 Hz。音频播放线程独立按声卡时钟消费数据，测试中要记录 PCM 队列水位，确保不会长期欠载/溢出；不要把固定每帧样本数当成时钟方案。

## 技术社区调研结论

- NESdev 论坛和 Wiki 讨论的是完整 APU：帧计数器独立于 PPU VBlank，负责 envelope、length、sweep、triangle linear counter；DMC 涉及内存读取、DMA/CPU 停顿和 IRQ。若追求兼容性，需继续按 CPU/APU 时钟实现这些硬件行为。
- r/EmuDev 的采样讨论指出，固定每 N 个 CPU 周期输出一个样本会带来采样率误差；自行生成 PCM 时应使用相位累积，并把播放空隙先视为供样/缓冲问题，而不是滤波问题。
- 因此本轮沿用原始实现的振荡器抽象，同时让 Windows 音频设备持续消费 PCM；完整硬件精度作为明确的后续版本工作，不与原始参考的能力混为一谈。

## 实施阶段

### 阶段 1：Windows 振荡器与播放后端

- 新增 `audio_win32.c/.h`：waveOut 初始化、PCM 缓冲块、音频线程、静音回退、欠载计数和安全关闭。
- 用独立相位累加器产生固定音高方波、三角波和噪声；支持调频、调增益和组合声道。
- 测试设备启动/关闭、持续播放、削波、爆音、队列欠载和缓冲延迟；完成后做 EXE 冒烟测试并归档源码、脚本与结果。

**执行记录（2026-11-15）：** 已完成独立 `audio_win32.c/.h` 与合成测试程序。使用 WinLibs MinGW-w64 GCC 16.2.0 构建；waveOut 实机冒烟运行约 5 秒，完成 499 个 10 ms PCM 块、写入失败 0，正常退出。归档位于 `archive/V2-audio/stage-01-windows-audio/`。该轮验证了播放设备/队列线程，不代表已完成声学回环测量或游戏寄存器接入。

### 阶段 2：NES 寄存器映射

- 新增 `apu.c/.h` 保存四个声道状态，并接入 `$4000–$4013` 写入、`$4015` 声道启停/状态读取。
- 按原实现支持 pulse timer、triangle timer、volume、noise period；修正计时器高低位拼接和越界值。
- 以合成 ROM/寄存器序列验证四个声道音高、音量、静音/启用和状态位；标注 DMC、`$4017`、envelope/sweep/length 等尚未实现。阶段完成后冒烟测试并归档。

**构建发现：** 目前 GCC 16.2.0 下，给含有 1 MiB 零初始化 ROM 缓冲区的主程序启用 `-fdata-sections` 会令输出 EXE 膨胀到约 1.2 MiB；移除该选项后约 32 KiB。因此 V2 发布构建保留 `-ffunction-sections` 和链接回收，不启用 `-fdata-sections`。

**阶段执行记录（2026-11-15）：** 合成寄存器测试通过：复位状态 `0x00`、启用读回 `0x0f`、关闭部分声道后读回 `0x05`；连续完成 500 个音频块，waveOut 写入失败 0。集成版 `pig.NES` 窗口/像素 smoke 通过（中心像素 `0x8C0000`），合成 APU ROM 窗口启动和关闭通过。阶段快照位于 `archive/V2-audio/stage-02-apu-registers/`。测试验证了 APU 总线接线和窗口生命周期，未做声卡回录或主观音质测量。

### 阶段 3：稳定混音与长时间播放

- 实现四路振荡器线性求和、总增益、限幅与平滑参数变化；确保相位跨音频块连续。
- 测量频率误差、峰值/RMS、点击声、PCM 队列水位及欠载/溢出；持续运行观察 A/V 节拍。真实 NES 非线性混音、模拟滤波和完整 LFSR 参数作为后续音质/精度增强项。
- 完成后冒烟测试并归档。

**阶段执行记录（2026-11-15）：** 纯软件波形测试通过：方波 440 Hz 计到 439 次正向穿越，三角波计到 440 次；噪声窗口出现 37 次符号变化；四声道高增益混音触发限幅，440→880 Hz 调整时首个样本差为 8。waveOut APU 测试完成 497 个缓冲块、写入失败 0；集成版 `pig.NES` 窗口/像素 smoke 通过（`0x8C0000`）。快照位于 `archive/V2-audio/stage-03-mix-and-clock/`。未做声卡回录、长期多小时漂移或人工听感评分。

### 阶段 4：整合、ROM 冒烟和交付

- 整合 CPU 内存总线、窗口消息循环、重置与退出清理；音频线程不能阻塞键盘/窗口响应。
- 使用可再分发测试 ROM 和用户本地 `pig.NES` 验证游戏发声与播放稳定性；商业 ROM 不复制进仓库或快照。
- 对照 V1.2 测量每帧 CPU/PPU 工作时间、音频欠载计数和延迟，完成后归档 V2 候选快照及交付件。

**阶段执行记录（2026-11-15）：** 将 waveOut 提交队列调整为 6×10 ms 后，60 秒 `pig.NES` 运行通过：6039 个音频缓冲块、欠载 0、正常关闭。该诊断构建平均整机归一化进程 CPU 1.88%，每帧 CPU/PPU/GDI 主线程工作平均 4.666 ms（分组峰值中的最大单帧 9.848 ms），GDI 平均 1.616 ms。已有 V1.2 性能 CSV 的对应平均值为 3.344 ms 和 GDI 1.265 ms；采样不在同一轮 A/B 条件下，差异只作参考，不据此认定是音频代码造成。名义提交缓冲量约 60 ms，未做声卡回录延迟测量。

无诊断候选 `dist/V2/smolnes-v2.exe` 已构建，32,768 字节，SHA-256 `5B75EAA16E245DAB92F98271860227712E99414F258ED4713A60198449C95757`。最终 `pig.NES` 画面冒烟（中心像素 `0x8C0000`）和合成 APU ROM 启动/关闭均通过。阶段快照在 `archive/V2-audio/stage-04-v2-candidate/`。

首轮 V2 候选曾整理在 `dist/V2/smolnes-v2-package/`（32,768 字节候选）并生成 source ZIP；该中间包已按清理决定移除。可复现的 V2 最终快照保存在 `archive/V2-final/`，正式交付件位于 `dist/V2/`。

## V2 首轮验收条件

1. 无第三方运行库，只依赖 Windows 系统组件并链接 `winmm`。
2. 两个 pulse、triangle、noise 四路振荡器可由 ROM 写寄存器驱动，音量/音高/启停正常；状态读回采用原型级 enable bits。
3. 连续运行无周期性欠载/溢出，静音回退有效，窗口关闭后线程和句柄正常释放。
4. 清楚记录本轮未实现项：DMC、完整 `$4017` 帧计数器、pulse duty/envelope/sweep/length counter、triangle linear counter、noise mode/length counter 和 NES 非线性混音/模拟滤波。

## 来源

- jsnes-lite 原始振荡器实现：`build-support/upstream/src/papu.js`
- jsnes-lite 后续完整 APU：`src/jsnes-apu/index.js`
- [NESdev: How to emulate the APU](https://forums.nesdev.org/viewtopic.php?t=2123)
- [NESdev: Some questions about APU clocking](https://forums.nesdev.org/viewtopic.php?t=25452)
- [NESdev Wiki: APU registers](https://www.nesdev.org/wiki/APU_registers)
- [NESdev Wiki: Catch-up](https://www.nesdev.org/wiki/Catch-up)
- [r/EmuDev: NES APU filters, mixing and sampling](https://www.reddit.com/r/EmuDev/comments/1fxbpxz/nes_apu_questions_about_filters_mixing_and/)
- [Microsoft Learn: `waveOutOpen`](https://learn.microsoft.com/en-us/previous-versions/dd743866%28v%3Dvs.85%29)
- [Microsoft Learn: WASAPI overview](https://learn.microsoft.com/en-us/windows/win32/coreaudio/wasapi)

## 阶段 5：按 `src/papu.js` 补齐 APU 控制单元（2026-11-15）

- Pulse：实现四档 duty、包络/constant volume、sweep、长度计数器。
- Triangle：实现长度计数器和 linear counter。
- Noise：实现包络、长度计数器、period 表和长/短 LFSR 模式。
- APU：新增基于累计 CPU 指令周期的帧序列器、quarter/half-frame 时钟、`$4015` 长度状态/IRQ 读清除，以及 `$4017` 四步/五步模式和 IRQ inhibit。
- 音频采样线程继续生成 PCM；APU 状态变化更新声道控制。噪声 LFSR 按 CPU 周期率推进，duty 和 noise mode 由声道采样器读取。
- DMC、精确到总线读写周期的 APU 相位、NES 非线性混音器和模拟滤波仍未实现。
- 验证：合成 APU 测试通过，帧序列运行 29,830 CPU 周期后长度计数归零并报告/清除 frame IRQ；waveOut 约 5 秒完成 499 个缓冲块，欠载 0。实际 `dist/V2/魂斗罗.nes` 的窗口客户区直接采样到多种非黑像素；桌面 `CopyFromScreen` 截图在 Codex 会话中返回黑色，不能代表窗口内容。用户确认当前画面可见且声音正常。此前误用的 `roms/commercial/魂斗罗.nes` 是旧冒烟脚本在文件缺失时生成的合成 ROM，已删除；冒烟脚本现对缺失 ROM 报错，不再自动写入调用者指定的路径。
- 候选文件：`dist/V2/stage-05-apu-core/`；快照：`archive/V2-audio/stage-05-apu-core/`。

### 阶段 5 回归修正：Pulse 1 负向 Sweep 下溢（2026-11-15）

- `pig.NES` 实际写入 `$4001 = $08`，其 Pulse 1 sweep target 按参考代码为 `-1`。原 C 版以无符号整数保存，导致下溢成大正数，并被错误判为超出 `$7FF` 而静音。
- 改为有符号 target，负值按参考代码不触发“大于 `$7FF`”的静音条件；只有合法的 0..$7FF 目标参与 sweep 更新。
- 另修正 Noise mode 位检查：`$400E` bit 7 对应短模式，而非 bit 8。
- 回归测试用 `$4001=$08`、timer `$01F1` 验证 Pulse 1 约 224 Hz、正负波形均存在；Duty 与长短噪声模式波形测试通过。`pig.NES` 窗口冒烟通过。
- 修正版可执行文件：`dist/V2/stage-05-apu-core/smolnes-v2-apu-core-fixed.exe`；归档快照：`archive/V2-audio/stage-05-pulse-sweep-fix/`。

## 阶段 6：对齐 JSNES-lite 增益响应（2026-11-15）

- `papu.js` 的 `GainNode.setTargetAtTime` 使用 1 ms 时间常数；C 版原先每采样按 0.002 收敛，约为 10 ms。改为每采样系数 0.0206（48 kHz 下约 1 ms），让起音和包络变化更接近参考实现。
- 验证：方波/三角波频率、duty 占空比、噪声模式和 Pulse 1 sweep 回归测试通过；`pig.NES` 窗口网格冒烟通过；3 秒 waveOut 播放完成 300 个缓冲块，欠载 0。
- 用户反馈总体声音基本正常，但仍能听出与 jsnes-lite 不同。剩余显著实现差异是 C 方波/三角波没有 Web Audio 的带限处理，且 C 音频按 10 ms 块读取共享状态、维持约 60 ms 播放队列，JS 则按 APU 时间戳安排 AudioParam 更新。
- 候选文件：`dist/V2/stage-05-apu-core/smolnes-v2-apu-jsneslike.exe`；归档：`archive/V2-audio/stage-06-jsnes-response/`。
- 用户已决定 V2 封版；PolyBLEP/带限波形与按 APU 时间戳处理寄存器事件列为封版后的遗留工作，不进入 V2 发布范围。
