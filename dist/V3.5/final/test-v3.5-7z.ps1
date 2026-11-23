$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$roms = 'D:\2026work\LzmaCApi\forTest'
$sevenZip = Join-Path $env:ProgramFiles '7-Zip\7z.exe'
$debugExe = Join-Path $root 'build-support\test\smolnes-v3.5-7z-debug.exe'
$releaseExe = Join-Path $root 'build-support\test\smolnes-v3.5-7z-release-test.exe'
$testTemp = Join-Path ([IO.Path]::GetTempPath()) ('smolnes-v3.5-7z-' + [guid]::NewGuid().ToString('N'))
$extractRoot = Join-Path $testTemp 'extracted'
$inputRoot = Join-Path $testTemp 'inputs'

& (Join-Path $root 'build-v3.5-7z-debug.cmd')
if ($LASTEXITCODE -ne 0) { throw 'Debug validation executable build failed.' }
if (-not (Test-Path -LiteralPath $sevenZip)) { throw "7z.exe not found: $sevenZip" }
New-Item -ItemType Directory -Force -Path $extractRoot, $inputRoot | Out-Null

$results = [Collections.Generic.List[object]]::new()
$index = 0
try {
    foreach ($archive in Get-ChildItem -LiteralPath $roms -Filter '*.7z' -File -Recurse) {
        $index++
        $listing = & $sevenZip l -slt $archive.FullName
        $methodLine = $listing | Where-Object { $_ -match '^Method = ' } | Select-Object -First 1
        if ($methodLine -notmatch '^Method = LZMA(?:$|:|\s)') {
            $results.Add([pscustomobject]@{ Archive=$archive.Name; Debug='SKIP'; Release='SKIP' })
            continue
        }
        $caseDir = Join-Path $extractRoot ("case-{0:D3}" -f $index)
        New-Item -ItemType Directory -Force -Path $caseDir | Out-Null
        & $sevenZip x -y "-o$caseDir" $archive.FullName | Out-Null
        if ($LASTEXITCODE -ne 0) {
            $results.Add([pscustomobject]@{ Archive=$archive.Name; Debug='FAIL extraction'; Release='FAIL extraction' })
            continue
        }
        $members = @(Get-ChildItem -LiteralPath $caseDir -File -Recurse)
        if ($members.Count -ne 1) {
            $results.Add([pscustomobject]@{ Archive=$archive.Name; Debug='SKIP'; Release='SKIP' })
            continue
        }
        $testArchive = Join-Path $inputRoot ("archive-{0:D3}.7z" -f $index)
        Copy-Item -LiteralPath $archive.FullName -Destination $testArchive -Force
        $start = [Diagnostics.ProcessStartInfo]::new()
        $start.FileName = $debugExe
        $start.Arguments = $testArchive
        $start.UseShellExecute = $false
        $process = [Diagnostics.Process]::Start($start)
        $process.WaitForExit()
        $debugResult = if ($process.ExitCode -eq 0) { 'PASS' } else { "FAIL exit $($process.ExitCode)" }
        $start.FileName = $releaseExe
        $process = [Diagnostics.Process]::Start($start)
        $process.WaitForExit()
        $releaseResult = if ($process.ExitCode -eq 0) { 'PASS' } else { "FAIL exit $($process.ExitCode)" }
        $results.Add([pscustomobject]@{ Archive=$archive.Name; Debug=$debugResult; Release=$releaseResult })
    }
} finally {
    Remove-Item -LiteralPath $testTemp -Recurse -Force
}

$report = Join-Path $root 'results\7z-validation-results.csv'
$results | Export-Csv -LiteralPath $report -NoTypeInformation -Encoding UTF8
$results | Group-Object Debug,Release | Select-Object Name,Count | Format-Table -AutoSize
Write-Output "Report: $report"
if (@($results | Where-Object { $_.Debug -like 'FAIL*' -or $_.Release -like 'FAIL*' }).Count) {
    throw 'At least one archive failed debug preflight or decompression.'
}
