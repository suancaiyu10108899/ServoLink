#ifndef PARAMETEROPTIMIZER_H
#define PARAMETEROPTIMIZER_H

#include <QVector>
#include <QPair>
#include <functional>
#include "LinkageParams.h"
#include "LinkageAnalysis.h"
#include "KinematicSolver.h"
#include "OperationResult.h"

/**
 * @brief 参数优化器
 *
 * 给定目标约束（期望的舵面偏转范围、最小传动角等），
 * 自动搜索最优的 (r₁, r₂, L) 组合。
 *
 * 优化方法：基于网格搜索 + 局部细化（简化 SLSQP 思想）
 *
 * 优化目标：
 *   1. 最大化线性度
 *   2. 满足偏转范围约束
 *   3. 最小传动角 ≥ 30°
 *
 * 使用示例：
 *   ParameterOptimizer opt;
 *   opt.setBaseDistance(50.0);                  // d 固定
 *   opt.setTargetDeflectionRange(90.0);          // 希望舵面偏转 ±45° (共90°)
 *   opt.setServoRange(-60, 60);
 *   auto results = opt.optimize();
 *   for (auto &r : results) {
 *       qDebug() << "r1=" << r.params.servoArmRadius
 *                << "r2=" << r.params.hornRadius
 *                << "L=" << r.params.linkLength
 *                << "score=" << r.score;
 *   }
 */
class ParameterOptimizer
{
public:
    /**
     * @brief 优化结果
     */
    struct OptimizationResult {
        LinkageParams params;
        LinkageAnalysis analysis;      // 对该组参数的全范围扫描结果
        double score = 0.0;            // 综合评分 [0, 1]，越高越好
        QString description;

        // 排序用
        bool operator<(const OptimizationResult &other) const
        {
            return score > other.score;  // 高分数在前
        }
    };

    /**
     * @brief 优化约束
     */
    struct Constraints {
        // 固定参数
        double baseDistance = 45.0;            // d：基距 (mm)，通常由机体结构决定
        double servoAngleMin = -60.0;          // 伺服臂最小转角 (°)
        double servoAngleMax = 60.0;           // 伺服臂最大转角 (°)

        // 目标约束
        double minDeflectionRange = 80.0;      // 期望的最小偏转范围 (°)，例如 ±40° 对应 80°
        double minTransmissionAngle = 30.0;    // 最小允许传动角 (°)
        double maxHornRadius = 25.0;           // 舵角最大半径（受舵面结构限制）
        double maxServoArmRadius = 25.0;       // 伺服臂最大半径
        double maxLinkLength = 100.0;          // 连杆最大长度

        // 优化权重
        double weightLinearity = 0.4;          // 线性度权重
        double weightTransmission = 0.3;       // 传动角权重
        double weightDeflection = 0.3;         // 偏转范围权重
    };

    ParameterOptimizer() = default;

    void setConstraints(const Constraints &constraints);
    Constraints constraints() const { return m_constraints; }

    // 便捷设置
    void setBaseDistance(double d) { m_constraints.baseDistance = d; }
    void setTargetDeflectionRange(double range) { m_constraints.minDeflectionRange = range; }
    void setServoRange(double minDeg, double maxDeg)
    {
        m_constraints.servoAngleMin = minDeg;
        m_constraints.servoAngleMax = maxDeg;
    }

    /**
     * @brief 执行优化
     *
     * @param topN 返回前 N 个最优结果（默认 10）
     * @param gridResolution 网格搜索分辨率（默认 20，即每维采样20个点）
     */
    QVector<OptimizationResult> optimize(int topN = 10, int gridResolution = 20);

    /**
     * @brief 对一组参数打分
     *
     * 评分范围 [0, 1]，越高越好。
     */
    double scoreParams(const LinkageParams &params,
                       const LinkageAnalysis &analysis) const;

private:
    Constraints m_constraints;

    /**
     * @brief 在给定 (r₁, r₂) 下搜索最优 L
     */
    OptimizationResult optimizeL(
        double r1, double r2, int resolution = 40) const;

    /**
     * @brief 局部细化
     *
     * 在 grid search 找到的候选点附近小范围密集搜索
     */
    OptimizationResult localRefine(
        const LinkageParams &baseParams, int gridSize = 10) const;
};

#endif // PARAMETEROPTIMIZER_H
