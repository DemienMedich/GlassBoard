param([switch]$SkipPackage, [string]$IsccPath = 'C:\Users\mrdem\AppData\Local\Programs\Inno Setup 6\ISCC.exe')
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$cmake = Get-Content "$root\CMakeLists.txt" -Raw
if ($cmake -notmatch 'project\(GlassBoard VERSION (\d+\.\d+\.\d+)') { throw 'Canonical version missing' }
$version = $Matches[1]
if (-not $SkipPackage) { & "$root\package.ps1" }
$package = "$root\dist\GlassBoard-$version"
$forbidden = @(Get-ChildItem -LiteralPath $package -Recurse -File -Force | Where-Object { $_.Extension -in '.db','.sqlite','.sqlite3','.ini','.log' -or $_.Name -like 'smoke*' })
if ($forbidden.Count) { throw 'Package contains user or test data' }
if ((Get-Item "$package\GlassBoard.exe").VersionInfo.ProductVersion -ne $version) { throw 'EXE version mismatch' }
& "$root\verify_package.ps1" -PackageDirectory $package
& $IsccPath "/DAppVersion=$version" "/DPackageDir=$package" "/DOutputDir=$root\dist" "$PSScriptRoot\GlassBoard.iss"
if ($LASTEXITCODE) { throw 'Installer compilation failed' }
$installer = "$root\dist\GlassBoardSetup_$version.exe"
"$((Get-FileHash -LiteralPath $installer).Hash)  $([IO.Path]::GetFileName($installer))" | Set-Content ($installer + '.sha256') -Encoding ascii
Write-Output "INSTALLER=$installer"
