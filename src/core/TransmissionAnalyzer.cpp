#include "TransmissionAnalyzer.h"
#include "KinematicSolver.h"
#include "UnitSystem.h"
#include <algorithm>
#include <array>

// ── Grashof 分类 ──

TransmissionAnalyzer::GrashofType TransmissionAnalyzer::classifyGrashof(
    const LinkageParams &params)
{
    double r1 = params.servoArmRadius;
    double L  = params.linkLength;
    double r2 = params.hornRadius;
    double d  = params.baseDistance;

    std::array<double, 4> sides = {r1, L, r2, d};
    std::sort(sides.begin(), sides.end());

    double s = sides[0];  // 最短
    double l = sides[3];  // 最长

    double sum_sl = s + l;
    double sum_pq = sides[1] + sides[2];

    using GT = GrashofType;

    // 判断 Grashof 条件
    if (qFuzzyCompare(sum_sl, sum_pq)) {
        return GT::ChangePoint;
    }
    if (sum_sl > sum_pq) {
        return GT::NonGrashof;
    }

    // sum_sl < sum_pq → Grashof 机构
    // 最短杆的位置决定类型：
    if (qFuzzyCompare(s, d)) {
        // 最短杆是机架 → 双曲柄
        return GT::DoubleCrank;
    }
    if (qFuzzyCompare(s, L)) {
        // 最短杆是连杆 → 双摇杆
        return GT::DoubleRocker;
    }
    // 最短杆是连架杆（r₁ 或 r₂）→ 曲柄摇杆
    return GT::CrankRocker;
}

QString TransmissionAnalyzer::grashofTypeName(GrashofType type)
{
    switch (type) {
    case GrashofType::CrankRocker:  return QString("曲柄摇杆机构");
    case GrashofType::DoubleCrank:  return QString("双曲柄机构");
    case GrashofType::DoubleRocker: return QString("双摇杆机构");
    case GrashofType::NonGrashof:   return QString("非Grashof（三摇杆）");
    case GrashofType::ChangePoint:  return QString("变点机构");
    case GrashofType::Unknown:      return QString("未知");
    }
    return QString("未知");
}

// ── 快速评估 ──

TransmissionAnalyzer::QuickAssessment TransmissionAnalyzer::quickAssess(
    const LinkageParams &params)
{
    QuickAssessment qa;
    qa.grashofType = classifyGrashof(params);

    double r1 = params.servoArmRadius;
    double r2 = params.hornRadius;
    double L  = params.linkLength;
    double d  = params.baseDistance;

    // 传动角 μ 的粗略估计：
    // 机构在运动中，传动角在 [μ_min, μ_max] 间变化。
    // 传动角是连杆与舵角的夹角。
    //
    // 粗略公式（来自机构学教科书）：
    // cos μ_max = (L² + r₂² - (d + r₁)²) / (2·L·r₂)
    // cos μ_min = (L² + r₂² - (d - r₁)²) / (2·L·r₂)
    // 但这只针对特定位置成立。
    //
    // 更简化的工程估算：
    // 最危险位置发生在 O₁、A、O₂ 共线时。
    // 用 KinematicSolver 取几个关键点检查。

    KinematicSolver solver(params);

    // 检查几个关键角度
    double checkAngles[] = {
        params.servoAngleMin,
        params.servoAngleMin * 0.5,
        0.0,
        params.servoAngleMax * 0.5,
        params.servoAngleMax
    };

    double minTrans = 90.0;
    bool deadPoint = false;

    for (double angle : checkAngles) {
        auto state = solver.solveForward(angle);
        if (state.has_value()) {
            double trans = state.value().transmissionAngle;
            if (trans < minTrans) minTrans = trans;
            if (state.value().isNearDeadPoint()) deadPoint = true;
        }
    }

    qa.estimatedMinTransAngle = minTrans;
    qa.likelyHasDeadPoint = deadPoint;

    // 生成摘要
    QStringList parts;
    parts << grashofTypeName(qa.grashofType);
    parts << QString("估测最小传动角 %1°").arg(minTrans, 0, 'f', 1);

    if (minTrans >= 30.0) {
        parts << "传动质量：良好";
    } else if (minTrans >= 20.0) {
        parts << "传动质量：一般";
    } else {
        parts << "传动质量：差，存在死点风险";
    }
    if (deadPoint) {
        parts << "⚠ 检测到近死点位置";
    }

    qa.summary = parts.join(" | ");

    return qa;
}

// ── 改进建议 ──

QStringList TransmissionAnalyzer::improvementSuggestions(
    const LinkageParams &params,
    const LinkageAnalysis &analysis)
{
    QStringList suggestions;

    // 传动角不足
    if (analysis.minTransmissionAngle < 30.0) {
        suggestions << QString(
            "最小传动角仅 %1°，建议加大连杆长度或减小舵角半径以改善传动角")
            .arg(analysis.minTransmissionAngle, 0, 'f', 1);

        // 更具体的建议
        double tol = 5.0;
        if (analysis.minTransmissionAngle < 15.0) {
            // 假设增加连杆长度可以改善
            double suggestL = params.linkLength * 1.2;
            suggestions << QString(
                "尝试将连杆长度 L 从 %1 mm 增加到约 %2 mm")
                .arg(params.linkLength, 0, 'f', 1)
                .arg(suggestL, 0, 'f', 1);
        }
        if (params.servoArmRadius > params.hornRadius * 0.9) {
            suggestions << QString(
                "伺服臂半径 r₁ (%1 mm) 接近或超过舵角半径 r₂ (%2 mm)，"
                "建议适当减小伺服臂半径")
                .arg(params.servoArmRadius, 0, 'f', 1)
                .arg(params.hornRadius, 0, 'f', 1);
        }
    }

    // 死点
    if (analysis.hasDeadPoint) {
        suggestions << QString(
            "机构存在死点位置！建议减小伺服臂工作范围或调整杆长比");
    }

    // 线性度差
    if (analysis.linearityScore < 0.85) {
        suggestions << QString(
            "输入-输出线性度较低 (R²=%1)，"
            "如需线性响应，建议调整 r₁/r₂ 的比例")
            .arg(analysis.linearityScore, 0, 'f', 3);
    }

    // 机构类型建议
    auto grashof = classifyGrashof(params);
    if (grashof == GrashofType::NonGrashof) {
        suggestions << QString(
            "当前为三摇杆机构（无整周回转），伺服臂不能360°旋转。"
            "这在舵面应用中通常可接受，但需确保工作范围内无死点。");
    }

    // 如果没有问题
    if (suggestions.isEmpty()) {
        suggestions << QString("当前参数传动性能良好，无需调整。");
    }

    return suggestions;
}
