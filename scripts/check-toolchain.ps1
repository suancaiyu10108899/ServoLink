# check-toolchain.ps1
# 检查 ServoLink 所需的工具链是否就绪
Write-Host "=== ServoLink 工具链检查 ===" -ForegroundColor Cyan

$errors = 0

# MSVC
try {
    $clVersion = & cl 2>&1 | Select-Object -First 1
    Write-Host "[OK] MSVC compiler found" -ForegroundColor Green
} catch {
    Write-Host "[FAIL] MSVC compiler (cl.exe) not found. Run Launch-VsDevShell.ps1 first." -ForegroundColor Red
    $errors++
}

# CMake
try {
    $cmakeVersion = & cmake --version 2>&1 | Select-Object -First 1
    Write-Host "[OK] $cmakeVersion" -ForegroundColor Green
} catch {
    Write-Host "[FAIL] CMake not found" -ForegroundColor Red
    $errors++
}

# Ninja
try {
    $ninjaVersion = & ninja --version 2>&1
    Write-Host "[OK] Ninja $ninjaVersion" -ForegroundColor Green
} catch {
    Write-Host "[FAIL] Ninja not found" -ForegroundColor Red
    $errors++
}

# Qt
if (Test-Path "D:\Qt\6.11.0\msvc2022_64\bin\qmake.exe") {
    Write-Host "[OK] Qt 6.11.0 found" -ForegroundColor Green
} else {
    Write-Host "[WARN] Qt 6.11.0 not found at default path" -ForegroundColor Yellow
}

Write-Host ""
if ($errors -eq 0) {
    Write-Host "All checks passed!" -ForegroundColor Green
} else {
    Write-Host "$errors check(s) failed." -ForegroundColor Red
}
exit $errors
