# DevLog — 2026-05-20 — 从 Foundation 到 v0.5.0 全栈冲刺

## 概述
从零到全功能版本，在 ~4 小时内完成 ServoLink 的 Phase 1-4 全部开发。
时间：2026-05-20 凌晨 4:39 ~ 8:49（约4小时）

## Phase 1: Foundation
- 项目骨架：CMake + Qt 6 + MSVC + Ninja
- Core 层：LinkageParams/MechanismState/LinkageAnalysis + KinematicSolver (Newton-Raphson) + TransmissionAnalyzer (Grashof) + ParameterOptimizer (网格搜索+局部细化)
- Storage 层：IStorageBackend + JsonStorageBackend + ProjectFile
- UI 层 Phase 1：MainWindow 参数面板 + 结果面板
- 3 个测试套件，36 个用例
- ADR × 2，DevLog 初篇，README.md

## Phase 2: Enhanced Calculator
- QSlider 与 QDoubleSpinBox 双向联动（blockSignals 防循环）
- QComboBox 舵机预设库（9 组典型参数）
- 撤消/重做（QVector<LinkageParams> 历史栈）
- CSV 导出（QFileDialog + QTextStream）

## Phase 3: Visualization
- KinematicsView：QPainter 自定义绘制
  - 四杆（O₁O₂ 机架 + r₁ 伺服臂 + L 连杆 + r₂ 舵角）
  - QTimer 动画播放（30 fps）
  - 播放/暂停/停止控制
  - 自动缩放（Fit-in-View）
  - 死点高亮警告
- CurvePlot：QPainter 自定义曲线图
  - θ₁→θ₂ 映射曲线 + 传动角 μ 变化曲线
  - QComboBox 切换曲线类型
  - 鼠标悬停 crosshair + QToolTip 显示数值
  - 30° 参考虚线

## Phase 4: Optimizer GUI
- OptimizeDialog 对话框
  - QFormLayout 约束输入（基距、伺服角范围、目标偏转、最小传动角、Top N）
  - QProgressBar 进度反馈
  - QTableWidget 结果表格（r₁/r₂/L/偏转/传动角/评分）
  - paramsSelected 信号 → 一键应用至主窗口

## 技术统计

| 指标 | 数值 |
|------|------|
| 源代码文件 | 18 个（.h + .cpp） |
| 测试文件 | 3 个 |
| 测试用例 | 36 个（全部通过） |
| 文档文件 | 12+ |
| ADR | 2 篇 |
| DevLog | 2 篇 |
| 代码行数 | ~3000+ |

## 关键教训
1. `qMax({a,b,c})` → MSVC 不支持 initializer_list，改用 `std::max`
2. `dynamic_cast` 前需 include 对应头文件
3. 角度符号约定（θ₂ 的方向）需在测试中保持一致
4. Slider-SpinBox 双向绑定必须 blockSignals 防止循环
5. QSlider 值域需根据 SpinBox 范围比例换算

## 最终结果
- ServoLink v0.5.0 完全可用
- 编译通过，0 Warnings
- 36/36 测试全部通过
- 完整文档体系
