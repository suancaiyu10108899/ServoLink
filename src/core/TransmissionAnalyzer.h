#ifndef TRANSMISSIONANALYZER_H
#define TRANSMISSIONANALYZER_H

#include "LinkageParams.h"
#include "LinkageAnalysis.h"
#include "OperationResult.h"

/**
 * @brief 传动质量分析器
 *
 * 在不做全范围扫描的前提下，快速评判一组参数的传动质量。
 *
 * 检查项：
 * 1. Grashof 准则（杆长条件判定机构类型）
 * 2. 传动角极值（最近似公式）
 * 3. 死点检测
 * 4. 偏转范围估计
 */
class TransmissionAnalyzer
{
public:
    TransmissionAnalyzer() = default;

    /**
     * @brief Grashof 类型
     *
     * 对平面四杆机构，设 s = 最短杆长, l = 最长杆长, p, q = 中间两杆。
     * Grashof 条件：s + l ≤ p + q
     */
    enum class GrashofType {
        CrankRocker,       // 曲柄摇杆：最短杆为连架杆（且不为机架对边）
        DoubleCrank,       // 双曲柄：最短杆为机架
        DoubleRocker,      // 双摇杆：最短杆为连杆
        NonGrashof,        // 非 Grashof：s+l > p+q（三摇杆，无整周回转）
        ChangePoint,       // 变点：s+l = p+q
        Unknown
    };

    /**
     * @brief 判定 Grashof 类型
     *
     * 四杆：r₁（伺服臂），L（连杆），r₂（舵角），d（基距/机架）
     */
    static GrashofType classifyGrashof(const LinkageParams &params);

    static QString grashofTypeName(GrashofType type);

    /**
     * @brief 快速分析传动质量（不做全扫描）
     *
     * 基于杆长关系给出快速评估，包括：
     * - Grashof 类型
     * - 死点风险
     * - 最小传动角估计
     */
    struct QuickAssessment {
        GrashofType grashofType = GrashofType::Unknown;
        bool likelyHasDeadPoint = false;
        double estimatedMinTransAngle = 90.0;  // 估计的最小传动角
        QString summary;
    };

    static QuickAssessment quickAssess(const LinkageParams &params);

    /**
     * @brief 生成一组参数的改进建议
     *
     * 当传动质量不佳时，给出参数调整方向。
     */
    static QStringList improvementSuggestions(
        const LinkageParams &params,
        const LinkageAnalysis &analysis);
};

#endif // TRANSMISSIONANALYZER_H
