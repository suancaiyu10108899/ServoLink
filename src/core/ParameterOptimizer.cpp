#include "ParameterOptimizer.h"
#include "UnitSystem.h"
#include <QtMath>
#include <QDebug>
#include <algorithm>

void ParameterOptimizer::setConstraints(const Constraints &constraints)
{
    m_constraints = constraints;
}

double ParameterOptimizer::scoreParams(
    const LinkageParams &params,
    const LinkageAnalysis &analysis) const
{
    if (!analysis.isAssemblyable) {
        return 0.0;
    }

    double score = 0.0;

    // ── 成分 1: 线性度 ──
    double linearityScore = analysis.linearityScore;
    score += m_constraints.weightLinearity * linearityScore;

    // ── 成分 2: 传动角 ──
    double minTrans = analysis.minTransmissionAngle;
    double transScore = 0.0;
    if (minTrans >= m_constraints.minTransmissionAngle) {
        // 超过最小要求，满分
        transScore = 1.0;
    } else if (minTrans >= 5.0) {
        // 在 5° 到 minTransmissionAngle 之间线性插值
        transScore = (minTrans - 5.0) / (m_constraints.minTransmissionAngle - 5.0);
        if (transScore < 0.0) transScore = 0.0;
    } else {
        transScore = 0.0;
    }
    score += m_constraints.weightTransmission * transScore;

    // ── 成分 3: 偏转范围 ──
    double range = analysis.deflectionRange;
    double rangeScore = 0.0;
    double target = m_constraints.minDeflectionRange;
    if (range >= target) {
        // 满足或超过目标：满分
        rangeScore = 1.0;
    } else if (range > 0.0) {
        // 线性插值
        rangeScore = range / target;
        if (rangeScore > 1.0) rangeScore = 1.0;
    } else {
        rangeScore = 0.0;
    }
    score += m_constraints.weightDeflection * rangeScore;

    // ── 惩罚项 ──
    // 死点惩罚
    if (analysis.hasDeadPoint) {
        score *= 0.3;  // 有死点的方案大幅降分
    }

    // 杆长可制造性奖励（偏好短杆）
    double maxLen = std::max({params.linkLength, params.servoArmRadius, params.hornRadius});
    if (maxLen < 30.0) {
        score *= 1.05;  // 紧凑设计 +5%
    }

    return score;
}

ParameterOptimizer::OptimizationResult ParameterOptimizer::optimizeL(
    double r1, double r2, int resolution) const
{
    OptimizationResult best;
    best.score = -1.0;

    KinematicSolver solver;

    // L 的有效范围：[|d - max(r₁,r₂)| + min(r₁,r₂), d + r₁ + r₂]
    // 实际上 L 需要满足可装配性
    double d = m_constraints.baseDistance;
    double maxR = qMax(r1, r2);
    double minR = qMin(r1, r2);

    // L 的下界：max(|d - maxR|, |r₁ - r₂|) + 小裕量
    double lMin = qMax(qAbs(d - maxR), qAbs(r1 - r2));
    lMin = qMax(lMin, 2.0);  // 至少 2mm

    // L 的上界：d + r₁ + r₂ - 小裕量
    double lMax = d + r1 + r2;
    lMax = qMin(lMax, m_constraints.maxLinkLength);

    if (lMin >= lMax) {
        return best;
    }

    double step = (lMax - lMin) / (resolution - 1);

    for (int i = 0; i < resolution; ++i) {
        double L = lMin + i * step;

        LinkageParams params;
        params.servoArmRadius = r1;
        params.hornRadius = r2;
        params.linkLength = L;
        params.baseDistance = d;
        params.servoAngleMin = m_constraints.servoAngleMin;
        params.servoAngleMax = m_constraints.servoAngleMax;

        auto vr = params.validate();
        if (!vr.valid) continue;

        solver.setParams(params);
        auto analysis = solver.sweepRange(100);  // 粗扫描 100 步

        if (!analysis.isAssemblyable) continue;

        double s = scoreParams(params, analysis);
        if (s > best.score) {
            best.score = s;
            best.params = params;
            best.analysis = analysis;
            best.description = QString("r₁=%1 r₂=%2 L=%3")
                .arg(r1, 0, 'f', 1)
                .arg(r2, 0, 'f', 1)
                .arg(L, 0, 'f', 1);
        }
    }

    return best;
}

ParameterOptimizer::OptimizationResult ParameterOptimizer::localRefine(
    const LinkageParams &baseParams, int gridSize) const
{
    OptimizationResult best;
    best.score = -1.0;
    best.params = baseParams;

    double r1Base = baseParams.servoArmRadius;
    double r2Base = baseParams.hornRadius;
    double lBase = baseParams.linkLength;

    // 在基准值 ±20% 范围内搜索
    double r1Range = r1Base * 0.2;
    double r2Range = r2Base * 0.2;
    double lRange = lBase * 0.2;

    double r1Min = qMax(2.0, r1Base - r1Range);
    double r1Max = qMin(m_constraints.maxServoArmRadius, r1Base + r1Range);
    double r2Min = qMax(2.0, r2Base - r2Range);
    double r2Max = qMin(m_constraints.maxHornRadius, r2Base + r2Range);

    KinematicSolver solver;

    for (int i = 0; i < gridSize; ++i) {
        double r1 = r1Min + (r1Max - r1Min) * i / (gridSize - 1);
        for (int j = 0; j < gridSize; ++j) {
            double r2 = r2Min + (r2Max - r2Min) * j / (gridSize - 1);

            auto result = optimizeL(r1, r2, gridSize);
            if (result.score > best.score) {
                best = result;
            }
        }
    }

    // 如果局部搜索没找到更好的，返回原来的
    if (best.score < 0) {
        best.score = 0;

        // 对原参数做一次精细分析
        KinematicSolver solver(baseParams);
        auto analysis = solver.sweepRange(200);
        best.params = baseParams;
        best.analysis = analysis;
        best.score = scoreParams(baseParams, analysis);
    }

    return best;
}

QVector<ParameterOptimizer::OptimizationResult> ParameterOptimizer::optimize(
    int topN, int gridResolution)
{
    QVector<OptimizationResult> allResults;
    double d = m_constraints.baseDistance;

    // ── R₁ 搜索空间 ──
    double r1Min = 3.0;  // 最小伺服臂 3mm
    double r1Max = m_constraints.maxServoArmRadius;
    double r1Step = (r1Max - r1Min) / (gridResolution - 1);

    // ── R₂ 搜索空间 ──
    double r2Min = 3.0;  // 最小舵角 3mm
    double r2Max = m_constraints.maxHornRadius;
    double r2Step = (r2Max - r2Min) / (gridResolution - 1);

    qDebug() << "[ParameterOptimizer] Grid search: r1 in ["
             << r1Min << "," << r1Max << "], r2 in ["
             << r2Min << "," << r2Max << "], resolution ="
             << gridResolution;

    KinematicSolver solver;

    for (int i = 0; i < gridResolution; ++i) {
        double r1 = r1Min + i * r1Step;
        for (int j = 0; j < gridResolution; ++j) {
            double r2 = r2Min + j * r2Step;

            // 快速装配检查：排除明显不合理的组合
            // r₁ + r₂ 不能太大（否则需求过长的连杆）
            if (r1 + r2 > d * 1.5) continue;

            auto result = optimizeL(r1, r2, gridResolution * 2);
            if (result.score > 0.0) {
                allResults.append(result);
            }
        }
    }

    qDebug() << "[ParameterOptimizer] Found" << allResults.size()
             << "valid candidates in grid search";

    // ── 排序：按分数降序 ──
    std::sort(allResults.begin(), allResults.end());

    // ── 对前 topN 个做局部细化 ──
    QVector<OptimizationResult> finalResults;
    int refineCount = qMin(topN * 2, allResults.size());
    for (int i = 0; i < refineCount; ++i) {
        auto refined = localRefine(allResults[i].params, 10);
        if (refined.score >= 0) {
            finalResults.append(refined);
        }
    }

    // 再排序
    std::sort(finalResults.begin(), finalResults.end());

    // 截取 topN
    if (finalResults.size() > topN) {
        finalResults.resize(topN);
    }

    // 补充描述
    for (auto &r : finalResults) {
        r.description = QString(
            "r₁=%1mm, r₂=%2mm, L=%3mm | "
            "偏转=%4° 传动角=%5° 线性度=%6 评分=%7")
            .arg(r.params.servoArmRadius, 0, 'f', 1)
            .arg(r.params.hornRadius, 0, 'f', 1)
            .arg(r.params.linkLength, 0, 'f', 1)
            .arg(r.analysis.deflectionRange, 0, 'f', 1)
            .arg(r.analysis.minTransmissionAngle, 0, 'f', 1)
            .arg(r.analysis.linearityScore, 0, 'f', 3)
            .arg(r.score, 0, 'f', 4);
    }

    return finalResults;
}
