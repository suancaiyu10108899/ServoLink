# ServoLink / 舵连设计台

> **项目定位**：航模舵面连杆机构的工程计算、参数优化与运动仿真桌面工具
>
> ServoLink（舵连设计台）是一个面向航模爱好者和机械设计学习者的桌面应用，专注于快速求解舵面连杆机构的运动学参数，评估传动质量，并自动搜索最优机构尺寸。

## 当前阶段

- ✅ **Foundation Phase**（v0.1.0～v0.7.0）：核心计算引擎 + 完整 UI + 可视化系统
- 🔄 **当前**（v0.7.x）：装配模式、解析解、完整工程报告、输出优先设计、基距倾角、定位标注、多项式拟合
- 📋 计划中：JSON 导出、限位检测、CI 集成

## 当前技术路线

| 项目 | 选型 |
|------|------|
| Language | C++17 |
| GUI Framework | Qt 6 Widgets |
| Build System | CMake 3.25+ / Ninja |
| IDE/Editor | VSCode |
| Platform | Windows (primary) |
| Compiler | MSVC (VS 2022) |
| CI | GitHub Actions (待添加) |

## 数据文件路径

- **存储位置**：`%LOCALAPPDATA%\ServoLink\ServoLink\servolink.json`
- **格式**：JSON (version 1)
- **决策记录**：[ADR 0001](docs/adr/0001-use-local-appdata-for-storage.md)

```powershell
Get-Content "$env:LOCALAPPDATA\ServoLink\ServoLink\servolink.json" -Encoding utf8
```

## 仓库结构

```text
src/
    app/          应用入口（main.cpp，依赖组装）
    core/         业务逻辑（运动学求解器 + 传动分析 + 参数优化器）
    storage/      持久化层（JsonStorageBackend + ProjectFile）
    ui/           界面层（MainWindow）
tests/
    core/         Core 层单元测试（求解器 + 传动分析 + 优化器）
docs/
    adr/          架构决策记录
    ai-memory/    AI 协作状态
    architecture/ 架构与机构学理论文档
    debug-log/    调试日志
    devlog/       开发日志
    product/      产品文档
scripts/         开发辅助脚本
training/        独立训练项目
```

## 构建与运行

### 首次配置（仅一次，或 CMakeLists 变更后）

& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64
cd D:\Dev\ServoLink
cmake --preset win-msvc-debug

### 日常编译 + 测试 + 运行（每次修改代码后）

& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64
cd D:\Dev\ServoLink
cmake --build --preset win-msvc-debug
cd build\win-msvc-debug
ctest --output-on-failure
cd D:\Dev\ServoLink
.\build\win-msvc-debug\src\app\ServoLink.exe

> 提示：鼠标三击某行即可选中整行，Ctrl+C 复制到终端运行。
> 如果报 LNK1168 错误，说明上一个 ServoLink.exe 还在运行，先关掉窗口即可。

## 核心功能

1. **运动学正解/反解**：给定伺服臂转角求舵面偏转，或反向求解
2. **全范围扫描**：200点采样评估机构全域性能
3. **传动质量分析**：Grashof 类型判定、传动角、死点检测
4. **参数优化器**：网格搜索 + 局部细化，自动推荐最优杆长
5. **项目持久化**：JSON 文件保存/加载参数

## 文档入口

- [项目状态](docs/product/status.md)
- [产品愿景](docs/product/vision.md)
- [架构概览](docs/architecture/architecture-overview.md)
- [机构学理论](docs/architecture/kinematics-theory.md)
- [开发日志](docs/devlog/)
- [调试日志](docs/debug-log/INDEX.md)

## 说明

本项目同时也是作者的 C++ 工程能力训练项目，采用与 MindPath 相同的四层架构
（app/ui/core/storage）、CMake + Qt 6 + MSVC 技术栈，以及完整的文档体系。

训练路线参照 MindPath 的工程规范，目标是将 C++ 桌面应用开发能力从 L2 提升到 L3。
