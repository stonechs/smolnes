param(
    [Parameter(Mandatory = $true)][string]$ExePath,
    [Parameter(Mandatory = $true)][string]$RomPath,
    [Parameter(Mandatory = $true)][string]$LogPath,
    [switch]$MissingRom,
    [switch]$VerifyPixels,
    [long]$ExpectedPixel = -1
)

$ErrorActionPreference = 'Stop'
$exe = (Resolve-Path -LiteralPath $ExePath).Path
$rom = [IO.Path]::GetFullPath($RomPath)
$log = [IO.Path]::GetFullPath($LogPath)
$logDir = Split-Path -Parent $log
New-Item -ItemType Directory -Path $logDir -Force | Out-Null

if (-not ('SmokeWin32' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class SmokeWin32 {
    [DllImport("user32.dll")] public static extern IntPtr GetDC(IntPtr hwnd);
    [DllImport("user32.dll")] public static extern int ReleaseDC(IntPtr hwnd, IntPtr hdc);
    [DllImport("gdi32.dll")] public static extern uint GetPixel(IntPtr hdc, int x, int y);
    [DllImport("user32.dll", CharSet=CharSet.Ansi)] public static extern IntPtr FindWindowA(string className, string windowName);
    [DllImport("user32.dll")] public static extern bool PostMessageA(IntPtr hwnd, uint message, IntPtr wParam, IntPtr lParam);
}
'@
}

if (-not $MissingRom -and -not (Test-Path -LiteralPath $rom)) {
    $romDir = Split-Path -Parent $rom
    New-Item -ItemType Directory -Path $romDir -Force | Out-Null
    [byte[]]$image = New-Object byte[] (16 + 16384 + 8192)
    $image[0] = 0x4E; $image[1] = 0x45; $image[2] = 0x53; $image[3] = 0x1A
    $image[4] = 1; $image[5] = 1
    [byte[]]$program = @(
        0xA9, 0x3F, 0x8D, 0x06, 0x20,
        0xA9, 0x00, 0x8D, 0x06, 0x20,
        0xA9, 0x01, 0x8D, 0x07, 0x20,
        0xA9, 0x18, 0x8D, 0x01, 0x20,
        0x4C, 0x14, 0x80
    )
    [Array]::Copy($program, 0, $image, 16, $program.Length)
    $image[16 + 0x3FFC] = 0x00; $image[16 + 0x3FFD] = 0x80
    [IO.File]::WriteAllBytes($rom, $image)
}

$runRom = if ($MissingRom) { Join-Path (Split-Path -Parent $rom) 'missing-smoke-rom.nes' } else { $rom }
if ($MissingRom -and (Test-Path -LiteralPath $runRom)) { Remove-Item -LiteralPath $runRom }
$start = [Diagnostics.ProcessStartInfo]::new()
$start.FileName = $exe
$start.Arguments = '"' + $runRom.Replace('"', '\"') + '"'
$start.WorkingDirectory = Split-Path -Parent $exe
$start.UseShellExecute = $false
$process = [Diagnostics.Process]::Start($start)
$lines = [Collections.Generic.List[string]]::new()
$lines.Add("exe=$exe")
$lines.Add("rom=$runRom")

try {
    $deadline = [DateTime]::UtcNow.AddSeconds(12)
    do {
        $process.Refresh()
        if ($process.HasExited) { break }
        if ($process.MainWindowHandle -ne [IntPtr]::Zero) { break }
        Start-Sleep -Milliseconds 150
    } while ([DateTime]::UtcNow -lt $deadline)

    $process.Refresh()
    if ($MissingRom) {
        if (-not $process.HasExited) {
            $dialog = [SmokeWin32]::FindWindowA('#32770', 'Could not open ROM')
            if ($dialog -ne [IntPtr]::Zero) {
                [void][SmokeWin32]::PostMessageA($dialog, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero)
                $lines.Add('error_dialog=detected_and_dismissed')
            } elseif ($process.MainWindowHandle -ne [IntPtr]::Zero) {
                [void]$process.CloseMainWindow()
            }
            if (-not $process.WaitForExit(5000)) { Stop-Process -Id $process.Id -Force }
            $process.Refresh()
            if (-not $process.HasExited) { throw 'Missing-ROM run stayed alive; expected a prompt nonzero exit.' }
            if ($process.ExitCode -eq 0) { throw 'Missing-ROM run returned exit code 0.' }
            $lines.Add("missing_rom_exit=$($process.ExitCode)")
            $lines.Add('result=PASS')
        } else {
            if ($process.ExitCode -eq 0) { throw 'Missing-ROM run returned exit code 0.' }
            $lines.Add("missing_rom_exit=$($process.ExitCode)")
            $lines.Add('result=PASS')
        }
    } else {
        if ($process.HasExited) { throw "Emulator exited before opening a window (exit $($process.ExitCode))." }
        if ($process.MainWindowHandle -eq [IntPtr]::Zero) { throw 'Emulator did not create a top-level window within 12 seconds.' }
        $lines.Add("window_title=$($process.MainWindowTitle)")
        $lines.Add("window_handle=$($process.MainWindowHandle)")
        if ($VerifyPixels) {
            Start-Sleep -Milliseconds 250
            $dc = [SmokeWin32]::GetDC($process.MainWindowHandle)
            if ($dc -eq [IntPtr]::Zero) { throw 'GetDC failed for the emulator window.' }
            try { $pixel = [SmokeWin32]::GetPixel($dc, 512, 420) }
            finally { [void][SmokeWin32]::ReleaseDC($process.MainWindowHandle, $dc) }
            if ($pixel -eq [uint32]::MaxValue) { throw 'GetPixel failed at the center of the emulator window.' }
            if ($pixel -eq 0) { throw 'The center pixel is black; expected the smoke ROM framebuffer to be painted.' }
            if ($ExpectedPixel -ge 0 -and $pixel -ne [uint32]$ExpectedPixel) {
                throw ('Unexpected center pixel: got 0x{0:X6}, expected 0x{1:X6}.' -f $pixel, $ExpectedPixel)
            }
            $lines.Add(('center_pixel=0x{0:X6}' -f $pixel))
        }
        if (-not $process.CloseMainWindow()) { throw 'Could not send WM_CLOSE through CloseMainWindow().' }
        if (-not $process.WaitForExit(5000)) { throw 'Emulator did not exit after WM_CLOSE within 5 seconds.' }
        $lines.Add("exit_code=$($process.ExitCode)")
        if ($process.ExitCode -ne 0) { throw "Emulator exited with code $($process.ExitCode)." }
        $lines.Add('result=PASS')
    }
} catch {
    $lines.Add('result=FAIL')
    $lines.Add("error=$($_.Exception.Message)")
    if ($process -and -not $process.HasExited) { Stop-Process -Id $process.Id -Force }
    $lines | Set-Content -LiteralPath $log -Encoding utf8
    throw
} finally {
    if ($process) { $process.Dispose() }
}

$lines | Set-Content -LiteralPath $log -Encoding utf8
$lines | ForEach-Object { Write-Output $_ }
