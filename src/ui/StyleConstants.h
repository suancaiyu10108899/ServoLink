#ifndef STYLECONSTANTS_H
#define STYLECONSTANTS_H

#include <QString>

/**
 * @brief 样式常量
 *
 * 集中管理 UI 层的颜色、字体、间距等常量。
 * 避免在多个文件中重复硬编码样式值。
 *
 * 设计原则：对齐 MindPath 的 StyleConstants 模式。
 */
namespace StyleConstants {

// ── 颜色 ──
namespace Colors {
    // 主色调：科技蓝
    const QString Primary    = "#2563EB";
    const QString PrimaryDark  = "#1D4ED8";
    const QString PrimaryLight = "#3B82F6";

    // 背景
    const QString Background = "#F8FAFC";
    const QString Surface    = "#FFFFFF";
    const QString Border     = "#E2E8F0";

    // 文字
    const QString TextPrimary   = "#1E293B";
    const QString TextSecondary = "#64748B";
    const QString TextDisabled  = "#94A3B8";

    // 功能色
    const QString Success  = "#10B981";
    const QString Warning  = "#F59E0B";
    const QString Danger   = "#EF4444";
    const QString Info     = "#3B82F6";

    // 机构可视化专用
    const QString LinkServoArm  = "#EF4444";   // 伺服臂 — 红色
    const QString LinkRod       = "#3B82F6";   // 连杆 — 蓝色
    const QString LinkHorn      = "#10B981";   // 舵角 — 绿色
    const QString LinkFrame     = "#64748B";   // 机架 — 灰色
    const QString JointPoint    = "#1E293B";   // 铰点 — 深色
}

// ── 字体 ──
namespace Fonts {
    const QString Default = "Segoe UI";
    const QString Monospace = "Cascadia Code";
    const int SizeBody = 12;
    const int SizeTitle = 16;
    const int SizeSmall = 10;
    const int SizeResult = 14;
}

// ── 间距 ──
namespace Spacing {
    const int XS = 4;
    const int SM = 8;
    const int MD = 12;
    const int LG = 16;
    const int XL = 24;
}

// ── 应用程序样式表 ──
const QString GlobalStyleSheet = R"(
    QWidget {
        background-color: #F8FAFC;
        color: #1E293B;
        font-family: "Segoe UI";
        font-size: 12px;
    }
    QMainWindow {
        background-color: #F8FAFC;
    }
    QGroupBox {
        font-weight: bold;
        border: 1px solid #E2E8F0;
        border-radius: 6px;
        margin-top: 12px;
        padding-top: 16px;
    }
    QGroupBox::title {
        subcontrol-origin: margin;
        left: 12px;
        padding: 0 6px;
        color: #1E293B;
    }
    QPushButton {
        background-color: #2563EB;
        color: white;
        border: none;
        border-radius: 4px;
        padding: 6px 16px;
        font-weight: bold;
    }
    QPushButton:hover {
        background-color: #1D4ED8;
    }
    QPushButton:pressed {
        background-color: #1E40AF;
    }
    QPushButton:disabled {
        background-color: #94A3B8;
    }
    QDoubleSpinBox, QSpinBox {
        border: 1px solid #E2E8F0;
        border-radius: 4px;
        padding: 4px 8px;
        background-color: #FFFFFF;
    }
    QDoubleSpinBox:focus, QSpinBox:focus {
        border-color: #2563EB;
    }
    QLabel#resultValue {
        font-weight: bold;
        color: #2563EB;
        font-size: 14px;
    }
    QLabel#sectionTitle {
        font-weight: bold;
        color: #1E293B;
        font-size: 13px;
    }
)";

} // namespace StyleConstants

#endif // STYLECONSTANTS_H
