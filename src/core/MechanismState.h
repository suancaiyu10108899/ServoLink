#ifndef MECHANISMSTATE_H
#define MECHANISMSTATE_H

#include <QPointF>

/**
 * @brief 机构的瞬时状态
 *
 * 描述在某个特定伺服臂转角 θ₁ 下，机构各节点的完整状态。
 *
 * 坐标约定：
 * - 原点 O₁（伺服旋转中心）固定在 (0, 0)
 * - O₂（舵面铰链中心）固定在 (d, 0)，即 x 轴正方向距离 baseDistance
 * - 所有坐标单位为毫米 (mm)
 * - 角度基准：O₁O₂ 方向（x 轴正向）为 0°
 *
 *   Y
 *   ↑
 *   │   ●球头A
 *   │  ╱
 *   │ ╱  连杆L
 *   │╱
 *   O₁(0,0)────O₂(d,0)──→ X
 *   │╲
 *   │ ╲
 *   │  ╲
 *   │   ●球头B (舵角末端)
 */
struct MechanismState
{
    double inputAngle = 0.0;           // θ₁：伺服臂转角 (°)
    double outputAngle = 0.0;          // θ₂：舵角偏转角 (°)，相对于 x 轴负方向
    double transmissionAngle = 0.0;    // 传动角 (°)：连杆与舵角的夹角
    double mechanicalAdvantage = 1.0;  // 力放大比（力矩比）

    QPointF jointA;                    // 球头A 坐标 (mm)
    QPointF jointB;                    // 球头B 坐标 (mm)

    /**
     * @brief 判断当前状态是否有效
     *
     * 传动角太小时机构接近死点，力传递效率极低。
     * 工程上建议传动角不低于 30°。
     */
    bool isValid() const
    {
        return transmissionAngle >= 1.0;  // 至少有 1° 的传动角
    }

    /**
     * @brief 是否接近死点位置
     *
     * 传动角 < 5° 视为接近死点（Danger zone）
     */
    bool isNearDeadPoint() const
    {
        return transmissionAngle < 5.0;
    }
};

#endif // MECHANISMSTATE_H
