# ============================================================
# 测试启动脚本：以正确环境启动 multiclipboard（开发调试用）
#
# 解决要点（vcpkg 版 Qt6 布局，实测踩坑）：
#   1. 启动目录必须为项目根目录（不能设为 exe 所在目录，否则
#      报 "no Qt platform plugin could be initialized"）
#   2. PATH 需包含 vcpkg 的 installed\x64-windows\bin 与 tools\Qt6\bin
#   3. QT_PLUGIN_PATH 指向 installed\x64-windows\Qt6\plugins
#      （qwindows.dll 所在目录）
# ============================================================

[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

# ---------- 1. 定位 vcpkg 根目录（优先使用环境变量 VCPKG_ROOT） ----------
if (-not $env:VCPKG_ROOT) {
    Write-Warning "环境变量 VCPKG_ROOT 未设置，回退到默认路径 D:\SDK\vcpkg"
    $vcpkgRoot = "D:\SDK\vcpkg"
} else {
    $vcpkgRoot = $env:VCPKG_ROOT
}
if (-not (Test-Path $vcpkgRoot)) {
    Write-Error "vcpkg 目录不存在: $vcpkgRoot，请设置 VCPKG_ROOT 环境变量"
    exit 1
}

# ---------- 2. 计算相关路径 ----------
$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$exePath = Join-Path $projectRoot "build\windows\x64\release\multiclipboard.exe"
$vcpkgBinDir = Join-Path $vcpkgRoot "installed\x64-windows\bin"
$qtBinDir = Join-Path $vcpkgRoot "installed\x64-windows\tools\Qt6\bin"
$qtPluginDir = Join-Path $vcpkgRoot "installed\x64-windows\Qt6\plugins"

# ---------- 3. 校验依赖 ----------
if (-not (Test-Path $exePath)) {
    Write-Error "未找到可执行文件: $exePath`n请先执行 xmake build 构建"
    exit 1
}
if (-not (Test-Path $qtBinDir)) {
    Write-Error "未找到 Qt 工具目录: $qtBinDir"
    exit 1
}
if (-not (Test-Path $qtPluginDir)) {
    Write-Error "未找到 Qt 插件目录: $qtPluginDir"
    exit 1
}

# ---------- 4. 停止旧实例（避免多实例与 exe 占用冲突） ----------
Stop-Process -Name multiclipboard -ErrorAction SilentlyContinue

# ---------- 5. 设置环境并启动 ----------
$env:QT_PLUGIN_PATH = $qtPluginDir
$env:PATH = "$env:PATH;$vcpkgBinDir;$qtBinDir"

Write-Host "启动 multiclipboard（测试模式）..."
Write-Host "  可执行文件 : $exePath"
Write-Host "  启动目录   : $projectRoot"
Write-Host "  插件路径   : $qtPluginDir"

Start-Process -FilePath $exePath -WorkingDirectory $projectRoot

Start-Sleep -Milliseconds 800
$proc = Get-Process -Name multiclipboard -ErrorAction SilentlyContinue
if ($proc) {
    Write-Host "启动成功，PID: $($proc.Id)" -ForegroundColor Green
} else {
    Write-Host "启动失败：进程未存活，请检查上方弹窗错误信息" -ForegroundColor Red
    exit 1
}
