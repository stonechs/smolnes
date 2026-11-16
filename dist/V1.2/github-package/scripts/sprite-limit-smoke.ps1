param(
    [Parameter(Mandatory = $true)][string]$ExePath,
    [Parameter(Mandatory = $true)][string]$RomPath,
    [Parameter(Mandatory = $true)][string]$LogPath,
    [long]$ExpectedPixel = -1
)

$ErrorActionPreference = 'Stop'
$exe = (Resolve-Path -LiteralPath $ExePath).Path
$rom = (Resolve-Path -LiteralPath $RomPath).Path
$log = [IO.Path]::GetFullPath($LogPath)
New-Item -ItemType Directory -Path (Split-Path -Parent $log) -Force | Out-Null

if (-not ('SpriteLimitSmokeWin32' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class SpriteLimitSmokeWin32 {
    [DllImport("user32.dll")] public static extern IntPtr GetDC(IntPtr hwnd);
    [DllImport("user32.dll")] public static extern int ReleaseDC(IntPtr hwnd, IntPtr hdc);
    [DllImport("gdi32.dll")] public static extern uint GetPixel(IntPtr hdc, int x, int y);
}
'@
}

$start = [Diagnostics.ProcessStartInfo]::new()
$start.FileName = $exe
$start.Arguments = '"' + $rom.Replace('"', '\"') + '"'
$start.WorkingDirectory = Split-Path -Parent $exe
$start.UseShellExecute = $false
$process = [Diagnostics.Process]::Start($start)
$result = [Collections.Generic.List[string]]::new()
$result.Add("exe=$exe")
$result.Add("rom=$rom")

try {
    $deadline = [DateTime]::UtcNow.AddSeconds(12)
    do {
        $process.Refresh()
        if ($process.HasExited) { throw "Emulator exited during startup ($($process.ExitCode))." }
        if ($process.MainWindowHandle -ne [IntPtr]::Zero) { break }
        Start-Sleep -Milliseconds 100
    } while ([DateTime]::UtcNow -lt $deadline)

    $process.Refresh()
    if ($process.MainWindowHandle -eq [IntPtr]::Zero) { throw 'Emulator did not create a window.' }
    Start-Sleep -Milliseconds 500
    $dc = [SpriteLimitSmokeWin32]::GetDC($process.MainWindowHandle)
    if ($dc -eq [IntPtr]::Zero) { throw 'GetDC failed.' }
    try { $pixel = [SpriteLimitSmokeWin32]::GetPixel($dc, 512, 420) }
    finally { [void][SpriteLimitSmokeWin32]::ReleaseDC($process.MainWindowHandle, $dc) }
    if ($pixel -eq [uint32]::MaxValue) { throw 'GetPixel failed.' }
    $result.Add(('center_pixel=0x{0:X6}' -f $pixel))
    if ($ExpectedPixel -ge 0 -and $pixel -ne [uint32]$ExpectedPixel) {
        throw ('Unexpected center pixel: got 0x{0:X6}, expected 0x{1:X6}.' -f $pixel, [uint32]$ExpectedPixel)
    }

    if (-not $process.CloseMainWindow()) { throw 'Could not send WM_CLOSE.' }
    if (-not $process.WaitForExit(5000)) { throw 'Emulator did not close after WM_CLOSE.' }
    if ($process.ExitCode -ne 0) { throw "Emulator exited with code $($process.ExitCode)." }
    $result.Add('result=PASS')
} catch {
    $result.Add('result=FAIL')
    $result.Add("error=$($_.Exception.Message)")
    if ($process -and -not $process.HasExited) { Stop-Process -Id $process.Id -Force }
    $result | Set-Content -LiteralPath $log -Encoding utf8
    throw
} finally {
    if ($process) { $process.Dispose() }
}

$result | Set-Content -LiteralPath $log -Encoding utf8
$result | ForEach-Object { Write-Output $_ }
