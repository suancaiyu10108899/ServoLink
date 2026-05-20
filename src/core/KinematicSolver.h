#ifndef KINEMATICSOLVER_H
#define KINEMATICSOLVER_H

#include <optional>
#include "LinkageParams.h"
#include "MechanismState.h"
#include "LinkageAnalysis.h"
#include "OperationResult.h"

/**
 * @brief 运动学求解器
 *
 * 对标准平面四杆机构进行正解 / 反解 / 全范围扫描。
 *
 * 数学原理：
 *   使用 Newton-Raphson 迭代法求解非线性闭环方程。
 *   闭环矢量方程：O₁A + AB + BO₂ + O₂O₁ = 0
 *
 *   标量形式（坐标约定 O₁=(0,0), O₂=(d,0)）：
 *     r₁·cos(θ₁) + L·cos(φ) - r₂·cos(θ₂) - d = 0  ... (1)
 *     r₁·sin(θ₁) + L·sin(φ) - r₂·sin(θ₂)     = 0  ... (2)
 *
 *   其中 φ 是连杆 AB 的方位角，θ₂ 是舵角的方位角（从 O₂ 到 B 的方向）。
 *
 *   消去 φ，得到关于 θ₂ 的一元方程 F(θ₂; θ₁) = 0。
 *   对于给定的 θ₁，用 Newton-Raphson 迭代求 θ₂。
 *
 * 收敛性说明：
 *   舵面连杆机构中，r₁, r₂ 通常远小于 d 和 L，
 *   机构远离奇异位形，Newton-Raphson 在 5-10 次迭代内必然收敛。
 */
class KinematicSolver
{
public:
    /**
     * @brief 装配模式
     *
     * 四杆机构的闭环方程对 θ₂ 有两个解，对应两种装配模式：
     *
     *   Closed (闭式) — A 和 B 在机架 O₁O₂ 同侧（航模标准配置）
     *     初值：θ₂ = ψ - ∠AO₂B
     *
     *   Open (开式) — A 和 B 分居机架 O₁O₂ 两侧
     *     初值：θ₂ = ψ + ∠AO₂B
     */
    enum class AssemblyMode {
        Closed,   // 闭式：同侧（默认）
        Open      // 开式：对侧
    };

    KinematicSolver() = default;
    explicit KinematicSolver(const LinkageParams &params);

    void setParams(const LinkageParams &params);
    LinkageParams params() const { return m_params; }

    void setAssemblyMode(AssemblyMode mode) { m_assemblyMode = mode; }
    AssemblyMode assemblyMode() const { return m_assemblyMode; }

    /**
     * @brief 正解：给定 θ₁（伺服角度），求机构完整状态
     *
     * @param inputAngleDeg 伺服臂转角（度）
     * @return MechanismState 机构状态（若迭代不收敛返回 std::nullopt）
     */
    std::optional<MechanismState> solveForward(double inputAngleDeg) const;

    /**
     * @brief 反解：给定期望的 θ₂（舵面偏转角），求需要的 θ₁
     *
     * 注：反解可能有两个解（对应机构的两种装配模式——开式/闭式）。
     *   默认返回闭式装配（航模舵面常见模式）。
     *
     * @param outputAngleDeg 目标舵面偏转角（度）
     * @return MechanismState 机构状态（含对应的 θ₁）
     */
    std::optional<MechanismState> solveInverse(double outputAngleDeg) const;

    /**
     * @brief 全范围扫描
     *
     * 在 [servoAngleMin, servoAngleMax] 范围内均匀采样 steps 个点，
     * 对每个点做正解，汇总为 LinkageAnalysis。
     *
     * @param steps 采样点数（默认 200）
     */
    LinkageAnalysis sweepRange(int steps = 200) const;

    /**
     * @brief 解析正解（闭式解）
     *
     * 四杆机构闭环方程可化为 A + B·cosθ₂ + C·sinθ₂ = 0，
     * 解析解为 θ₂ = atan2(-C, -B) ± arccos(-A/√(B²+C²))
     *
     * @param inputAngleDeg 伺服臂转角（度）
     * @return std::pair<MechanismState, MechanismState>
     *         .first  = 闭式装配 (同侧)
     *         .second = 开式装配 (对侧)
     *         若机构不可装配，返回空 optional 的 pair
     */
    std::optional<std::pair<MechanismState, MechanismState>>
        solveForwardAnalytic(double inputAngleDeg) const;

    /**
     * @brief 验证参数合法性
     */
    OperationResult validateParams() const;

private:
    LinkageParams m_params;
    AssemblyMode m_assemblyMode = AssemblyMode::Closed;

    /**
     * @brief 计算球头A的坐标 (从 O₁ 出发)
     */
    void computeJointA(double theta1Deg, double &ax, double &ay) const;

    /**
     * @brief Newton-Raphson 核心迭代
     *
     * 求解方程 F(θ₂) = 0，其中 F 是闭环方程残差。
     *
     * @param theta1Rad 当前伺服臂转角（弧度）
     * @param initialGuess 迭代初值（弧度）
     * @return 收敛后的 θ₂（弧度），若不收敛返回 std::nullopt
     */
    std::optional<double> newtonRaphson(
        double theta1Rad,
        double initialGuess) const;

    /**
     * @brief 闭环方程残差
     *
     * F(θ₂) = |O₁A + AB|² - L²
     * 这是"B 点到 A 点距离减去连杆长度"的平方形式。
     */
    double residual(double theta2Rad, double ax, double ay) const;

    /**
     * @brief 残差对 θ₂ 的导数 dF/dθ₂（解析求导）
     */
    double residualDerivative(double theta2Rad, double ax, double ay) const;
};

#endif // KINEMATICSOLVER_H
