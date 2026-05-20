#include "CurvePlot.h"
#include <QPainter>
#include <QMouseEvent>
#include <QtMath>
#include <QToolTip>

CurvePlot::CurvePlot(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setMinimumSize(250, 200);
}

void CurvePlot::setAnalysisData(const LinkageAnalysis &analysis)
{
    m_analysis = analysis;
    m_hoverIndex = -1;
    update();
}

void CurvePlot::setCurveType(CurveType type)
{
    m_type = type;
    m_hoverIndex = -1;
    update();
}

QPointF CurvePlot::dataToPixel(double x, double y) const
{
    double pw = width() - marginLeft - marginRight;
    double ph = height() - marginTop - marginBottom;

    // X range
    double xMin = m_analysis.params.servoAngleMin;
    double xMax = m_analysis.params.servoAngleMax;

    // Y range
    double yMin, yMax;
    if (m_type == InputOutput) {
        yMin = 1e9; yMax = -1e9;
        for (const auto &s : m_analysis.sweepResults) {
            if (s.outputAngle < yMin) yMin = s.outputAngle;
            if (s.outputAngle > yMax) yMax = s.outputAngle;
        }
        if (yMax - yMin < 10) { yMin -= 5; yMax += 5; }
    } else {
        yMin = 0; yMax = 100;
    }

    double px = marginLeft + (x - xMin) / (xMax - xMin) * pw;
    double py = marginTop + (yMax - y) / (yMax - yMin) * ph;
    return QPointF(px, py);
}

void CurvePlot::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor("#FFFFFF"));

    if (m_analysis.sweepResults.isEmpty()) {
        painter.setPen(QColor("#94A3B8"));
        painter.drawText(rect(), Qt::AlignCenter, "无数据 — 请先执行计算");
        return;
    }

    // Axes
    double pw = width() - marginLeft - marginRight;
    double ph = height() - marginTop - marginBottom;
    QPointF origin(marginLeft, marginTop + ph);

    painter.setPen(QPen(QColor("#CBD5E1"), 1));
    painter.drawLine(origin, QPointF(origin.x() + pw, origin.y()));
    painter.drawLine(origin, QPointF(origin.x(), origin.y() - ph));

    // Grid
    painter.setPen(QPen(QColor("#F1F5F9"), 1, Qt::DotLine));
    for (int i = 1; i < 5; ++i) {
        double y = origin.y() - ph * i / 5;
        painter.drawLine(QPointF(origin.x(), y), QPointF(origin.x() + pw, y));
        double x = origin.x() + pw * i / 5;
        painter.drawLine(QPointF(x, origin.y()), QPointF(x, origin.y() - ph));
    }

    // Axis labels
    QFont axisFont("Segoe UI", 8);
    painter.setFont(axisFont);
    painter.setPen(QColor("#64748B"));
    QString xLabel = "θ₁ (°)";
    QString yLabel = m_type == InputOutput ? "θ₂ (°)" : "μ (°)";
    painter.drawText(QPointF(origin.x() + pw/2 - 20, origin.y() + 18), xLabel);
    painter.save();
    painter.translate(12, origin.y() - ph/2 + 30);
    painter.rotate(-90);
    painter.drawText(0, 0, yLabel);
    painter.restore();

    // Tick labels
    double xMin = m_analysis.params.servoAngleMin;
    double xMax = m_analysis.params.servoAngleMax;
    for (int i = 0; i <= 5; ++i) {
        double val = xMin + (xMax - xMin) * i / 5;
        QPointF p = dataToPixel(val, 0);
        painter.drawText(QPointF(p.x() - 12, origin.y() + 16),
                         QString::number(val, 'f', 1));
    }
    double yMin, yMax;
    if (m_type == InputOutput) {
        yMin = 1e9; yMax = -1e9;
        for (const auto &s : m_analysis.sweepResults) {
            if (s.outputAngle < yMin) yMin = s.outputAngle;
            if (s.outputAngle > yMax) yMax = s.outputAngle;
        }
    } else {
        yMin = 0; yMax = 100;
    }
    for (int i = 0; i <= 5; ++i) {
        double val = yMin + (yMax - yMin) * i / 5;
        QPointF p = dataToPixel(0, val);
        painter.drawText(QPointF(p.x() - 48, p.y() + 4),
                         QString::number(val, 'f', 1));
    }

    // Curve
    QPen curvePen(QColor("#2563EB"), 2);
    painter.setPen(curvePen);

    QVector<QPointF> points;
    for (int i = 0; i < m_analysis.sweepResults.size(); ++i) {
        const auto &s = m_analysis.sweepResults[i];
        double yv = m_type == InputOutput ? s.outputAngle : s.transmissionAngle;
        points.append(dataToPixel(s.inputAngle, yv));
    }

    for (int i = 0; i < points.size() - 1; ++i) {
        painter.drawLine(points[i], points[i + 1]);
    }

    // 30° reference line for transmission angle
    if (m_type == TransmissionAngle) {
        QPen refPen(QColor("#EF4444"), 1, Qt::DashLine);
        painter.setPen(refPen);
        QPointF p1 = dataToPixel(xMin, 30);
        QPointF p2 = dataToPixel(xMax, 30);
        painter.drawLine(p1, p2);
        painter.setPen(QColor("#EF4444"));
        painter.setFont(QFont("Segoe UI", 7));
        painter.drawText(QPointF(p2.x() + 2, p2.y()), "30°");
    }

    // Hover crosshair
    if (m_hoverIndex >= 0 && m_hoverIndex < points.size()) {
        QPointF hp = points[m_hoverIndex];
        painter.setPen(QPen(QColor("#1E293B"), 1, Qt::DotLine));
        painter.drawLine(QPointF(hp.x(), origin.y()), QPointF(hp.x(), origin.y() - ph));
        painter.drawLine(QPointF(origin.x(), hp.y()), QPointF(origin.x() + pw, hp.y()));
        // Dot
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor("#2563EB"));
        painter.drawEllipse(hp, 4, 4);
    }

    // Legend
    QRect legendRect(width() - 200, 8, 192, 22);
    painter.fillRect(legendRect, QColor(255, 255, 255, 210));
    painter.setPen(QColor("#1E293B"));
    painter.setFont(QFont("Segoe UI", 9, QFont::Bold));
    QString legend = m_type == InputOutput ? "θ₁ → θ₂ 映射" : "传动角 μ 变化";
    painter.drawText(legendRect, Qt::AlignCenter, legend);
}

void CurvePlot::mouseMoveEvent(QMouseEvent *event)
{
    m_mousePos = event->pos();
    if (m_analysis.sweepResults.isEmpty()) return;

    double pw = width() - marginLeft - marginRight;
    double xMin = m_analysis.params.servoAngleMin;
    double xMax = m_analysis.params.servoAngleMax;
    double mouseX = event->pos().x();

    double frac = (mouseX - marginLeft) / pw;
    if (frac < 0) frac = 0;
    if (frac > 1) frac = 1;

    m_hoverIndex = static_cast<int>(frac * (m_analysis.sweepResults.size() - 1));
    if (m_hoverIndex < 0) m_hoverIndex = 0;
    if (m_hoverIndex >= m_analysis.sweepResults.size())
        m_hoverIndex = m_analysis.sweepResults.size() - 1;

    const auto &s = m_analysis.sweepResults[m_hoverIndex];
    QString tipText;
    if (m_type == InputOutput) {
        tipText = QString("θ₁=%1°  θ₂=%2°")
            .arg(s.inputAngle, 0, 'f', 1)
            .arg(s.outputAngle, 0, 'f', 2);
    } else {
        tipText = QString("θ₁=%1°  μ=%2°")
            .arg(s.inputAngle, 0, 'f', 1)
            .arg(s.transmissionAngle, 0, 'f', 1);
    }
    QToolTip::showText(event->globalPos(), tipText, this);

    update();
}

void CurvePlot::leaveEvent(QEvent *)
{
    m_hoverIndex = -1;
    QToolTip::hideText();
    update();
}
