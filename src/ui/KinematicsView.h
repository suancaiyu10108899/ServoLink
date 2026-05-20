#ifndef KINEMATICSVIEW_H
#define KINEMATICSVIEW_H

#include <QWidget>
#include <QTimer>
#include "MechanismState.h"
#include "LinkageParams.h"

/**
 * @brief 机构运动可视化组件
 *
 * 使用 QPainter 绘制舵面连杆机构的平面简图。
 * 支持静态显示（单个状态）和动画播放（QTimer 驱动帧刷新）。
 */
class KinematicsView : public QWidget
{
    Q_OBJECT

public:
    explicit KinematicsView(QWidget *parent = nullptr);

    /**
     * @brief 设置当前显示的状态
     */
    void setState(const MechanismState &state);
    void setParams(const LinkageParams &params);

    /**
     * @brief 设置全扫描数据（用于播放动画）
     */
    void setSweepData(const QVector<MechanismState> &states);

    /**
     * @brief 动画控制
     */
    void play(int fps = 30);
    void pause();
    void stop();
    bool isPlaying() const { return m_timer->isActive(); }

    /**
     * @brief 设置动画帧
     */
    void setFrame(int frame);

    int frameCount() const { return m_sweepData.size(); }
    int currentFrame() const { return m_currentFrame; }

    QSize minimumSizeHint() const override { return QSize(300, 250); }
    QSize sizeHint() const override { return QSize(450, 380); }

signals:
    void frameChanged(int frame);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void drawMechanism(QPainter &painter);
    QPointF worldToPixel(double wx, double wy) const;

    MechanismState m_state;
    LinkageParams m_params;

    QVector<MechanismState> m_sweepData;
    QTimer *m_timer;
    int m_currentFrame = 0;

    // 视图变换
    double m_scale = 4.0;        // 像素/mm
    QPointF m_offset;            // 视图偏移（像素）
    const double m_margin = 40;  // 边距
};

#endif // KINEMATICSVIEW_H
