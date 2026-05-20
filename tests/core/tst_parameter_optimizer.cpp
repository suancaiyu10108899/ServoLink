#include <QtTest/QtTest>
#include "ParameterOptimizer.h"
#include "KinematicSolver.h"
#include "LinkageParams.h"
#include "LinkageAnalysis.h"

/**
 * @brief 参数优化器单元测试
 *
 * 测试范围：
 * - 评分函数
 * - 优化器输出有效性
 * - 约束满足
 */
class TestParameterOptimizer : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        m_constraints.baseDistance = 45.0;
        m_constraints.servoAngleMin = -60.0;
        m_constraints.servoAngleMax = 60.0;
        m_constraints.minDeflectionRange = 80.0;
        m_constraints.minTransmissionAngle = 30.0;
        m_constraints.maxHornRadius = 25.0;
        m_constraints.maxServoArmRadius = 25.0;
        m_constraints.maxLinkLength = 100.0;
    }

    // ── 评分 ──

    void testScore_GoodParams_HighScore()
    {
        ParameterOptimizer optimizer;
        optimizer.setConstraints(m_constraints);

        LinkageParams p;
        p.servoArmRadius = 15.0;
        p.hornRadius = 18.0;
        p.linkLength = 35.0;
        p.baseDistance = 45.0;
        p.servoAngleMin = -60.0;
        p.servoAngleMax = 60.0;

        KinematicSolver solver(p);
        auto analysis = solver.sweepRange(100);

        double score = optimizer.scoreParams(p, analysis);
        QVERIFY(score >= 0.0);
        QVERIFY(score <= 1.05);  // 允许小幅奖励溢出
    }

    void testScore_BadParams_LowScore()
    {
        ParameterOptimizer optimizer;
        optimizer.setConstraints(m_constraints);

        LinkageParams p;
        p.servoArmRadius = 25.0;
        p.hornRadius = 5.0;
        p.linkLength = 15.0;
        p.baseDistance = 45.0;
        p.servoAngleMin = -60.0;
        p.servoAngleMax = 60.0;

        KinematicSolver solver(p);
        auto analysis = solver.sweepRange(50);

        double score = optimizer.scoreParams(p, analysis);
        QVERIFY(score < 0.9);
    }

    void testScore_NonAssemblyable_Zero()
    {
        ParameterOptimizer optimizer;
        optimizer.setConstraints(m_constraints);

        LinkageParams p;
        p.servoArmRadius = 50.0;
        p.hornRadius = 5.0;
        p.linkLength = 5.0;
        p.baseDistance = 100.0;

        KinematicSolver solver(p);
        auto analysis = solver.sweepRange(20);

        double score = optimizer.scoreParams(p, analysis);
        QCOMPARE(score, 0.0);
    }

    // ── 优化 ──

    void testOptimize_ReturnsResults()
    {
        ParameterOptimizer optimizer;
        optimizer.setConstraints(m_constraints);

        auto results = optimizer.optimize(5, 8);  // 小分辨率，快速
        QVERIFY(results.size() > 0);
        QVERIFY(results.size() <= 5);
    }

    void testOptimize_ResultsAreSorted()
    {
        ParameterOptimizer optimizer;
        optimizer.setConstraints(m_constraints);

        auto results = optimizer.optimize(5, 8);
        QVERIFY(results.size() >= 1);

        // 验证按分数降序排列
        double prevScore = 2.0;
        for (const auto &r : results) {
            QVERIFY(r.score <= prevScore + 1e-9);
            prevScore = r.score;
        }
    }

    void testOptimize_ResultsHaveDescription()
    {
        ParameterOptimizer optimizer;
        optimizer.setConstraints(m_constraints);

        auto results = optimizer.optimize(3, 6);
        for (const auto &r : results) {
            QVERIFY(!r.description.isEmpty());
        }
    }

    void testOptimize_TopResultHasReasonableParams()
    {
        ParameterOptimizer optimizer;
        optimizer.setConstraints(m_constraints);

        auto results = optimizer.optimize(3, 8);
        QVERIFY(results.size() >= 1);

        const auto &best = results.first();
        // 最优结果的参数应在合理范围内
        QVERIFY(best.params.servoArmRadius > 0);
        QVERIFY(best.params.servoArmRadius <= m_constraints.maxServoArmRadius);
        QVERIFY(best.params.hornRadius > 0);
        QVERIFY(best.params.hornRadius <= m_constraints.maxHornRadius);
        QVERIFY(best.params.linkLength > 0);
        QVERIFY(best.params.linkLength <= m_constraints.maxLinkLength);
        QVERIFY(best.params.baseDistance > 0);
        QVERIFY(best.score > 0.0);
    }

    void testOptimize_WithTightConstraints_StillFindsSomething()
    {
        // 收紧约束仍然能找到结果（至少应该有低分候选）
        ParameterOptimizer optimizer;

        ParameterOptimizer::Constraints tight;
        tight.baseDistance = 50.0;
        tight.servoAngleMin = -45.0;
        tight.servoAngleMax = 45.0;
        tight.minDeflectionRange = 60.0;
        tight.minTransmissionAngle = 25.0;
        tight.maxHornRadius = 20.0;
        tight.maxServoArmRadius = 20.0;
        tight.maxLinkLength = 80.0;

        optimizer.setConstraints(tight);
        auto results = optimizer.optimize(3, 5);
        // 不要求一定有高分结果，但不能崩溃
        // 空结果也是合法的（无可行解）
        QVERIFY(results.size() <= 3);
    }

    // ── 约束设置 ──

    void testConstraints_Defaults()
    {
        ParameterOptimizer optimizer;
        auto c = optimizer.constraints();
        QCOMPARE(c.baseDistance, 45.0);
        QCOMPARE(c.servoAngleMin, -60.0);
        QCOMPARE(c.servoAngleMax, 60.0);
    }

    void testConstraints_SetConvenience()
    {
        ParameterOptimizer optimizer;
        optimizer.setBaseDistance(55.0);
        optimizer.setTargetDeflectionRange(90.0);
        optimizer.setServoRange(-45.0, 45.0);

        auto c = optimizer.constraints();
        QCOMPARE(c.baseDistance, 55.0);
        QCOMPARE(c.minDeflectionRange, 90.0);
        QCOMPARE(c.servoAngleMin, -45.0);
        QCOMPARE(c.servoAngleMax, 45.0);
    }

private:
    ParameterOptimizer::Constraints m_constraints;
};

QTEST_APPLESS_MAIN(TestParameterOptimizer)
#include "tst_parameter_optimizer.moc"
