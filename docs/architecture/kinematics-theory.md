# 舵面连杆机构运动学理论

> **定位**：本文档是 ServoLink 的机构学理论基石，既是工程参考手册，也是教学材料。
> 所有公式均按 ServoLink 内部的坐标约定和命名规范编写。
>
> **内在单位约定**：长度 — mm，角度 — °。

## 1. 机构描述

舵面连杆机构是标准平面四杆机构（Four-Bar Linkage）的一种工程应用特例。

### 1.1 坐标约定

以伺服旋转中心 \(O_1\) 为原点，舵面铰链中心 \(O_2\) 在 x 轴正方向距离 \(d\) 处：

$$ O_1 = (0, 0),\quad O_2 = (d, 0) $$

### 1.2 四根杆的命名

| 符号 | 名称 | 工程含义 |
|------|------|----------|
| \(r_1\) | 伺服臂 (Servo Arm) | 舵机输出盘中心到球头 A 的距离，主动件 |
| \(L\) | 连杆 (Link Rod) | 球头 A 到球头 B 的距离，中间件 |
| \(r_2\) | 舵角 (Control Horn) | 舵面铰链中心到球头 B 的距离，从动件 |
| \(d\) | 基距 (Base Distance) | 两旋转中心的固定距离，机架 (Frame) |

### 1.3 机构简图

```
         Y
         ↑
         │    ● A (伺服臂末端，球头)
         │   ╱
         │  ╱  连杆 L
         │ ╱
    O₁(0,0)─────────────O₂(d,0)──→ X
         │╲              │
         │ ╲             │ 舵角 r₂
         │  ╲            │
         │   ● B (舵角末端，球头)
```

---

## 2. 位置正解（已知 θ₁ 求 θ₂）

### 2.1 问题表述

给定伺服臂转角 \(\theta_1\)（定义为 O₁A 与 x 轴正向的夹角），求舵角 \(\theta_2\)。

### 2.2 闭环矢量方程

$$ \vec{O_1A} + \vec{AB} = \vec{O_1O_2} + \vec{O_2B} $$

写成分量形式：

$$
\begin{aligned}
r_1 \cos\theta_1 + L \cos\phi &= d + r_2 \cos\theta_2 \\
r_1 \sin\theta_1 + L \sin\phi &=       r_2 \sin\theta_2
\end{aligned}
$$

其中 \(\phi\) 是连杆 AB 的方位角。

### 2.3 消元法

将两个方程中的 \(\phi\) 消去。将含 \(\phi\) 的项移项后平方相加：

$$
(L \cos\phi)^2 + (L \sin\phi)^2 = (d + r_2\cos\theta_2 - r_1\cos\theta_1)^2 + (r_2\sin\theta_2 - r_1\sin\theta_1)^2
$$

化简得 **闭环方程**：

$$ F(\theta_2) = (d + r_2\cos\theta_2 - r_1\cos\theta_1)^2 + (r_2\sin\theta_2 - r_1\sin\theta_1)^2 - L^2 = 0 $$

### 2.4 Newton-Raphson 迭代

由于 \(F(\theta_2) = 0\) 是关于 \(\theta_2\) 的非线性方程，ServoLink 使用 Newton-Raphson 迭代法求解。

**迭代公式**：

$$ \theta_2^{(k+1)} = \theta_2^{(k)} - \frac{F(\theta_2^{(k)})}{F'(\theta_2^{(k)})} $$

**解析导数**（准确高效）：

$$
\begin{aligned}
F'(\theta_2) &= 2 (B_x - A_x) \cdot \frac{dB_x}{d\theta_2} + 2 (B_y - A_y) \cdot \frac{dB_y}{d\theta_2} \\
&= 2 r_2 \big[-(B_x - A_x)\sin\theta_2 + (B_y - A_y)\cos\theta_2\big]
\end{aligned}
$$

其中 \((A_x, A_y)\) 和 \((B_x, B_y)\) 分别用当前 \(\theta_1\) 和 \(\theta_2\) 计算。

**初值估算**（关键！好初值决定收敛速度）：

利用三角形 O₂AB 的余弦定理：

$$ \cos\angle AO_2B = \frac{O_2A^2 + r_2^2 - L^2}{2\cdot O_2A \cdot r_2} $$

O₂A 的方向角 \(\psi = \operatorname{atan2}(A_y, A_x - d)\)，则初值：

$$ \theta_2^{(0)} = \psi - \angle AO_2B \quad (\text{闭式装配}) $$

**收敛性质**：在舵面连杆机构的典型工作区（传动角 ≥ 20°），Newton-Raphson 通常在 3-7 次迭代内收敛到 \(10^{-10}\) 精度。

---

## 3. 传动质量指标

### 3.1 传动角 (Transmission Angle)

传动角 \(\mu\) 是连杆 AB 与舵角 O₂B 的夹角，取值范围 \([0°, 180°]\)。

$$ \mu = \arccos\left(\frac{\vec{AB} \cdot \vec{O_2B}}{|\vec{AB}| \cdot |\vec{O_2B}|}\right) $$

**工程意义**：
- \(\mu = 90°\)：力传递效率最高
- \(\mu \to 0°\) 或 \(\mu \to 180°\)：机构逼近死点，力传递效率趋近于零
- **工程准则**：\(\mu_{\min} \geq 30°\)

### 3.2 死点 (Dead Point)

当传动角 \(\mu < 5°\) 时，定义为**近死点**。此时：
- 微小的伺服扭矩无法驱动舵面
- 舵面受气流扰动可能反向回弹
- 应严格避免在正常工作范围内出现死点

### 3.3 Grashof 准则

对四根杆长排序：\(s \leq p \leq q \leq l\)（最短到最长），则：

| 条件 | 类型 | 工程意义 |
|------|------|----------|
| \(s+l < p+q\)，最短为连架杆 | 曲柄摇杆 | 伺服臂可整周回转 ✅ |
| \(s+l < p+q\)，最短为机架 | 双曲柄 | 两连架杆均可整周回转 |
| \(s+l < p+q\)，最短为连杆 | 双摇杆 | 两连架杆均不可整周回转 |
| \(s+l = p+q\) | 变点机构 | 临界状态 |
| \(s+l > p+q\) | 非Grashof | 三摇杆，无整周回转 |

舵面连杆机构通常为**曲柄摇杆**或**双摇杆**，伺服臂工作范围小（±60°），即使无法整周回转也无妨。

### 3.4 线性度

线性度描述 \(\theta_1 \mapsto \theta_2\) 映射偏离线性关系的程度。ServoLink 使用确定系数 \(R^2\) 作为线性度评分。

设采集到的 \((\theta_1^{(i)}, \theta_2^{(i)})\) 共 \(n\) 对，最小二乘拟合直线 \(y = mx + b\)：

$$ R^2 = 1 - \frac{\sum_i (\theta_2^{(i)} - m\theta_1^{(i)} - b)^2}{\sum_i (\theta_2^{(i)} - \bar\theta_2)^2} $$

\(R^2 \to 1\) 表示输入-输出近乎线性，舵面操控手感最佳。

---

## 4. 参考

- 机构学教材：《Mechanisms and Mechanical Devices Sourcebook》
- MindPath 工程规范（四层架构、CMake、Qt 6）
- ServoLink 源码：`src/core/KinematicSolver.cpp`（实现细节与注释）
