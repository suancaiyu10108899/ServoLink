#ifndef UNITSYSTEM_H
#define UNITSYSTEM_H

#include <cmath>

namespace {
    constexpr double PI = 3.14159265358979323846;
}

/**
 * @brief 单位转换工具
 *
 * 所有内部计算使用统一的物理量，对外提供转换。
 *
 * 内部约定：
 * - 长度：毫米 (mm)
 * - 角度：度 (°)
 *
 * 命名规范：以 from 开头表示"从外部单位转为内部"，以 to 开头表示"从内部转为外部单位"
 */
namespace UnitSystem {

// ── 角度转换 ──
inline double degToRad(double degrees) { return degrees * PI / 180.0; }
inline double radToDeg(double radians) { return radians * 180.0 / PI; }

// ── 长度转换（毫米为基准） ──
inline double mmToCm(double mm) { return mm / 10.0; }
inline double cmToMm(double cm) { return cm * 10.0; }

inline double mmToInch(double mm) { return mm / 25.4; }
inline double inchToMm(double inch) { return inch * 25.4; }

// ── 角度规范化 ──

/**
 * @brief 将角度规范化到 [-180°, 180°)
 */
inline double normalizeAngle180(double degrees)
{
    double a = fmod(degrees, 360.0);
    if (a >= 180.0) a -= 360.0;
    if (a < -180.0) a += 360.0;
    return a;
}

/**
 * @brief 将角度规范化到 [0°, 360°)
 */
inline double normalizeAngle360(double degrees)
{
    double a = fmod(degrees, 360.0);
    if (a < 0.0) a += 360.0;
    return a;
}

// ── 几何工具 ──

/**
 * @brief 两点间距离
 */
inline double distance(double x1, double y1, double x2, double y2)
{
    double dx = x2 - x1;
    double dy = y2 - y1;
    return sqrt(dx * dx + dy * dy);
}

/**
 * @brief 两点间连线的角度（相对于 x 轴正向，返回度）
 */
inline double angleBetween(double x1, double y1, double x2, double y2)
{
    return radToDeg(atan2(y2 - y1, x2 - x1));
}

/**
 * @brief 计算两条向量之间的夹角度数
 *
 * 向量 v1: (x1,y1) 从原点出发, v2: (x2,y2) 从原点出发
 * 返回夹角 [0°, 180°]
 */
inline double angleBetweenVectors(double x1, double y1, double x2, double y2)
{
    double dot = x1 * x2 + y1 * y2;
    double mag1 = sqrt(x1 * x1 + y1 * y1);
    double mag2 = sqrt(x2 * x2 + y2 * y2);
    if (mag1 < 1e-10 || mag2 < 1e-10) return 0.0;
    double cosTheta = dot / (mag1 * mag2);
    // 防止浮点误差导致 cosTheta 超出 [-1, 1]
    if (cosTheta > 1.0) cosTheta = 1.0;
    if (cosTheta < -1.0) cosTheta = -1.0;
    return radToDeg(acos(cosTheta));
}

} // namespace UnitSystem

#endif // UNITSYSTEM_H
