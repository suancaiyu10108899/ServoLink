# dev-shell.ps1
# 快速进入 ServoLink 开发环境（类似 MindPath）
Write-Host "Launching MSVC Dev Shell + Switching to ServoLink..." -ForegroundColor Cyan
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64
cd D:\Dev\ServoLink
Write-Host "Ready. Try: cmake --preset win-msvc-debug && cmake --build --preset win-msvc-debug" -ForegroundColor Green
