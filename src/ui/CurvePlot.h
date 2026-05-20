#ifndef CURVEPLOT_H
#define CURVEPLOT_H

#include <QWidget>
#include <QVector>
#include "LinkageAnalysis.h"

/**
 * @brief 曲线图组件
 *
 * 使用 QPainter 绘制机构特性曲线：
 * - 输入-输出映射曲线 (θ₁ → θ₂)
 * - 传动角变化曲线 (θ₁ → μ)
 *
 * 支持鼠标悬停显示数值。
 */
class CurvePlot : public QWidget
{
    Q_OBJECT

public:
    enum CurveType {
        InputOutput,      // θ₁ → θ₂
        TransmissionAngle // θ₁ → μ
    };

    explicit CurvePlot(QWidget *parent = nullptr);

    void setAnalysisData(const LinkageAnalysis &analysis);
    void setCurveType(CurveType type);

    QSize minimumSizeHint() const override { return QSize(250, 200); }
    QSize sizeHint() const override { return QSize(400, 300); }

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QPointF dataToPixel(double x, double y) const;

    LinkageAnalysis m_analysis;
    CurveType m_type = InputOutput;

    // 鼠标悬停
    int m_hoverIndex = -1;
    QPoint m_mousePos;

    // 边距
    static constexpr double marginLeft = 55;
    static constexpr double marginRight = 20;
    static constexpr double marginTop = 25;
    static constexpr double marginBottom = 40;
};

#endif // CURVEPLOT_H
