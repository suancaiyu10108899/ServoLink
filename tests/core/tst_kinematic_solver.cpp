#include <QtTest/QtTest>
#include <QDebug>
#include "KinematicSolver.h"
#include "LinkageParams.h"
#include "MechanismState.h"
#include "LinkageAnalysis.h"
#include "UnitSystem.h"

/**
 * @brief 运动学求解器单元测试
 *
 * 测试范围：
 * - 参数验证
 * - 正解（给定 θ₁ 求 θ₂）
 * - 全范围扫描
 * - 闭环约束检验（|AB| = L）
 * - 边界情况
 */
class TestKinematicSolver : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        // 使用典型舵面连杆参数
        m_defaultParams.servoArmRadius = 15.0;
        m_defaultParams.hornRadius = 18.0;
        m_defaultParams.linkLength = 35.0;
        m_defaultParams.baseDistance = 45.0;
        m_defaultParams.servoAngleMin = -60.0;
        m_defaultParams.servoAngleMax = 60.0;
    }

    // ── 参数验证 ──

    void testValidateParams_Valid()
    {
        KinematicSolver solver(m_defaultParams);
        auto result = solver.validateParams();
        QVERIFY(result.isSuccess());
    }

    void testValidateParams_Invalid_ZeroLength()
    {
        LinkageParams p = m_defaultParams;
        p.servoArmRadius = 0.0;
        KinematicSolver solver(p);
        auto result = solver.validateParams();
        QVERIFY(result.isFailure());
    }

    void testValidateParams_Invalid_NegativeLength()
    {
        LinkageParams p = m_defaultParams;
        p.linkLength = -5.0;
        KinematicSolver solver(p);
        auto result = solver.validateParams();
        QVERIFY(result.isFailure());
    }

    void testValidateParams_Invalid_AngleRange()
    {
        LinkageParams p = m_defaultParams;
        p.servoAngleMin = 30.0;
        p.servoAngleMax = -30.0;
        KinematicSolver solver(p);
        auto result = solver.validateParams();
        QVERIFY(result.isFailure());
    }

    // ── 正解 ──

    void testSolveForward_ZeroAngle()
    {
        KinematicSolver solver(m_defaultParams);
        auto state = solver.solveForward(0.0);
        QVERIFY(state.has_value());

        // 检查闭环约束：|AB| 应接近 L
        double L = m_defaultParams.linkLength;
        double computedL = UnitSystem::distance(
            state->jointA.x(), state->jointA.y(),
            state->jointB.x(), state->jointB.y());
        QVERIFY(qFuzzyCompare(computedL, L));

        // 输入角应为 0°
        QCOMPARE(state->inputAngle, 0.0);
    }

    void testSolveForward_PositiveAngle()
    {
        KinematicSolver solver(m_defaultParams);
        auto state = solver.solveForward(45.0);
        QVERIFY(state.has_value());

        // 闭环约束
        double L = m_defaultParams.linkLength;
        double computedL = UnitSystem::distance(
            state->jointA.x(), state->jointA.y(),
            state->jointB.x(), state->jointB.y());
        QVERIFY(qFuzzyCompare(computedL, L));

        // 输出角应有合理值（约在 150°-180° 范围，取决于装配模式）
        QVERIFY(state->outputAngle > 0.0);
    }

    void testSolveForward_NegativeAngle()
    {
        KinematicSolver solver(m_defaultParams);
        auto state = solver.solveForward(-45.0);
        QVERIFY(state.has_value());

        // 闭环约束
        double L = m_defaultParams.linkLength;
        double computedL = UnitSystem::distance(
            state->jointA.x(), state->jointA.y(),
            state->jointB.x(), state->jointB.y());
        QVERIFY(qFuzzyCompare(computedL, L));

        // 输出角应有合理值
        QVERIFY(state->outputAngle < 0.0);
    }

    void testSolveForward_TransmissionAngle_Valid()
    {
        KinematicSolver solver(m_defaultParams);
        auto state = solver.solveForward(0.0);
        QVERIFY(state.has_value());

        // 典型舵面机构传动角应该在 30°~90° 之间
        QVERIFY(state->transmissionAngle >= 30.0);
        QVERIFY(state->transmissionAngle <= 90.0);
    }

    void testSolveForward_Consistency()
    {
        // 正反解一致性：对 θ₁ 做正解得 θ₂，再对 θ₂ 做反解应该回到 θ₁
        KinematicSolver solver(m_defaultParams);

        double input = 30.0;
        auto fwd = solver.solveForward(input);
        QVERIFY(fwd.has_value());

        auto inv = solver.solveInverse(fwd->outputAngle);
        QVERIFY(inv.has_value());

        // 输入角应该接近（误差 < 0.5°）
        double err = qAbs(inv->inputAngle - input);
        QVERIFY2(err < 0.5,
                 qPrintable(QString("Input angle mismatch: %1 vs %2, err=%3")
                            .arg(input).arg(inv->inputAngle).arg(err)));
    }

    // ── 全范围扫描 ──

    void testSweepRange_ReturnsCorrectCount()
    {
        KinematicSolver solver(m_defaultParams);
        int steps = 100;
        auto analysis = solver.sweepRange(steps);
        QVERIFY(analysis.isAssemblyable);
        // 扫描点数应接近 steps（可能有少数不收敛）
        QVERIFY(analysis.sweepResults.size() >= steps * 0.95);
    }

    void testSweepRange_CalculatesDeflection()
    {
        KinematicSolver solver(m_defaultParams);
        auto analysis = solver.sweepRange(200);
        QVERIFY(analysis.isAssemblyable);
        QVERIFY(analysis.deflectionRange > 0.0);
        QVERIFY(analysis.maxDeflection > 0.0);
    }

    void testSweepRange_HasLinearityScore()
    {
        KinematicSolver solver(m_defaultParams);
        auto analysis = solver.sweepRange(200);
        QVERIFY(analysis.isAssemblyable);
        QVERIFY(analysis.linearityScore >= 0.0);
        QVERIFY(analysis.linearityScore <= 1.0);
    }

    void testSweepRange_MinTransmissionAngle()
    {
        KinematicSolver solver(m_defaultParams);
        auto analysis = solver.sweepRange(200);
        QVERIFY(analysis.isAssemblyable);
        QVERIFY(analysis.minTransmissionAngle > 0.0);
        QVERIFY(analysis.minTransmissionAngle <= 90.0);
    }

    // ── 边界情况 ──

    void testSolveForward_UnreasonableParams()
    {
        // 不合理参数：基距极小，两臂很大
        LinkageParams p;
        p.servoArmRadius = 30.0;
        p.hornRadius = 30.0;
        p.linkLength = 5.0;    // 连杆比两臂半径之和短
        p.baseDistance = 10.0;
        p.servoAngleMin = -60.0;
        p.servoAngleMax = 60.0;

        KinematicSolver solver(p);
        auto state = solver.solveForward(0.0);
        // 可能不收敛
        if (state.has_value()) {
            // 如果收敛了，闭环约束还是要满足
            double computedL = UnitSystem::distance(
                state->jointA.x(), state->jointA.y(),
                state->jointB.x(), state->jointB.y());
            QVERIFY(qFuzzyCompare(computedL, p.linkLength));
        }
    }

    void testSweepRange_NonAssemblyable()
    {
        LinkageParams p;
        p.servoArmRadius = 50.0;
        p.hornRadius = 5.0;
        p.linkLength = 5.0;
        p.baseDistance = 100.0;
        p.servoAngleMin = -60.0;
        p.servoAngleMax = 60.0;

        KinematicSolver solver(p);
        auto analysis = solver.sweepRange(50);
        QVERIFY(!analysis.isAssemblyable);
    }

    // ── 节点坐标 ──

    void testJointCoordinates_O2Fixed()
    {
        KinematicSolver solver(m_defaultParams);
        auto state = solver.solveForward(0.0);
        QVERIFY(state.has_value());

        // B 点到 O₂ 的距离应等于 r₂
        double d = m_defaultParams.baseDistance;
        double distO2B = UnitSystem::distance(
            d, 0.0,
            state->jointB.x(), state->jointB.y());
        double r2 = m_defaultParams.hornRadius;
        QVERIFY(qFuzzyCompare(distO2B, r2));
    }

    void testJointA_onCircle()
    {
        KinematicSolver solver(m_defaultParams);

        // 在多个角度验证 A 点在以 O₁ 为圆心、r₁ 为半径的圆上
        double testAngles[] = {-60, -30, 0, 30, 60};
        for (double angle : testAngles) {
            auto state = solver.solveForward(angle);
            QVERIFY(state.has_value());

            double distO1A = UnitSystem::distance(0, 0,
                state->jointA.x(), state->jointA.y());
            QVERIFY(qFuzzyCompare(distO1A, m_defaultParams.servoArmRadius));
        }
    }

private:
    LinkageParams m_defaultParams;
};

QTEST_APPLESS_MAIN(TestKinematicSolver)
#include "tst_kinematic_solver.moc"
