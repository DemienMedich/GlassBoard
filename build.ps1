param([string]$QtRoot = 'Z:\Soft\Qt\6.8.3\msvc2022_64', [string]$VcVars = 'Z:\Soft\VC2022\VC\Auxiliary\Build\vcvars64.bat')
$ErrorActionPreference = 'Stop'
$ninja = (Get-Command ninja.exe).Source
$command = 'call "{0}" && cmake -S "{1}" -B "{1}\build" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="{2}" -DCMAKE_MAKE_PROGRAM="{3}" && cmake --build "{1}\build"' -f $VcVars,$PSScriptRoot,$QtRoot,$ninja
cmd /c $command
if ($LASTEXITCODE -ne 0) { throw 'Build failed' }
