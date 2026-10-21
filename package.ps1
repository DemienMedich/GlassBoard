param([string]$QtRoot = 'Z:\Soft\Qt\6.8.3\msvc2022_64')
$ErrorActionPreference = 'Stop'
$cmake = Get-Content "$PSScriptRoot\CMakeLists.txt" -Raw
if ($cmake -notmatch 'project\(GlassBoard VERSION (\d+\.\d+\.\d+)') { throw 'Canonical version missing' }
$version = $Matches[1]
$PackageName = "GlassBoard-$version"
if (Test-Path -LiteralPath "$PSScriptRoot\dist\$PackageName.zip") { throw 'Versioned archive already exists; preserve it and increment the patch version' }
& "$PSScriptRoot\build.ps1" -QtRoot $QtRoot
$dest = Join-Path $PSScriptRoot "dist\$PackageName"
if (Test-Path -LiteralPath $dest) { throw "Package already exists; preserve it and increment the patch version: $dest" }
New-Item -ItemType Directory -Force -Path $dest | Out-Null
Copy-Item "$PSScriptRoot\build\GlassBoard.exe" $dest -Force
Copy-Item "$PSScriptRoot\README.md" $dest -Force
Copy-Item "$PSScriptRoot\THIRD_PARTY_NOTICES.txt" $dest -Force
& "$QtRoot\bin\windeployqt.exe" --release --compiler-runtime --no-translations --no-system-d3d-compiler --no-opengl-sw --dir $dest "$dest\GlassBoard.exe"
if ($LASTEXITCODE -ne 0) { throw 'Deployment failed' }
$crt = 'Z:\Soft\VC2022\VC\Redist\MSVC\14.44.35112\x64\Microsoft.VC143.CRT'
Copy-Item "$crt\*.dll" $dest -Force
if ((Get-Item "$dest\GlassBoard.exe").VersionInfo.ProductVersion -ne $version) { throw 'EXE version mismatch' }
Compress-Archive -Path "$dest\*" -DestinationPath "$PSScriptRoot\dist\$PackageName.zip" -Force
"$((Get-FileHash "$PSScriptRoot\dist\$PackageName.zip").Hash)  $PackageName.zip" | Set-Content "$PSScriptRoot\dist\$PackageName.zip.sha256" -Encoding ascii
Get-FileHash "$PSScriptRoot\dist\$PackageName.zip" | Format-List
