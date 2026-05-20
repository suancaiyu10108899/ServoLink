#ifndef REVERSEDIALOG_H
#define REVERSEDIALOG_H

#include <QDialog>
#include <QDoubleSpinBox>
#include <QTableWidget>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include "ParameterOptimizer.h"
#include "KinematicSolver.h"

/**
 * @brief 输出优先设计对话框
 *
 * 给定舵面偏转目标，反向搜索最优伺服参数。
 * 适用于：机体结构固定(d)、舵角已定(r₂)，找合适的伺服臂(r₁)和连杆(L)。
 */
class ReverseDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ReverseDialog(QWidget *parent = nullptr);

    void setFixedParams(double hornR2, double baseD, double hornR);

signals:
    void paramsSelected(const LinkageParams &params);

private slots:
    void onSearch();
    void onApplySelected();
    void onCancel();

private:
    void setupUi();
    void displayResults(const QVector<ParameterOptimizer::OptimizationResult> &results);

    QDoubleSpinBox *m_spinBaseDist = nullptr;
    QDoubleSpinBox *m_spinHornR2 = nullptr;
    QDoubleSpinBox *m_spinTargetRange = nullptr;
    QDoubleSpinBox *m_spinTargetCenter = nullptr;
    QDoubleSpinBox *m_spinMinTransAngle = nullptr;
    QDoubleSpinBox *m_spinTopN = nullptr;
    QTableWidget *m_tableResults = nullptr;
    QProgressBar *m_progressBar = nullptr;
    QLabel *m_labelStatus = nullptr;
    QPushButton *m_btnSearch = nullptr;
    QPushButton *m_btnApply = nullptr;

    QVector<ParameterOptimizer::OptimizationResult> m_results;
};

#endif // REVERSEDIALOG_H
