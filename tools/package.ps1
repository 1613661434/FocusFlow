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
$packageDirectory = Join-Path $distributionRoot "FocusFlow-0.1.40-$timestamp"
$executable = Join-Path $projectRoot "build\FocusFlow.exe"
$uninstaller = Join-Path $projectRoot "build\Uninstall.exe"
$deployTool = Join-Path $QtRoot "bin\windeployqt.exe"

if (-not (Test-Path $executable)) {
    throw "没有找到可执行文件：$executable"
}
if (-not (Test-Path $uninstaller)) {
    throw "没有找到卸载程序：$uninstaller"
}
if (-not (Test-Path $deployTool)) {
    throw "没有找到 windeployqt：$deployTool"
}

New-Item -ItemType Directory -Path $packageDirectory -Force | Out-Null
Copy-Item $executable (Join-Path $packageDirectory "FocusFlow.exe")
Copy-Item $uninstaller (Join-Path $packageDirectory "Uninstall.exe")
foreach ($document in @("README.md", "LICENSE")) {
    Copy-Item (Join-Path $projectRoot $document) $packageDirectory
}

& $deployTool `
    --release `
    --compiler-runtime `
    --no-translations `
    (Join-Path $packageDirectory "FocusFlow.exe")
if ($LASTEXITCODE -ne 0) {
    throw "Qt 运行库部署失败。"
}

$translationSource = Join-Path $QtRoot "translations"
$translationDestination = Join-Path $packageDirectory "translations"
New-Item -ItemType Directory -Path $translationDestination -Force | Out-Null
foreach ($catalog in @("qt_zh_CN.qm", "qtbase_zh_CN.qm")) {
    $catalogPath = Join-Path $translationSource $catalog
    if (-not (Test-Path $catalogPath)) {
        throw "没有找到 Qt 中文翻译文件：$catalogPath"
    }
    Copy-Item $catalogPath $translationDestination
}

$archivePath = "$packageDirectory.zip"
Compress-Archive -Path (Join-Path $packageDirectory "*") -DestinationPath $archivePath
Write-Host "发布包已生成：$archivePath"
