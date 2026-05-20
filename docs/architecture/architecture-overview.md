# Architecture Overview — ServoLink

## 分层架构

ServoLink 采用与 MindPath 相同的四层架构，依赖方向严格单向：

```
app/  (应用入口)
 ↑
ui/   (界面层)
 ↑
core/ (业务逻辑层)
 ↑
storage/ (持久化层)
```

### app/
应用启动、生命周期管理、根对象装配（依赖注入）。
- `main.cpp`：初始化 QApplication、组装各层依赖、启动 MainWindow

### ui/
界面层，负责窗口、控件、用户交互、事件转发。
- `MainWindow`：参数输入面板 + 计算结果展示
- (计划) `KinematicsView`：机构可视化
- (计划) `CurvePlot`：曲线图

### core/
核心业务层。纯计算逻辑，不依赖 Qt Widgets（仅依赖 Qt::Core 容器）。
领域模型均为值类型（非 QObject），无信号槽，可自由拷贝。

**领域模型**：
- `LinkageParams`：机构几何参数 (r₁, r₂, L, d, θ₁范围)
- `MechanismState`：某一时刻的瞬时状态（节点坐标、传动角）
- `LinkageAnalysis`：全范围扫描结果聚合

**计算引擎**：
- `KinematicSolver`：正解 (Newton-Raphson) / 反解 / 全范围扫描
- `TransmissionAnalyzer`：Grashof 分类 / 快速评估 / 改进建议
- `ParameterOptimizer`：网格搜索 + 局部细化

**工具**：
- `UnitSystem`：角度/长度转换、几何工具函数
- `OperationResult`：操作结果封装（成功/失败 + 消息）

### storage/
持久化层，负责数据序列化与文件 I/O。
- `IStorageBackend`：存储接口（纯虚类）
- `JsonStorageBackend`：JSON 文件实现
- `ProjectFile`：项目文档模型

## 依赖原则
- app 可以依赖 ui、core、storage
- ui 可以依赖 core 中稳定接口 + storage
- core 不依赖 ui（纯计算）
- storage 依赖 core 的领域模型

## 数据流

```
用户操作 (UI)
  │
  ▼
MainWindow::onCalculate()
  │ readParamsFromUi()
  │ kinSolver.setParams()
  │ kinSolver.sweepRange(200)
  ▼
LinkageAnalysis ── displayAnalysis() ──▶ UI 结果面板
                ── transmissionAnalyzer ──▶ 评估摘要
```

启动恢复流：main → JsonStorageBackend::loadAll() → LinkageParams::fromJson() → writeParamsToUi()
