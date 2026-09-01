[CmdletBinding()]
param(
    [string]$QtRoot = "D:\Qt\6.10.0\mingw_64",
    [string]$BuildType = "Release"
)

$ErrorActionPreference = "Stop"
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$buildDirectory = Join-Path $projectRoot "build"
$cmakeExecutable = if (Test-Path "D:\CMake\bin\cmake.exe") {
    "D:\CMake\bin\cmake.exe"
} else {
    (Get-Command cmake).Source
}
$ninjaExecutable = "D:\Qt\Tools\Ninja\ninja.exe"
$compilerExecutable = "D:\Qt\Tools\mingw1310_64\bin\g++.exe"

if (-not (Test-Path $QtRoot)) {
    throw "Qt 路径不存在：$QtRoot"
}
if (-not (Test-Path $ninjaExecutable)) {
    throw "Ninja 不存在：$ninjaExecutable"
}
if (-not (Test-Path $compilerExecutable)) {
    throw "MinGW 编译器不存在：$compilerExecutable"
}

& $cmakeExecutable `
    -S $projectRoot `
    -B $buildDirectory `
    -G Ninja `
    "-DCMAKE_BUILD_TYPE=$BuildType" `
    "-DCMAKE_PREFIX_PATH=$QtRoot" `
    "-DCMAKE_MAKE_PROGRAM=$ninjaExecutable" `
    "-DCMAKE_CXX_COMPILER=$compilerExecutable"
if ($LASTEXITCODE -ne 0) {
    throw "CMake 配置失败。"
}

& $cmakeExecutable --build $buildDirectory --parallel 4
if ($LASTEXITCODE -ne 0) {
    throw "FocusFlow 编译失败。"
}

& ctest --test-dir $buildDirectory --output-on-failure
if ($LASTEXITCODE -ne 0) {
    throw "FocusFlow 自动化测试失败。"
}

Write-Host "FocusFlow 已构建并通过测试：$buildDirectory"

