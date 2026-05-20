# DevLog — 2026-05-21 — Feature Batch: 装配验证 + 解析式 + 输出优先 + 定位标注

## 时间线
- 00:23 – 用户提出四个核心问题：同侧/异侧验证、θ₁/θ₂ 解析式、CSV 完整工程报告、输出优先设计
- 00:26 – 开始实现解析正解 `solveForwardAnalytic()`
- 00:35 – 解析解验证标签正确性（偏差 < 0.01°）
- 00:42 – 完整工程报告 CSV（5 区块）
- 00:50 – 输出优先设计面板 `ReverseDialog`
- 02:20 – 用户追加：基距倾角、定位标注、多项式拟合、动画进度条
- 02:33 – 实现基距倾角、定位标注 5 字段、多项式拟合（1次/2次）、动画 QSlider
- 02:43 – 全部编译通过、测试通过、推送 GitHub
- 02:47 – 补全文档：ADR 0003、DevLog、CSV 定位标注+多项式系数区块

## 新增文件
| 文件 | 说明 |
|------|------|
| `src/core/KinematicSolver.cpp` `solveForwardAnalytic()` | 解析正解（闭式/开式双解） |
| `src/ui/ReverseDialog.h/.cpp` | 输出优先设计对话框 |
| `docs/adr/0003-baseangle-and-position-annotations.md` | 基距倾角+定位标注决策记录 |
| `docs/devlog/2026-05-21-feature-batch.md` | 本文件 |

## 修改文件
| 文件 | 变更 |
|------|------|
| `src/core/LinkageParams.h` | 新增 `baseAngle`、`hornOriginDeg`、`hornLimitUpDeg`、`hornLimitLoDeg`、`servoLimitMinDeg`、`servoLimitMaxDeg` |
| `src/core/KinematicSolver.cpp` | 全部 `(d,0)` → `(ox2,oy2)` 支持基距倾角 |
| `src/ui/MainWindow.h/.cpp` | 基距倾角控件、定位标注组、解析式 Tab、多项式拟合、动画进度条 |
| `src/ui/CMakeLists.txt` | 添加 `ReverseDialog.h/.cpp` |
| `README.md` | 命令格式优化 |

## 关键技术点
1. **解析解**：闭环方程化简为 `A + B·cosθ₂ + C·sinθ₂ = 0`，利用 `atan2 + ±arccos` 得出双解
2. **基距倾角**：O₂ 坐标统一改为 `(d·cosα, d·sinα)`，所有求解器适配
3. **输出优先**：固定 r₂/d，网格搜索最优 r₁/L 组合
4. **多项式拟合**：1次/2次最小二乘，自动推荐 R² 最高阶

## 工程指标
- 编译：0 warnings
- 测试：3/3 通过 (100%)
- 累计提交：6 commits
- 功能模块：Core求解器 / UI 6 Tab / 输出优先 / 定位标注 / 多项式拟合
