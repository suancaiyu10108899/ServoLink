# ADR 0003: BaseAngle and positioning annotations

## Status
Accepted

## Context
使用者反映：实际航模结构中 O₁O₂ 不总是水平，基距 d 可能倾斜。需要让机构坐标反映真实的机体安装姿态。

同时，连杆角度 θ₂ 不等于舵面姿态角——在球头 B 与舵面之间通常有安装偏置。需要标注舵面原点、限位与伺服物理限位之间的关系。

## Decision

1. **基距倾角**：在 `LinkageParams` 新增 `baseAngle` 字段（默认 0°），O₂ 坐标从 `(d, 0)` 改为 `(d·cosα, d·sinα)`。
   - 所有求解器计算（正解/反解/扫描/解析解）统一使用旋转后的 O₂ 坐标。

2. **坐标系不变**：O₁ 始终固定为 `(0, 0)`，θ₁ 始终相对于水平 x 轴。不引入局部坐标系——简化数学推导和代码维护。

3. **定位标注 5 字段**：
   - `hornOriginDeg`（舵面原点 θ₂）
   - `hornLimitUpDeg`（舵面上限 θ₂）
   - `hornLimitLoDeg`（舵面下限 θ₂）
   - `servoLimitMinDeg`（舵机物理限位 θ₁ 下限）
   - `servoLimitMaxDeg`（舵机物理限位 θ₁ 上限）

4. **标注不做限位检测**：当前阶段仅作为可视化辅助，不在计算逻辑中强制限位——避免干扰设计迭代。

## Consequences
1. 参数面板新增「基距倾角」SpinBox（-90°~90°）
2. 参数面板新增「定位标注」分组（5 个 SpinBox）
3. 所有求解器代码中 O₂ 坐标计算涉及 `cos(baseAngle)`/`sin(baseAngle)`
4. `KinematicsView` 和 `CurvePlot` 可视化自动适配基距倾角
