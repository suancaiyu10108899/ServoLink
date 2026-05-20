#include <QtTest/QtTest>
#include "TransmissionAnalyzer.h"
#include "KinematicSolver.h"
#include "LinkageParams.h"

/**
 * @brief 传动质量分析器单元测试
 *
 * 测试范围：
 * - Grashof 类型判定
 * - 快速评估
 * - 改进建议生成
 */
class TestTransmissionAnalyzer : public QObject
{
    Q_OBJECT

private slots:
    // ── Grashof 分类 ──

    void testGrashof_CrankRocker()
    {
        // 最短杆为连架杆（r₁ 或 r₂），且 s+l < p+q → 曲柄摇杆
        LinkageParams p;
        p.servoArmRadius = 10.0;   // 最短
        p.hornRadius = 20.0;
        p.linkLength = 30.0;       // 最长
        p.baseDistance = 25.0;
        // s=10,l=30; s+l=40; p+q=45; s+l<p+q; 最短=连架杆 → CrankRocker

        auto type = TransmissionAnalyzer::classifyGrashof(p);
        QCOMPARE(type, TransmissionAnalyzer::GrashofType::CrankRocker);
    }

    void testGrashof_DoubleCrank()
    {
        // 最短杆为机架（d），且 s+l < p+q → 双曲柄
        LinkageParams p;
        p.servoArmRadius = 20.0;
        p.hornRadius = 25.0;
        p.linkLength = 30.0;
        p.baseDistance = 10.0;     // 最短 → 机架
        // s=10,l=30; s+l=40; p+q=45; s+l<p+q; 最短=机架 → DoubleCrank

        auto type = TransmissionAnalyzer::classifyGrashof(p);
        QCOMPARE(type, TransmissionAnalyzer::GrashofType::DoubleCrank);
    }

    void testGrashof_DoubleRocker()
    {
        // 最短杆为连杆（L），且 s+l < p+q → 双摇杆
        LinkageParams p;
        p.servoArmRadius = 20.0;
        p.hornRadius = 25.0;
        p.linkLength = 10.0;       // 最短 → 连杆
        p.baseDistance = 30.0;     // 最长
        // s=10,l=30; s+l=40; p+q=45; s+l<p+q; 最短=连杆 → DoubleRocker

        auto type = TransmissionAnalyzer::classifyGrashof(p);
        QCOMPARE(type, TransmissionAnalyzer::GrashofType::DoubleRocker);
    }

    void testGrashof_NonGrashof()
    {
        // s + l > p + q → 非 Grashof
        LinkageParams p;
        p.servoArmRadius = 30.0;
        p.hornRadius = 30.0;
        p.linkLength = 45.0;       // 最长
        p.baseDistance = 20.0;     // 最短

        // s + l = 20 + 45 = 65, p + q = 30 + 30 = 60, 65 > 60
        auto type = TransmissionAnalyzer::classifyGrashof(p);
        QCOMPARE(type, TransmissionAnalyzer::GrashofType::NonGrashof);
    }

    void testGrashof_ChangePoint()
    {
        // s + l = p + q → 变点机构
        LinkageParams p;
        p.servoArmRadius = 20.0;
        p.hornRadius = 20.0;
        p.linkLength = 25.0;
        p.baseDistance = 15.0;     // 最短

        // s + l = 15 + 25 = 40, p + q = 20 + 20 = 40
        auto type = TransmissionAnalyzer::classifyGrashof(p);
        QCOMPARE(type, TransmissionAnalyzer::GrashofType::ChangePoint);
    }

    void testGrashof_TypeNames()
    {
        // 所有类型都有非空名称
        QStringList names;
        names << TransmissionAnalyzer::grashofTypeName(
            TransmissionAnalyzer::GrashofType::CrankRocker);
        names << TransmissionAnalyzer::grashofTypeName(
            TransmissionAnalyzer::GrashofType::DoubleCrank);
        names << TransmissionAnalyzer::grashofTypeName(
            TransmissionAnalyzer::GrashofType::DoubleRocker);
        names << TransmissionAnalyzer::grashofTypeName(
            TransmissionAnalyzer::GrashofType::NonGrashof);
        names << TransmissionAnalyzer::grashofTypeName(
            TransmissionAnalyzer::GrashofType::ChangePoint);
        names << TransmissionAnalyzer::grashofTypeName(
            TransmissionAnalyzer::GrashofType::Unknown);

        for (const auto &name : names) {
            QVERIFY(!name.isEmpty());
        }
    }

    // ── 快速评估 ──

    void testQuickAssess_StandardParams()
    {
        LinkageParams p;
        p.servoArmRadius = 15.0;
        p.hornRadius = 18.0;
        p.linkLength = 35.0;
        p.baseDistance = 45.0;
        p.servoAngleMin = -60.0;
        p.servoAngleMax = 60.0;

        auto qa = TransmissionAnalyzer::quickAssess(p);
        QVERIFY(!qa.summary.isEmpty());
        // 标准参数应该没有死点
        QVERIFY(!qa.likelyHasDeadPoint);
    }

    void testQuickAssess_BadParams()
    {
        // 传动角差的参数（但仍可求解）
        LinkageParams p;
        p.servoArmRadius = 20.0;
        p.hornRadius = 8.0;
        p.linkLength = 50.0;
        p.baseDistance = 45.0;
        p.servoAngleMin = -60.0;
        p.servoAngleMax = 60.0;

        auto qa = TransmissionAnalyzer::quickAssess(p);
        QVERIFY(!qa.summary.isEmpty());
        // 无论求解是否成功，不应崩溃
        QVERIFY(qa.estimatedMinTransAngle >= 0.0);
    }

    // ── 改进建议 ──

    void testImprovementSuggestions_GoodParams()
    {
        // 即使参数较好，函数也应正常运行不崩溃并返回结果
        LinkageParams p;
        p.servoArmRadius = 12.0;
        p.hornRadius = 20.0;
        p.linkLength = 50.0;
        p.baseDistance = 45.0;
        p.servoAngleMin = -45.0;
        p.servoAngleMax = 45.0;

        KinematicSolver solver(p);
        auto analysis = solver.sweepRange(100);
        QVERIFY(analysis.isAssemblyable);

        auto suggestions = TransmissionAnalyzer::improvementSuggestions(p, analysis);
        // 核心验证：函数不崩溃且返回结果
        QVERIFY(!suggestions.isEmpty());
        // 只要不为空就说明逻辑正常执行了
    }

    void testImprovementSuggestions_BadParams()
    {
        LinkageParams p;
        p.servoArmRadius = 25.0;
        p.hornRadius = 10.0;
        p.linkLength = 10.0;
        p.baseDistance = 45.0;
        p.servoAngleMin = -60.0;
        p.servoAngleMax = 60.0;

        KinematicSolver solver(p);
        auto analysis = solver.sweepRange(50);

        auto suggestions = TransmissionAnalyzer::improvementSuggestions(p, analysis);
        QVERIFY(!suggestions.isEmpty());

        // 坏参数应包含改进建议
        bool hasSuggestion = false;
        for (const auto &s : suggestions) {
            if (s.contains("建议") || s.contains("调整") || s.contains("增大")) {
                hasSuggestion = true;
                break;
            }
        }
        QVERIFY(hasSuggestion);
    }
};

QTEST_APPLESS_MAIN(TestTransmissionAnalyzer)
#include "tst_transmission_analyzer.moc"
