#include "KinematicsView.h"
#include <QPainter>
#include <QPaintEvent>
#include <QtMath>
#include <QFontMetrics>
#include "UnitSystem.h"

KinematicsView::KinematicsView(QWidget *parent)
    : QWidget(parent)
    , m_timer(new QTimer(this))
{
    setMinimumSize(300, 250);
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);

    connect(m_timer, &QTimer::timeout, this, [this]() {
        if (m_sweepData.isEmpty()) return;
        m_currentFrame = (m_currentFrame + 1) % m_sweepData.size();
        m_state = m_sweepData[m_currentFrame];
        emit frameChanged(m_currentFrame);
        update();
    });
}

void KinematicsView::setState(const MechanismState &state)
{
    m_state = state;
    update();
}

void KinematicsView::setParams(const LinkageParams &params)
{
    m_params = params;
    update();
}

void KinematicsView::setSweepData(const QVector<MechanismState> &states)
{
    m_sweepData = states;
    m_currentFrame = 0;
    if (!states.isEmpty()) {
        m_state = states[0];
    }
    update();
}

void KinematicsView::play(int fps)
{
    if (m_sweepData.isEmpty()) return;
    m_timer->start(1000 / fps);
}

void KinematicsView::pause()
{
    m_timer->stop();
}

void KinematicsView::stop()
{
    m_timer->stop();
    m_currentFrame = 0;
    if (!m_sweepData.isEmpty()) {
        m_state = m_sweepData[0];
    }
    update();
}

void KinematicsView::setFrame(int frame)
{
    if (m_sweepData.isEmpty()) return;
    if (frame < 0) frame = 0;
    if (frame >= m_sweepData.size()) frame = m_sweepData.size() - 1;
    m_currentFrame = frame;
    m_state = m_sweepData[frame];
    emit frameChanged(frame);
    update();
}

void KinematicsView::resizeEvent(QResizeEvent *)
{
    double vw = width() - 2 * m_margin;
    double vh = height() - 2 * m_margin;
    double worldW = m_params.baseDistance + m_params.servoArmRadius + m_params.hornRadius + 30;
    double worldH = 2 * qMax(m_params.servoArmRadius, m_params.hornRadius) + 30;
    double sx = vw / worldW;
    double sy = vh / worldH;
    m_scale = qMin(sx, sy);
    m_offset = QPointF(width() / 2.0 - (m_params.baseDistance * m_scale) / 2.0,
                       height() / 2.0);
    update();
}

QPointF KinematicsView::worldToPixel(double wx, double wy) const
{
    return QPointF(m_offset.x() + wx * m_scale,
                   m_offset.y() - wy * m_scale);  // y flip for screen
}

void KinematicsView::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor("#FAFBFC"));
    drawMechanism(painter);
}

void KinematicsView::drawMechanism(QPainter &painter)
{
    double d = m_params.baseDistance;
    double r2 = m_params.hornRadius;

    // O1, O2 (fixed)
    QPointF o1 = worldToPixel(0, 0);
    QPointF o2 = worldToPixel(d, 0);

    // A, B (moving)
    QPointF a = worldToPixel(m_state.jointA.x(), m_state.jointA.y());
    QPointF b = worldToPixel(m_state.jointB.x(), m_state.jointB.y());

    // Draw frame (base)
    QPen framePen(QColor("#94A3B8"), 3);
    painter.setPen(framePen);
    painter.drawLine(o1, o2);
    // Hatching for frame
    double hatchLen = 10;
    painter.drawLine(QPointF(o1.x() - hatchLen/2, o1.y() - hatchLen), QPointF(o1.x() + hatchLen/2, o1.y() + hatchLen));
    painter.drawLine(QPointF(o1.x() + hatchLen/2, o1.y() - hatchLen), QPointF(o1.x() - hatchLen/2, o1.y() + hatchLen));
    painter.drawLine(QPointF(o2.x() - hatchLen/2, o2.y() - hatchLen), QPointF(o2.x() + hatchLen/2, o2.y() + hatchLen));
    painter.drawLine(QPointF(o2.x() + hatchLen/2, o2.y() - hatchLen), QPointF(o2.x() - hatchLen/2, o2.y() + hatchLen));

    // Draw servo arm (r1)
    QPen armPen(QColor("#EF4444"), 3);
    painter.setPen(armPen);
    painter.drawLine(o1, a);

    // Draw horn (r2)
    QPen hornPen(QColor("#10B981"), 3);
    painter.setPen(hornPen);
    painter.drawLine(o2, b);

    // Draw link rod (L)
    QPen linkPen(QColor("#3B82F6"), 3);
    painter.setPen(linkPen);
    painter.drawLine(a, b);

    // Draw joints
    int jointR = 5;
    painter.setBrush(QColor("#1E293B"));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(a, jointR, jointR);
    painter.drawEllipse(b, jointR, jointR);
    painter.drawEllipse(o1, jointR, jointR);
    painter.drawEllipse(o2, jointR, jointR);

    // Labels
    QFont font("Segoe UI", 10, QFont::Bold);
    painter.setFont(font);
    painter.setPen(QColor("#1E293B"));

    // O1 label
    painter.drawText(QPointF(o1.x() - 20, o1.y() - 12), "O₁");
    // O2 label
    painter.drawText(QPointF(o2.x() + 6, o2.y() - 12), "O₂");
    // A label
    painter.drawText(QPointF(a.x() + 8, a.y() - 8), "A");
    // B label
    painter.drawText(QPointF(b.x() + 8, b.y() - 8), "B");

    // Dimension labels
    QFont smallFont("Segoe UI", 8);
    painter.setFont(smallFont);
    painter.setPen(QColor("#64748B"));

    // r1 label
    QPointF r1Mid = worldToPixel(m_state.jointA.x() / 2, m_state.jointA.y() / 2);
    painter.drawText(QPointF(r1Mid.x() + 4, r1Mid.y() - 4), "r₁");

    // L label
    QPointF lMid = worldToPixel((m_state.jointA.x() + m_state.jointB.x()) / 2,
                                 (m_state.jointA.y() + m_state.jointB.y()) / 2);
    painter.drawText(QPointF(lMid.x() + 4, lMid.y() - 4), "L");

    // d label
    QPointF dMid = worldToPixel(d / 2, 0);
    painter.drawText(QPointF(dMid.x(), dMid.y() + 16), "d");

    // Info box
    QString info = QString("θ₁=%1° θ₂=%2° μ=%3°")
        .arg(m_state.inputAngle, 0, 'f', 1)
        .arg(m_state.outputAngle, 0, 'f', 1)
        .arg(m_state.transmissionAngle, 0, 'f', 1);
    QRect infoRect(8, 8, 350, 22);
    painter.fillRect(infoRect, QColor(255, 255, 255, 200));
    painter.setPen(QColor("#1E293B"));
    painter.setFont(QFont("Cascadia Code", 9));
    painter.drawText(infoRect, Qt::AlignLeft | Qt::AlignVCenter, info);

    // Dead point warning
    if (m_state.isNearDeadPoint()) {
        QRect warnRect(8, height() - 28, 200, 22);
        painter.fillRect(warnRect, QColor(239, 68, 68, 180));
        painter.setPen(Qt::white);
        painter.setFont(QFont("Segoe UI", 10, QFont::Bold));
        painter.drawText(warnRect, Qt::AlignCenter, "⚠ 接近死点");
    }
}
