# AI Memory — Current Status

## 项目状态
- **日期**：2026-05-20
- **版本**：v0.5.0 全功能完成
- **阶段**：Phase 1-4 全部完成，36/36 测试通过

## 最近完成的工作
1. Phase 1: Foundation（Core 求解器 + Storage + 基础 UI）
2. Phase 2: Enhanced Calculator（Slider 联动 + 预设库 + 撤消/重做 + CSV 导出）
3. Phase 3: Visualization（KinematicsView 动画 + CurvePlot 曲线）
4. Phase 4: Optimizer GUI（OptimizeDialog 集成）
5. 完整文档体系（ADR × 2 + 架构文档 + 机构学理论 + DevLog × 2 + AI Memory）

## 当前 Task
- ✅ 项目完成，等待用户醒来验证使用

## 关键设计决策
- Newton-Raphson 迭代法正解（ADR 0002）
- AppLocalData 存储路径（ADR 0001）
- 四层架构对齐 MindPath（app/ui/core/storage）
- 评分函数权重：线性度 0.4、传动角 0.3、偏转范围 0.3

## 用户偏好
- 教学与项目开发分离：项目代码 + DevLog 上传 GitHub
- 个人学习笔记留在 `docs/personal-learning/`，不上传
- 教学越详细越好，使用 LaTeX 数学公式
- 单位系统：mm + °
