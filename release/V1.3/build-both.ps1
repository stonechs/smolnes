$ErrorActionPreference='Stop'
$root=$PSScriptRoot
$source=Join-Path $root 'source\deobfuscated_win32.c'
$common=@('-Oz','-flto','-DNDEBUG','-ffunction-sections','-fno-unwind-tables','-fno-asynchronous-unwind-tables','-DSMOLNES_NO_ERROR_CHECKS')
$targets=@(
  @{Arch='x64';Gcc='C:\Users\maco\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin\gcc.exe';Entry='mainCRTStartup'},
  @{Arch='x86';Gcc='D:\toolsbig\WinLibs-i686-UCRT-GCC16.2.0\toolchain\mingw32\bin\i686-w64-mingw32-gcc.exe';Entry='_mainCRTStartup'}
)
foreach($target in $targets){
  $arch=$target.Arch;$gcc=$target.Gcc;$entry=$target.Entry
  if(!(Test-Path -LiteralPath $gcc)){throw "Compiler not found: $gcc"}
  $object=Join-Path $root ('objects\deobfuscated_win32-'+$arch+'.o')
  $exe=Join-Path $root ('smolnes-v1.3-native-caption-'+$arch+'.exe')
  $map=Join-Path $root ('link-'+$arch+'.map')
  $ld=Join-Path $root ('linker-merge-one-section-'+$arch+'.ld')
  & $gcc @common '-c' $source '-o' $object
  if($LASTEXITCODE -ne 0){throw "Object compile failed: $arch"}
  $link=@($common)+@('-static','-mwindows','-Wl,--gc-sections','-s','-Wl,--no-insert-timestamp','-nostartfiles',('-Wl,-e,'+$entry),'-nostdlib',('-Wl,-T,'+$ld))
  if($arch -eq 'x86'){$link+=@('-Wl,--disable-dynamicbase','-Wl,--disable-reloc-section')}
  $link+=@($object,'-o',$exe,('-Wl,-Map='+$map),'-luser32','-lgdi32','-lkernel32','-lcomdlg32','-lgcc')
  & $gcc @link
  if($LASTEXITCODE -ne 0){throw "Link failed: $arch"}
  # The sole file-backed section carries imports, code, constants and writable data.
  # Grant RWX so all these inputs remain usable in the merged section.
  $bytes=[IO.File]::ReadAllBytes($exe)
  $pe=[BitConverter]::ToInt32($bytes,0x3c)
  if([Text.Encoding]::ASCII.GetString($bytes,$pe,4) -ne "PE`0`0"){throw 'Invalid PE signature'}
  $count=[BitConverter]::ToUInt16($bytes,$pe+6)
  $optional=[BitConverter]::ToUInt16($bytes,$pe+20)
  $headers=$pe+24+$optional
  $found=$false
  for($i=0;$i -lt $count;$i++){
    $sh=$headers+40*$i
    $name=[Text.Encoding]::ASCII.GetString($bytes,$sh,8).Trim([char]0)
    if($name -eq '.idata'){
      $field=$sh+36
      $flags=[BitConverter]::ToUInt32($bytes,$field) -bor 0xA0000020
      [Array]::Copy([BitConverter]::GetBytes($flags),0,$bytes,$field,4)
      $found=$true;break
    }
  }
  if(!$found){throw 'Merged .idata section was not produced'}
  [IO.File]::WriteAllBytes($exe,$bytes)
  $hash=(Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash
  @("Architecture: $arch","Compiler: $((& $gcc --version|Select-Object -First 1))","Target: $((& $gcc -dumpmachine))","Source: $source","Flags: $($common -join ' ')","Link flags: $($link -join ' ')",'Layout: one RWX .idata file-backed section plus zero-fill .bss','Post-link patch: .idata characteristics set to code/read/write/execute',"Object bytes: $((Get-Item -LiteralPath $object).Length)","EXE bytes: $((Get-Item -LiteralPath $exe).Length)","SHA256: $hash")|Set-Content -LiteralPath ($exe+'.build-info.txt') -Encoding utf8
  Get-Item -LiteralPath $exe|Select-Object FullName,Length
  Write-Output "Map: $map"
  Write-Output "SHA256: $hash"
}
