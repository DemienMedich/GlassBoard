param([string]$PackageDirectory = '')
$ErrorActionPreference = 'Stop'
$cmake = Get-Content "$PSScriptRoot\CMakeLists.txt" -Raw
if ($cmake -notmatch 'project\(GlassBoard VERSION (\d+\.\d+\.\d+)') { throw 'Canonical version missing' }
$version = $Matches[1]
if (-not $PackageDirectory) { $PackageDirectory = "$PSScriptRoot\dist\GlassBoard-$version" }
$exe = Join-Path $PackageDirectory 'GlassBoard.exe'
$testRoot = Join-Path $PSScriptRoot ('build\package-smoke-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $testRoot | Out-Null
$before = @{}
foreach ($f in Get-ChildItem -LiteralPath $PackageDirectory -Recurse -File -Force) { $before[$f.FullName] = (Get-FileHash -LiteralPath $f.FullName).Hash }
$oldPath = $env:PATH
$environmentNames = @('QT_PLUGIN_PATH','QT_QPA_PLATFORM_PLUGIN_PATH','QT_QPA_PLATFORM','QML2_IMPORT_PATH','QML_IMPORT_PATH')
$savedEnvironment = @{}
foreach ($name in $environmentNames) { $savedEnvironment[$name] = [Environment]::GetEnvironmentVariable($name,'Process') }
try {
    $env:PATH = "$env:SystemRoot\System32;$env:SystemRoot"
    foreach ($name in $environmentNames) { [Environment]::SetEnvironmentVariable($name,$null,'Process') }
    $process = Start-Process -FilePath $exe -WindowStyle Hidden -ArgumentList @('--smoke-test','--data-dir',"`"$testRoot\profile`"",'--smoke-output',"`"$testRoot\output`"") -PassThru -RedirectStandardError "$testRoot\stderr.txt"
    if (-not $process.WaitForExit(15000)) { $process.Kill(); $process.WaitForExit(); throw 'Startup timed out' }
    if ($process.ExitCode -ne 0) { throw "Smoke test failed: $($process.ExitCode)" }
    $report = Get-Content "$testRoot\output\report.json" -Raw | ConvertFrom-Json
    if (-not $report.passed -or $report.version -ne $version -or $report.windowTitle -ne "GlassBoard $version") { throw 'Runtime version or smoke mismatch' }
    if (@(Get-ChildItem -LiteralPath $PackageDirectory -Recurse -File -Force).Count -ne $before.Count) { throw 'Smoke wrote into package' }
    foreach ($path in $before.Keys) { if ((Get-FileHash -LiteralPath $path).Hash -ne $before[$path]) { throw "Package changed: $path" } }
    Write-Output "PASS: GlassBoard $version; isolated profile; drawing/eraser/undo/redo/resize/capture; clean PATH; package unchanged. Evidence=$testRoot"
} finally {
    $env:PATH = $oldPath
    foreach ($name in $environmentNames) { [Environment]::SetEnvironmentVariable($name,$savedEnvironment[$name],'Process') }
}
