#ifndef LINKAGEPARAMS_H
#define LINKAGEPARAMS_H

#include <QString>
#include <QJsonObject>

/**
 * @brief 舵面连杆机构参数
 *
 * 标准四杆机构（伺服臂 - 连杆 - 舵角）的全部几何参数。
 *
 * 命名约定：
 * - 所有长度统一使用毫米 (mm)
 * - 所有角度统一使用度 (°)
 *
 * 机构简图：
 *
 *        O₁ (伺服旋转中心)
 *        │
 *        ● 球头A  ← 伺服臂长 r₁
 *        │
 *        │  连杆长 L
 *        │
 *        ● 球头B
 *        │
 *        └──── 舵角长 r₂
 *             │
 *        O₂ (舵面铰链中心)
 *
 *    O₁ 和 O₂ 之间的距离为基距 d
 */
struct LinkageParams
{
    double servoArmRadius = 15.0;   // r₁：伺服臂半径 (mm)
    double hornRadius = 18.0;       // r₂：舵角半径 (mm)
    double linkLength = 35.0;       // L ：连杆长度 (mm)
    double baseDistance = 45.0;     // d ：基距 —— 伺服旋转中心到舵面铰链中心的距离 (mm)
    double servoAngleMin = -60.0;   // θ₁ 最小：伺服臂最小转角 (°)
    double servoAngleMax = 60.0;    // θ₁ 最大：伺服臂最大转角 (°)
    double baseAngle = 0.0;         // 基距倾角：O₁O₂ 连线与水平面的倾斜角 (°)
    double hornOriginDeg = 155.0;   // 舵面原点（中立）时的摇杆角 θ₂
    double hornLimitUpDeg = 120.0;  // 舵面上限（最大上偏）时的摇杆角 θ₂
    double hornLimitLoDeg = 180.0;  // 舵面下限（最大下偏）时的摇杆角 θ₂
    double servoLimitMinDeg = -90.0; // 舵机物理限位最小 (°)
    double servoLimitMaxDeg = 90.0;  // 舵机物理限位最大 (°)

    /**
     * @brief 验证参数是否在合理的物理范围内
     *
     * 检查项：
     * 1. 所有长度 > 0
     * 2. 角度范围合理（min < max，在 ±120° 内）
     * 3. 满足三角形不等式（机构可装配）
     */
    struct ValidationResult {
        bool valid = true;
        QString message;
    };

    ValidationResult validate() const
    {
        ValidationResult result;

        // 长度必须为正
        if (servoArmRadius <= 0) {
            result.valid = false;
            result.message = "伺服臂半径必须大于 0";
            return result;
        }
        if (hornRadius <= 0) {
            result.valid = false;
            result.message = "舵角半径必须大于 0";
            return result;
        }
        if (linkLength <= 0) {
            result.valid = false;
            result.message = "连杆长度必须大于 0";
            return result;
        }
        if (baseDistance <= 0) {
            result.valid = false;
            result.message = "基距必须大于 0";
            return result;
        }

        // 角度范围检查
        if (servoAngleMin >= servoAngleMax) {
            result.valid = false;
            result.message = "伺服臂最小角度必须小于最大角度";
            return result;
        }
        if (servoAngleMin < -180.0 || servoAngleMax > 180.0) {
            result.valid = false;
            result.message = "伺服臂角度范围须在 ±180° 以内";
            return result;
        }

        // 可装配性：任意一边必须小于其他三边之和（对于四杆机构）
        // 最长边 + 最短边 ≤ 其他两边之和 → 可装配（Grashof 的变体）
        // 这里只做基础检查，详细的 Grashof 判定在 TransmissionAnalyzer 中
        double sides[4] = {
            servoArmRadius,         // r₁
            linkLength,             // L
            hornRadius,             // r₂
            baseDistance            // d
        };
        // 排序前3个+最后一个的简单检查
        // 对于四杆机构：最长杆 ≤ 其他三杆之和（否则无法闭环）
        double maxSide = sides[0];
        double sum = 0.0;
        for (int i = 0; i < 4; ++i) {
            sum += sides[i];
            if (sides[i] > maxSide) maxSide = sides[i];
        }
        // maxSide 能被其他三杆搭到吗？
        double otherSum = sum - maxSide;
        if (maxSide > otherSum * 1.001) {
            // 给一点容差（0.1%）
            result.valid = false;
            result.message = QString(
                "机构无法闭环装配：最长杆 (%1 mm) 大于其他三杆之和 (%2 mm)")
                .arg(maxSide, 0, 'f', 2)
                .arg(otherSum, 0, 'f', 2);
            return result;
        }

        return result;
    }

    // ── 序列化 ──
    QJsonObject toJson() const
    {
        QJsonObject obj;
        obj["servoArmRadius"] = servoArmRadius;
        obj["hornRadius"] = hornRadius;
        obj["linkLength"] = linkLength;
        obj["baseDistance"] = baseDistance;
        obj["baseAngle"] = baseAngle;
        obj["servoAngleMin"] = servoAngleMin;
        obj["servoAngleMax"] = servoAngleMax;
        obj["hornOriginDeg"] = hornOriginDeg;
        obj["hornLimitUpDeg"] = hornLimitUpDeg;
        obj["hornLimitLoDeg"] = hornLimitLoDeg;
        obj["servoLimitMinDeg"] = servoLimitMinDeg;
        obj["servoLimitMaxDeg"] = servoLimitMaxDeg;
        return obj;
    }

    static LinkageParams fromJson(const QJsonObject &obj)
    {
        LinkageParams p;
        p.servoArmRadius = obj["servoArmRadius"].toDouble(15.0);
        p.hornRadius = obj["hornRadius"].toDouble(18.0);
        p.linkLength = obj["linkLength"].toDouble(35.0);
        p.baseDistance = obj["baseDistance"].toDouble(45.0);
        p.baseAngle = obj["baseAngle"].toDouble(0.0);
        p.servoAngleMin = obj["servoAngleMin"].toDouble(-60.0);
        p.servoAngleMax = obj["servoAngleMax"].toDouble(60.0);
        p.hornOriginDeg = obj["hornOriginDeg"].toDouble(155.0);
        p.hornLimitUpDeg = obj["hornLimitUpDeg"].toDouble(120.0);
        p.hornLimitLoDeg = obj["hornLimitLoDeg"].toDouble(180.0);
        p.servoLimitMinDeg = obj["servoLimitMinDeg"].toDouble(-90.0);
        p.servoLimitMaxDeg = obj["servoLimitMaxDeg"].toDouble(90.0);
        return p;
    }

    bool operator==(const LinkageParams &other) const
    {
        return qFuzzyCompare(servoArmRadius, other.servoArmRadius)
            && qFuzzyCompare(hornRadius, other.hornRadius)
            && qFuzzyCompare(linkLength, other.linkLength)
            && qFuzzyCompare(baseDistance, other.baseDistance)
            && qFuzzyCompare(servoAngleMin, other.servoAngleMin)
            && qFuzzyCompare(servoAngleMax, other.servoAngleMax);
    }

    bool operator!=(const LinkageParams &other) const
    {
        return !(*this == other);
    }
};

#endif // LINKAGEPARAMS_H
