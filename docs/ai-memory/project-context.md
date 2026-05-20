# AI Memory — Project Context

## 项目核心信息
- **名称**：ServoLink / 舵连设计台
- **定位**：航模舵面连杆机构工程计算与优化桌面工具
- **技术栈**：C++17 + Qt 6 Widgets + CMake 3.25+ / Ninja + MSVC 2022

## 架构
四层：app → ui → core → storage，依赖严格单向。

## 关键文件
- 求解器：`src/core/KinematicSolver.h/.cpp`
- 传动分析：`src/core/TransmissionAnalyzer.h/.cpp`
- 优化器：`src/core/ParameterOptimizer.h/.cpp`
- 主窗口：`src/ui/MainWindow.h/.cpp`
- 入口：`src/app/main.cpp`

## 领域知识
四杆机构：r₁(伺服臂) ↔ L(连杆) ↔ r₂(舵角) → d(基距)
正解：Newton-Raphson 迭代
评估：传动角 ≥ 30°、Grashof 分类、线性度 R²

## 构建命令
```powershell
& '...\Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64
cd D:\Dev\ServoLink
cmake --preset win-msvc-debug
cmake --build --preset win-msvc-debug
