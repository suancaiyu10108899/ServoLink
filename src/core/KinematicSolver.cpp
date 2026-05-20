#include "KinematicSolver.h"
#include "UnitSystem.h"
#include <QtMath>
#include <QDebug>

// ── 构造 ──

KinematicSolver::KinematicSolver(const LinkageParams &params)
    : m_params(params)
    , m_assemblyMode(AssemblyMode::Closed)
{
}

void KinematicSolver::setParams(const LinkageParams &params)
{
    m_params = params;
}

// ── 参数验证 ──

OperationResult KinematicSolver::validateParams() const
{
    auto vr = m_params.validate();
    if (!vr.valid) {
        return OperationResult::failure(vr.message);
    }
    return OperationResult::success();
}

// ── 坐标计算 ──

void KinematicSolver::computeJointA(double theta1Deg, double &ax, double &ay) const
{
    // 球头A = O₁ + r₁ * (cos θ₁, sin θ₁)
    // O₁ 为原点 (0,0)
    double theta1Rad = UnitSystem::degToRad(theta1Deg);
    ax = m_params.servoArmRadius * cos(theta1Rad);
    ay = m_params.servoArmRadius * sin(theta1Rad);
}

// ── 闭环残差 ──

/**
 * 数学推导：
 *
 * 给定 θ₁（已知 → A 点坐标确定），设 θ₂ 为未知。
 * B 点坐标：O₂ + r₂·(cos θ₂, sin θ₂)
 *          = (d + r₂·cos θ₂,  0 + r₂·sin θ₂)
 *
 * 连杆长度约束：|AB|² = L²
 *   F(θ₂) = |B - A|² - L² = 0
 *
 * 展开：
 *   F = (d + r₂·cosθ₂ - a_x)² + (r₂·sinθ₂ - a_y)² - L²
 *     = (d - a_x)² + a_y² + r₂² + 2r₂[(d-a_x)cosθ₂ - a_y·sinθ₂] - L²
 *     = const + 2r₂[(d-a_x)cosθ₂ - a_y·sinθ₂]
 *
 *   const = (d - a_x)² + a_y² + r₂² - L²
 *
 * 这恰好可以化成 F = const + C·cos(θ₂ + ψ) 的形式，
 * 解析解存在，但我们用迭代法以保证通用性和可扩展性。
 */
double KinematicSolver::residual(double theta2Rad, double ax, double ay) const
{
    double r2 = m_params.hornRadius;
    double d = m_params.baseDistance;
    double L = m_params.linkLength;
    double baseRad = UnitSystem::degToRad(m_params.baseAngle);
    double ox2 = d * cos(baseRad);
    double oy2 = d * sin(baseRad);

    double bx = ox2 + r2 * cos(theta2Rad);
    double by = oy2 + r2 * sin(theta2Rad);

    double dx = bx - ax;
    double dy = by - ay;

    return dx * dx + dy * dy - L * L;
}

/**
 * 解析求导 dF/dθ₂：
 *
 * F = (Bx - Ax)² + (By - Ay)² - L²
 *
 * dBx/dθ₂ = -r₂·sinθ₂
 * dBy/dθ₂ =  r₂·cosθ₂
 *
 * dF/dθ₂ = 2(Bx-Ax)(-r₂·sinθ₂) + 2(By-Ay)(r₂·cosθ₂)
 *        = 2r₂ [ -(Bx-Ax)sinθ₂ + (By-Ay)cosθ₂ ]
 */
double KinematicSolver::residualDerivative(double theta2Rad, double ax, double ay) const
{
    double r2 = m_params.hornRadius;
    double d = m_params.baseDistance;
    double baseRad = UnitSystem::degToRad(m_params.baseAngle);
    double ox2 = d * cos(baseRad);
    double oy2 = d * sin(baseRad);

    double bx = ox2 + r2 * cos(theta2Rad);
    double by = oy2 + r2 * sin(theta2Rad);

    double dx = bx - ax;
    double dy = by - ay;

    return 2.0 * r2 * (-dx * sin(theta2Rad) + dy * cos(theta2Rad));
}

// ── Newton-Raphson 迭代 ──

std::optional<double> KinematicSolver::newtonRaphson(
    double theta1Rad,
    double initialGuess) const
{
    double ax = m_params.servoArmRadius * cos(theta1Rad);
    double ay = m_params.servoArmRadius * sin(theta1Rad);

    double theta2 = initialGuess;

    const int maxIterations = 50;
    const double tolerance = 1e-10;      // 残差容限 (mm²)
    const double angleTolerance = 1e-8;  // 角度容限 (rad)

    for (int iter = 0; iter < maxIterations; ++iter) {
        double f = residual(theta2, ax, ay);
        if (fabs(f) < tolerance) {
            return theta2;  // 收敛
        }

        double df = residualDerivative(theta2, ax, ay);
        if (fabs(df) < 1e-12) {
            // 导数为零 → 奇异点，Newton 法失效
            qDebug() << "[KinematicSolver] Derivative near zero at theta2 ="
                     << UnitSystem::radToDeg(theta2) << "deg";
            return std::nullopt;
        }

        double delta = -f / df;

        // 阻尼：防止大步长跳过解
        double maxStep = 0.5;  // 最大步长 0.5 rad ≈ 28.6°
        if (fabs(delta) > maxStep) {
            delta = (delta > 0) ? maxStep : -maxStep;
        }

        theta2 += delta;

        // 检查角度收敛
        if (fabs(delta) < angleTolerance) {
            return theta2;
        }
    }

    qDebug() << "[KinematicSolver] Newton-Raphson did not converge after"
             << maxIterations << "iterations";
    return std::nullopt;
}

// ── 正解 ──

std::optional<MechanismState> KinematicSolver::solveForward(double inputAngleDeg) const
{
    double theta1Rad = UnitSystem::degToRad(inputAngleDeg);
    double ax, ay;
    computeJointA(inputAngleDeg, ax, ay);

    double d = m_params.baseDistance;
    double r2 = m_params.hornRadius;
    double L = m_params.linkLength;

    // ── 初值估计 ──
    // 假定连杆 L 远长于 r₁ 和 r₂，则 θ₂ ≈ π - θ₁（近似反向）
    // 更精确的几何初值：
    // O₂ 坐标受基距倾角影响
    double baseRad = UnitSystem::degToRad(m_params.baseAngle);
    double ox2 = d * cos(baseRad);
    double oy2 = d * sin(baseRad);

    double distO2ToA = UnitSystem::distance(ax, ay, ox2, oy2);

    // 三角形 O₂-A-B：已知 O₂A、AB(=L)、O₂B(=r₂)
    // 用余弦定理求 ∠AO₂B
    double cosAOB = (distO2ToA * distO2ToA + r2 * r2 - L * L)
                    / (2.0 * distO2ToA * r2);
    // 夹逼到 [-1, 1]
    if (cosAOB > 1.0) cosAOB = 1.0;
    if (cosAOB < -1.0) cosAOB = -1.0;

    double angleAO2B = acos(cosAOB);

    // O₂A 的方向角
    double angleO2A = atan2(ay - oy2, ax - ox2);  // 从 O₂ 指向 A

    // 根据装配模式选择初值的符号
    // 闭式 (Closed)：B 在 O₂A 的"下方" → 同侧
    // 开式 (Open) ：B 在 O₂A 的"上方" → 对侧
    double initialTheta2 = (m_assemblyMode == AssemblyMode::Closed)
        ? angleO2A - angleAO2B
        : angleO2A + angleAO2B;

    // 迭代
    auto result = newtonRaphson(theta1Rad, initialTheta2);
    if (!result.has_value()) {
        return std::nullopt;
    }

    double theta2Rad = result.value();

    // ── 组装完整状态 ──
    MechanismState state;
    state.inputAngle = inputAngleDeg;
    state.outputAngle = UnitSystem::radToDeg(theta2Rad);
    state.jointA = QPointF(ax, ay);

    // B 点坐标
    double bx = ox2 + r2 * cos(theta2Rad);
    double by = oy2 + r2 * sin(theta2Rad);
    state.jointB = QPointF(bx, by);

    // 传动角 = 连杆 AB 与舵角 O₂B 的夹角
    double linkDx = bx - ax;
    double linkDy = by - ay;
    double hornDx = r2 * cos(theta2Rad);
    double hornDy = r2 * sin(theta2Rad);

    state.transmissionAngle = UnitSystem::angleBetweenVectors(
        linkDx, linkDy, hornDx, hornDy);

    // 力放大比（机械利益）：M = (r₁ / r₂) * (sin(传动角) / sin(连杆与r₁夹角))
    // 简化：M ≈ r₁/r₂ * sin(传动角) / sin(α)，α是连杆与伺服臂的夹角
    double servoDx = ax;
    double servoDy = ay;
    double alpha = UnitSystem::angleBetweenVectors(
        -linkDx, -linkDy, servoDx, servoDy);  // 连杆与伺服臂的夹角
    double sinTrans = sin(UnitSystem::degToRad(state.transmissionAngle));
    double sinAlpha = sin(UnitSystem::degToRad(alpha));
    if (sinAlpha > 1e-6) {
        state.mechanicalAdvantage = (m_params.servoArmRadius / r2)
                                    * (sinTrans / sinAlpha);
    } else {
        state.mechanicalAdvantage = 10.0;  // 奇异点附近取大值
    }

    return state;
}

// ── 解析正解 ──

std::optional<std::pair<MechanismState, MechanismState>>
KinematicSolver::solveForwardAnalytic(double inputAngleDeg) const
{
    double theta1Rad = UnitSystem::degToRad(inputAngleDeg);
    double ax = m_params.servoArmRadius * cos(theta1Rad);
    double ay = m_params.servoArmRadius * sin(theta1Rad);
    double d  = m_params.baseDistance;
    double r2 = m_params.hornRadius;

    // ── 解析系数推导 ──
    // 闭环方程展开（含基距倾角）：
    // O₂ = (d·cosα, d·sinα)
    // |A + O₂ - O₁|² = L² → |B - A|² = L²
    // → A + B·cosθ₂ + C·sinθ₂ = 0
    double baseRad = UnitSystem::degToRad(m_params.baseAngle);
    double ox2 = d * cos(baseRad);
    double oy2 = d * sin(baseRad);
    double r1 = m_params.servoArmRadius;
    double L  = m_params.linkLength;
    double A = ox2*ox2 + oy2*oy2 + r1*r1 + r2*r2 - L*L
               - 2.0 * r1 * (ox2 * cos(theta1Rad) + oy2 * sin(theta1Rad));
    double B = 2.0 * r2 * (ox2 - r1 * cos(theta1Rad));
    double C = 2.0 * r2 * (oy2 - r1 * sin(theta1Rad));

    double norm = sqrt(B*B + C*C);
    if (norm < 1e-12) {
        // 机构退化（B和C同时为零）：杆件共线
        return std::nullopt;
    }

    double cosArg = -A / norm;
    if (cosArg > 1.0) cosArg = 1.0;
    if (cosArg < -1.0) cosArg = -1.0;
    double delta = acos(cosArg);    // 0 ≤ δ ≤ π

    double phi = atan2(-C, -B);

    // 两解
    double theta2cRad = phi + delta;   // 闭式（同侧）
    double theta2oRad = phi - delta;   // 开式（对侧）

    // ── 构造 MechanismState ──
    auto buildState = [&](double t2Rad) {
        MechanismState s;
        s.inputAngle = inputAngleDeg;
        s.outputAngle = UnitSystem::radToDeg(t2Rad);
        s.jointA = QPointF(ax, ay);
        double bx = ox2 + r2 * cos(t2Rad);
        double by = oy2 + r2 * sin(t2Rad);
        s.jointB = QPointF(bx, by);
        double linkDx = bx - ax, linkDy = by - ay;
        double hornDx = r2 * cos(t2Rad), hornDy = r2 * sin(t2Rad);
        s.transmissionAngle = UnitSystem::angleBetweenVectors(linkDx, linkDy, hornDx, hornDy);
        double alpha = UnitSystem::angleBetweenVectors(-linkDx, -linkDy, ax, ay);
        double sinTrans = sin(UnitSystem::degToRad(s.transmissionAngle));
        double sinAlpha = sin(UnitSystem::degToRad(alpha));
        s.mechanicalAdvantage = (sinAlpha > 1e-6)
            ? (m_params.servoArmRadius / r2) * (sinTrans / sinAlpha) : 10.0;
        return s;
    };

    return std::make_pair(buildState(theta2cRad), buildState(theta2oRad));
}

// ── 反解 ──

std::optional<MechanismState> KinematicSolver::solveInverse(double outputAngleDeg) const
{
    // 反解：已知 θ₂ 求 θ₁
    // 与正解对称——交换 r₁↔r₂ 并镜像坐标系即可
    double r1 = m_params.servoArmRadius;
    double r2 = m_params.hornRadius;
    double d = m_params.baseDistance;
    double L = m_params.linkLength;

    double theta2Rad = UnitSystem::degToRad(outputAngleDeg);
    double bx = d + r2 * cos(theta2Rad);
    double by = r2 * sin(theta2Rad);

    // 三角形 B-O₁-A：已知 BO₁、O₁A(=r₁)、BA(=L)
    double distO1ToB = UnitSystem::distance(0.0, 0.0, bx, by);

    double cosBO1A = (distO1ToB * distO1ToB + r1 * r1 - L * L)
                     / (2.0 * distO1ToB * r1);
    if (cosBO1A > 1.0) cosBO1A = 1.0;
    if (cosBO1A < -1.0) cosBO1A = -1.0;

    double angleBO1A = acos(cosBO1A);
    double angleO1B = atan2(by, bx);

    // 闭式装配
    double theta1Rad = angleO1B + angleBO1A;
    double theta1Deg = UnitSystem::radToDeg(theta1Rad);

    // 用正解验证
    auto fwd = solveForward(theta1Deg);
    if (!fwd.has_value()) {
        return std::nullopt;
    }

    MechanismState state = fwd.value();

    // 验证输出角是否接近目标
    double err = fabs(state.outputAngle - outputAngleDeg);
    if (err > 0.1) {
        // 尝试另一种装配模式
        theta1Rad = angleO1B - angleBO1A;
        theta1Deg = UnitSystem::radToDeg(theta1Rad);
        fwd = solveForward(theta1Deg);
        if (fwd.has_value()) {
            state = fwd.value();
            err = fabs(state.outputAngle - outputAngleDeg);
        }
    }

    if (err > 0.1) {
        qDebug() << "[KinematicSolver] Inverse: output mismatch by" << err << "deg";
    }

    return state;
}

// ── 全范围扫描 ──

LinkageAnalysis KinematicSolver::sweepRange(int steps) const
{
    LinkageAnalysis analysis;
    analysis.params = m_params;

    auto vr = m_params.validate();
    if (!vr.valid) {
        analysis.isAssemblyable = false;
        return analysis;
    }

    analysis.isAssemblyable = true;

    double minAngle = m_params.servoAngleMin;
    double maxAngle = m_params.servoAngleMax;
    double stepSize = (maxAngle - minAngle) / (steps - 1);

    double maxDeflection = -1e9;
    double minDeflection = 1e9;
    double minTrans = 90.0;
    bool foundDeadPoint = false;

    // 线性度评估：收集 (θ₁, θ₂) 对
    QVector<QPair<double, double>> anglePairs;

    for (int i = 0; i < steps; ++i) {
        double theta1 = minAngle + i * stepSize;
        auto state = solveForward(theta1);

        if (state.has_value()) {
            MechanismState s = state.value();
            analysis.sweepResults.append(s);

            anglePairs.append({s.inputAngle, s.outputAngle});

            if (s.outputAngle > maxDeflection) maxDeflection = s.outputAngle;
            if (s.outputAngle < minDeflection) minDeflection = s.outputAngle;
            if (s.transmissionAngle < minTrans) minTrans = s.transmissionAngle;
            if (s.isNearDeadPoint()) foundDeadPoint = true;
        }
    }

    analysis.maxDeflection = maxDeflection;
    analysis.deflectionRange = maxDeflection - minDeflection;
    analysis.minTransmissionAngle = minTrans;
    analysis.hasDeadPoint = foundDeadPoint;

    // ── 计算线性度评分 ──
    // 对 (θ₁, θ₂) 做最小二乘线性拟合，
    // 评分 = 1 - (残差标准差 / 输出范围)
    // 完全线性时 R²=1，评分=1
    if (anglePairs.size() >= 3) {
        int n = anglePairs.size();
        double sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0, sumY2 = 0;
        for (const auto &pair : anglePairs) {
            double x = pair.first;
            double y = pair.second;
            sumX += x;
            sumY += y;
            sumXY += x * y;
            sumX2 += x * x;
            sumY2 += y * y;
        }
        double denom = n * sumX2 - sumX * sumX;
        if (fabs(denom) > 1e-10) {
            double slope = (n * sumXY - sumX * sumY) / denom;
            double intercept = (sumY - slope * sumX) / n;

            // 计算残差平方和
            double ssRes = 0.0;
            double ssTot = 0.0;
            double meanY = sumY / n;
            for (const auto &pair : anglePairs) {
                double yPred = slope * pair.first + intercept;
                double resid = pair.second - yPred;
                ssRes += resid * resid;
                double dev = pair.second - meanY;
                ssTot += dev * dev;
            }
            if (ssTot > 1e-10) {
                double rSquared = 1.0 - ssRes / ssTot;
                // 限制为 [0, 1]
                if (rSquared < 0.0) rSquared = 0.0;
                if (rSquared > 1.0) rSquared = 1.0;
                analysis.linearityScore = rSquared;
            } else {
                analysis.linearityScore = 1.0;  // 输出恒定时无意义，给满分
            }
        } else {
            analysis.linearityScore = 0.0;
        }
    } else {
        analysis.linearityScore = 0.0;
    }

    return analysis;
}
