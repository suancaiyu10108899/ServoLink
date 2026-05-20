# DevLog — 2026-05-20 — Project Init & Foundation Phase 完成

## 概述
从零创建 ServoLink 项目，完成 Foundation Phase（核心计算引擎 + 交互 UI + 全部测试）。
时间线：约 2 小时（AI 辅助全栈开发）。

## 关键事件

### 01 项目命名与规划
- 确定名称 ServoLink / 舵连设计台
- 对齐 MindPath 四层架构（app/ui/core/storage）
- 制定 5 阶段路线图

### 02 工程骨架
- CMake 3.25+ / Ninja / MSVC 构建配置
- Qt 6 Widgets + Qt Test
- .editorconfig / .gitignore

### 03 Core 层实现
- **LinkageParams**：机构参数结构体 + validate() 装配检查
- **MechanismState**：瞬时状态（节点坐标 / 传动角 / 机械利益）
- **LinkageAnalysis**：扫描结果聚合 + Grade 评定
- **KinematicSolver**（核心）：
  - Newton-Raphson 迭代正解（解析求导）
  - 几何初值估算（余弦定理）
  - 步长阻尼 0.5 rad
  - 正解/反解/全范围扫描 200 点
- **TransmissionAnalyzer**：
  - Grashof 五种类型分类
  - QuickAssessment（5 点采样估算）
  - improvementSuggestions（工程经验规则）
- **ParameterOptimizer**：
  - 网格搜索 (r₁, r₂, L)
  - 局部细化（±20% 小范围密集搜索）
  - 三目标评分（线性度/传动角/偏转范围）

### 04 Storage 层
- IStorageBackend 接口
- JsonStorageBackend 实现（QJsonDocument 读写）
- ProjectFile 文档模型

### 05 UI 层
- MainWindow 左侧参数输入 + 右侧结果面板
- 保存/加载项目（QMenu + QAction）
- StyleConstants 集中样式表

### 06 测试
- 3 个测试套件：kinematic / transmission / optimizer
- 36 个用例，全部通过

### 07 文档体系
- README.md
- ADR × 2（存储路径 / Newton-Raphson 决策）
- 架构概览 + 机构学理论（LaTeX）
- 产品文档（Vision / Status / Roadmap）

## 构建/测试验证
```powershell
cmake --preset win-msvc-debug       # Configure
cmake --build --preset win-msvc-debug  # Build
ctest --output-on-failure           # 3/3 tests passed
```

## 遇到的坑
1. `qMax({a,b,c})` 不支持 initializer_list → 改用 `std::max({a,b,c})`
2. `dynamic_cast<JsonStorageBackend*>` 前需 include 头文件
3. Float 角度符号与预期相反 → 修正测试断言（θ₂ 基准轴理解偏差）

## 下一步
Phase 2：Slider 联动 / 舵机预设库 / 导出功能
