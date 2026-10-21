param(
    [Parameter(Mandatory)][string]$LifecycleReport,
    [string]$CloudRoot = 'Z:\Yandex.Disk\PharosHub'
)
$ErrorActionPreference = 'Stop'
$cmake = Get-Content "$PSScriptRoot\CMakeLists.txt" -Raw
if ($cmake -notmatch 'project\(GlassBoard VERSION (\d+\.\d+\.\d+)') { throw 'Canonical version missing' }
$version = $Matches[1]
$report = Get-Content -LiteralPath $LifecycleReport -Raw | ConvertFrom-Json
if ($report.Version -ne $version -or $report.Install -ne 'passed' -or $report.Upgrade -ne 'passed' -or $report.Uninstall -ne 'passed' -or -not $report.UserDataPreserved) { throw 'Successful current-version lifecycle verification is required' }
$package = "$PSScriptRoot\dist\GlassBoard-$version"
$setup = "$PSScriptRoot\dist\GlassBoardSetup_$version.exe"
if ((Get-Item -LiteralPath $setup).VersionInfo.ProductVersion.Trim() -ne $version -or (Get-Item "$package\GlassBoard.exe").VersionInfo.ProductVersion -ne $version) { throw 'Setup / EXE / canonical version mismatch' }
$forbidden = @(Get-ChildItem -LiteralPath $package -Recurse -File -Force | Where-Object { $_.Extension -in '.db','.sqlite','.sqlite3','.ini','.log' -or $_.Name -like 'smoke*' })
if ($forbidden.Count) { throw 'Personal or test data in package' }
$catalog = Get-Content 'Z:\CPP\PharosHub\config\apps.json' -Raw | ConvertFrom-Json
$entry = @($catalog.applications | Where-Object id -eq 'glass-board')
if ($entry.Count -ne 1 -or $entry[0].version -ne $version) { throw 'Update the Hub catalog to match this verified release' }
& "$PSScriptRoot\verify_package.ps1" -PackageDirectory $package
$installerDir = Join-Path $CloudRoot 'installers\glass-board'
New-Item -ItemType Directory -Path $installerDir -Force | Out-Null
$destination = Join-Path $installerDir ([IO.Path]::GetFileName($setup))
$hash = (Get-FileHash -LiteralPath $setup).Hash
if (Test-Path -LiteralPath $destination) {
    if ((Get-FileHash -LiteralPath $destination).Hash -ne $hash) { throw 'An installer with this version and another hash already exists' }
} else { Copy-Item -LiteralPath $setup -Destination $destination }
if ((Get-FileHash -LiteralPath $destination).Hash -ne $hash) { throw 'Cloud installer SHA256 mismatch' }
"$hash  $([IO.Path]::GetFileName($setup))" | Set-Content ($destination + '.sha256') -Encoding ascii
& 'Z:\CPP\PharosHub\publish-release.ps1' -AppId glass-board -Version $version -Package $package -Executable GlassBoard.exe -CloudRoot $CloudRoot
$localCatalog = 'Z:\CPP\PharosHub\package\config\apps.json'
if (Test-Path -LiteralPath $localCatalog) {
    Copy-Item -LiteralPath 'Z:\CPP\PharosHub\config\apps.json' -Destination $localCatalog -Force
}
Write-Output "Installer=$destination; SHA256=$hash"
