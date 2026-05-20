# Project Status — ServoLink v0.1.0

## 当前版本
**v0.1.0 — Foundation Phase 完成**

## 已完成的能力

### 核心计算引擎
- [x] KinematicSolver：Newton-Raphson 正解 / 几何反解 / 200 点全范围扫描
- [x] TransmissionAnalyzer：Grashof 分类 / 快速评估 / 改进建议生成
- [x] ParameterOptimizer：网格搜索 + 局部细化 / 评分函数
- [x] UnitSystem：角度/长度转换 / 几何工具函数

### 持久化
- [x] JsonStorageBackend：JSON 文件读写（AppLocalData）
- [x] ProjectFile 模型：项目元数据 + 参数序列化
- [x] main.cpp 自动加载上次保存的参数

### UI
- [x] MainWindow：左侧参数输入 + 右侧计算结果 + 菜单栏
- [x] 保存/加载项目功能
- [x] 改进建议显示
- [x] StyleConstants 集中样式管理

### 测试
- [x] tst_kinematic_solver：17 个用例，覆盖正解/反解/扫描/边界
- [x] tst_transmission_analyzer：11 个用例，覆盖 Grashof/评估/建议
- [x] tst_parameter_optimizer：8 个用例，覆盖评分/优化/约束
- [x] 全部 33/33 测试通过

### 工程体系
- [x] CMake 3.25+ / Ninja / MSVC 构建
- [x] .editorconfig / .gitignore
- [x] ADR（2 篇）
- [x] README.md
- [x] scripts/ 开发脚本

## 进行中
- [ ] 文档体系完善（架构概览/机构学理论/DevLog/AI Memory）
- [ ] KinematicsView 机构可视化
- [ ] CurvePlot 曲线图组件

## 下一步
1. **Phase 2**：增强交互计算器（slider 联动 / 预设库 / 导出）
2. **Phase 3**：机构运动可视化（QPainter 动画）
3. **Phase 4**：优化器 GUI（OptimizeDialog）

## 已知限制
- 精度依赖 Newton-Raphson 收敛（边角参数可能失败）
- 仅支持单舵机单舵面标准四杆机构
- 无实时动画（Phase 3 计划中）
- 无导出功能（Phase 2 计划中）
