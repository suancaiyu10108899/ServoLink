# Roadmap — ServoLink

## Phase 1: Foundation ✅
- [x] 项目骨架 + CMake + Qt 环境
- [x] KinematicSolver（Newton-Raphson 正解/反解/扫描）
- [x] TransmissionAnalyzer（Grashof/快速评估/改进建议）
- [x] ParameterOptimizer（网格搜索+局部细化）
- [x] JsonStorageBackend（JSON 持久化）
- [x] MainWindow 基础 UI
- [x] 3 个测试套件，33/33 通过
- [x] ADR × 2，DevLog 初篇

## Phase 2: Enhanced Calculator (当前)
- [ ] Slider 联动参数输入
- [ ] 舵机规格预设库
- [ ] 结果导出（CSV/文本表格）
- [ ] 参数历史记录（撤销/重做）
- [ ] 快捷键系统（F5 计算等）

## Phase 3: Visualization
- [ ] KinematicsView：QPainter 绘制机构简图
- [ ] 实时动画（QTimer 步进）
- [ ] CurvePlot：传动角/偏转角特性曲线
- [ ] 死点位置高亮标注

## Phase 4: Optimizer GUI
- [ ] OptimizeDialog：约束输入界面
- [ ] 优化进度条
- [ ] Top N 结果排名列表
- [ ] 一键应用最优参数

## Phase 5: Polish
- [ ] 机构学理论教学文档（LaTeX）
- [ ] 完整回归测试套件
- [ ] GitHub Actions CI
- [ ] 安装包 / 发布
