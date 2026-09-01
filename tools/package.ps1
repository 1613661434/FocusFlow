[CmdletBinding()]
param(
    [string]$QtRoot = "D:\Qt\6.10.0\mingw_64"
)

$ErrorActionPreference = "Stop"
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
& (Join-Path $PSScriptRoot "build.ps1") -QtRoot $QtRoot -BuildType "Release"
if ($LASTEXITCODE -ne 0) {
    throw "Release 构建或测试失败。"
}

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$distributionRoot = Join-Path $projectRoot "dist"
$packageDirectory = Join-Path $distributionRoot "FocusFlow-0.1.2-$timestamp"
$executable = Join-Path $projectRoot "build\FocusFlow.exe"
$deployTool = Join-Path $QtRoot "bin\windeployqt.exe"

if (-not (Test-Path $executable)) {
    throw "没有找到可执行文件：$executable"
}
if (-not (Test-Path $deployTool)) {
    throw "没有找到 windeployqt：$deployTool"
}

New-Item -ItemType Directory -Path $packageDirectory -Force | Out-Null
Copy-Item $executable (Join-Path $packageDirectory "FocusFlow.exe")

& $deployTool `
    --release `
    --compiler-runtime `
    --no-translations `
    (Join-Path $packageDirectory "FocusFlow.exe")
if ($LASTEXITCODE -ne 0) {
    throw "Qt 运行库部署失败。"
}

$archivePath = "$packageDirectory.zip"
Compress-Archive -Path (Join-Path $packageDirectory "*") -DestinationPath $archivePath
Write-Host "演示包已生成：$archivePath"
