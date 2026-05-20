#ifndef LINKAGEANALYSIS_H
#define LINKAGEANALYSIS_H

#include <QVector>
#include "LinkageParams.h"
#include "MechanismState.h"

/**
 * @brief 机构完整分析结果
 *
 * 对一组参数进行全角度范围扫描后的汇总结果。
 * 这是 Core 层的主要输出，UI 层拿这个对象来展示数值和绘图。
 */
struct LinkageAnalysis
{
    LinkageParams params;                        // 输入参数
    QVector<MechanismState> sweepResults;        // 全范围扫描结果（按 θ₁ 递增排列）
    double maxDeflection = 0.0;                  // 最大舵面偏转角 (°)
    double deflectionRange = 0.0;                // 偏转范围（max - min, °）
    double minTransmissionAngle = 90.0;          // 最小传动角 (°)
    double linearityScore = 0.0;                 // 线性度评分 [0, 1]，1 表示完全线性
    bool hasDeadPoint = false;                   // 运动范围内是否有死点
    bool isAssemblyable = true;                  // 机构是否可装配

    // ── 便捷查询 ──

    int sweepSteps() const { return sweepResults.size(); }

    MechanismState stateAt(int index) const
    {
        if (index >= 0 && index < sweepResults.size())
            return sweepResults[index];
        return MechanismState();
    }

    MechanismState stateAtInputAngle(double angle) const
    {
        // 二分查找最接近的扫描点
        if (sweepResults.isEmpty()) return MechanismState();
        int lo = 0, hi = sweepResults.size() - 1;
        while (lo < hi) {
            int mid = (lo + hi) / 2;
            if (sweepResults[mid].inputAngle < angle)
                lo = mid + 1;
            else
                hi = mid;
        }
        // 找最近点
        if (lo > 0) {
            double d1 = qAbs(sweepResults[lo].inputAngle - angle);
            double d0 = qAbs(sweepResults[lo - 1].inputAngle - angle);
            if (d0 < d1) lo = lo - 1;
        }
        return sweepResults[lo];
    }

    // ── 判断结果质量 ──

    /**
     * @brief 对舵面连杆机构给出综合评价
     *
     * 工程经验准则（参考航模设计实践）：
     * - 传动角 ≥ 30°：合格
     * - 偏转范围覆盖且无死点：合格
     * - 线性度越高越好（≥0.85 为优）
     */
    enum class Grade {
        Excellent,      // 全部指标优秀
        Good,           // 可用
        Marginal,       // 勉强可用，建议优化
        Poor            // 不可用
    };

    Grade grade() const
    {
        if (!isAssemblyable) return Grade::Poor;
        if (hasDeadPoint) return Grade::Poor;

        bool transOk = minTransmissionAngle >= 30.0;
        bool linearOk = linearityScore >= 0.85;

        if (transOk && linearOk) return Grade::Excellent;
        if (transOk || minTransmissionAngle >= 20.0) return Grade::Good;
        return Grade::Marginal;
    }

    QString gradeDescription() const
    {
        switch (grade()) {
        case Grade::Excellent: return QString("优秀 — 各项指标均满足工程要求");
        case Grade::Good:      return QString("良好 — 机构可用");
        case Grade::Marginal:  return QString("勉强 — 建议优化参数");
        case Grade::Poor:      return QString("不良 — 机构不可用，请调整参数");
        }
        return QString();
    }
};

#endif // LINKAGEANALYSIS_H
