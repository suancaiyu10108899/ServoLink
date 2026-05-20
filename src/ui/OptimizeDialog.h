#ifndef OPTIMIZEDIALOG_H
#define OPTIMIZEDIALOG_H

#include <QDialog>
#include <QDoubleSpinBox>
#include <QTableWidget>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include "ParameterOptimizer.h"
#include "StyleConstants.h"

/**
 * @brief 参数优化对话框
 *
 * 允许用户设定优化目标和约束，执行网格搜索优化，
 * 展示 Top N 结果列表，并支持一键应用到主窗口。
 */
class OptimizeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OptimizeDialog(QWidget *parent = nullptr);

    void setFixedParams(double baseDistance, double servoMin, double servoMax);

signals:
    void paramsSelected(const LinkageParams &params);

private slots:
    void onOptimize();
    void onApplySelected();
    void onCancel();

private:
    void setupUi();
    ParameterOptimizer::Constraints readConstraints() const;
    void displayResults(const QVector<ParameterOptimizer::OptimizationResult> &results);

    QDoubleSpinBox *m_spinBaseDist = nullptr;
    QDoubleSpinBox *m_spinServoMin = nullptr;
    QDoubleSpinBox *m_spinServoMax = nullptr;
    QDoubleSpinBox *m_spinTargetRange = nullptr;
    QDoubleSpinBox *m_spinMinTransAngle = nullptr;
    QDoubleSpinBox *m_spinTopN = nullptr;
    QTableWidget *m_tableResults = nullptr;
    QProgressBar *m_progressBar = nullptr;
    QLabel *m_labelStatus = nullptr;
    QPushButton *m_btnOptimize = nullptr;
    QPushButton *m_btnApply = nullptr;

    ParameterOptimizer m_optimizer;
    QVector<ParameterOptimizer::OptimizationResult> m_results;
};

#endif // OPTIMIZEDIALOG_H
