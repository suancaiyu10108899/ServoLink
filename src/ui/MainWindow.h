#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QTextEdit>
#include <QComboBox>
#include <QSlider>
#include <QFileDialog>
#include <QTabWidget>

#include "IStorageBackend.h"
#include "KinematicsView.h"
#include "CurvePlot.h"
#include "OptimizeDialog.h"
#include "ReverseDialog.h"
#include "LinkageParams.h"
#include "LinkageAnalysis.h"
#include "KinematicSolver.h"
#include "TransmissionAnalyzer.h"
#include "ParameterOptimizer.h"
#include "StyleConstants.h"

/**
 * @brief ServoLink 主窗口
 *
 * 当前阶段（Phase 1）实现最小可用的主窗口：
 * - 左侧：参数输入面板
 * - 右侧：计算结果展示
 * - 底部：状态栏
 *
 * Phase 2 增强：Slider 联动、舵机预设库、参数历史、CSV 导出。
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(IStorageBackend *storage, QWidget *parent = nullptr);
    ~MainWindow() override = default;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onCalculate();
    void onResetToDefault();
    void onSaveProject();
    void onLoadProject();
    void onQuickAnalyze();
    void onExportCsv();
    void onUndo();
    void onRedo();
    void onPresetChanged(int index);
    void onOpenOptimizer();
    void onOpenReverse();

    // 相位2: SpinBox-Slider 双向同步
    void onSpinServoArmChanged(double v);
    void onSpinHornChanged(double v);
    void onSpinLinkChanged(double v);
    void onSpinBaseChanged(double v);
    void onSpinServoMinChanged(double v);
    void onSpinServoMaxChanged(double v);
    void onSliderServoArmChanged(int v);
    void onSliderHornChanged(int v);
    void onSliderLinkChanged(int v);
    void onSliderBaseChanged(int v);

private:
    void setupUi();
    void setupConnections();
    void applyStyle();
    void populatePresets();

    LinkageParams readParamsFromUi() const;
    void writeParamsToUi(const LinkageParams &params, bool updateSliders = true);
    void displayAnalysis(const LinkageAnalysis &analysis);
    void displayQuickAssessment(const TransmissionAnalyzer::QuickAssessment &qa);
    void loadStoredParams();

    void pushHistory();
    void updateUndoRedoState();

    // 依赖
    IStorageBackend *m_storage = nullptr;
    KinematicSolver m_solver;

    // 相位2: 参数历史
    QVector<LinkageParams> m_history;
    int m_historyIndex = -1;

    // SpinBox
    QDoubleSpinBox *m_spinServoArm = nullptr;
    QDoubleSpinBox *m_spinHornRadius = nullptr;
    QDoubleSpinBox *m_spinLinkLength = nullptr;
    QDoubleSpinBox *m_spinBaseDistance = nullptr;
    QDoubleSpinBox *m_spinServoMin = nullptr;
    QDoubleSpinBox *m_spinServoMax = nullptr;

    // Sliders (相位2)
    QSlider *m_sliderServoArm = nullptr;
    QSlider *m_sliderHornRadius = nullptr;
    QSlider *m_sliderLinkLength = nullptr;
    QSlider *m_sliderBaseDistance = nullptr;

    // 预设库 (相位2)
    QComboBox *m_comboPresets = nullptr;
    QPushButton *m_btnUndo = nullptr;
    QPushButton *m_btnRedo = nullptr;
    QVector<LinkageParams> m_presetParams;

    // 结果展示
    QLabel *m_labelDeflectionRange = nullptr;
    QLabel *m_labelMaxDeflection = nullptr;
    QLabel *m_labelMinTransAngle = nullptr;
    QLabel *m_labelLinearity = nullptr;
    QLabel *m_labelGrade = nullptr;
    QLabel *m_labelGrashof = nullptr;
    QLabel *m_labelDeadPoint = nullptr;
    QLabel *m_labelQuickSummary = nullptr;
    QTextEdit *m_textSuggestions = nullptr;

    // 相位3: 可视化
    QTabWidget *m_tabWidget = nullptr;
    KinematicsView *m_kinematicsView = nullptr;
    CurvePlot *m_curvePlot = nullptr;
    QComboBox *m_curveTypeCombo = nullptr;
    QPushButton *m_btnPlay = nullptr;
    QPushButton *m_btnPause = nullptr;
    QPushButton *m_btnStop = nullptr;
    QSlider *m_animSlider = nullptr;

    // 装配模式
    QComboBox *m_comboAssembly = nullptr;

    // 定位标注
    QDoubleSpinBox *m_spinBaseAngle = nullptr;
    QDoubleSpinBox *m_spinHornOrigin = nullptr;
    QDoubleSpinBox *m_spinHornLimitUp = nullptr;
    QDoubleSpinBox *m_spinHornLimitLo = nullptr;
    QDoubleSpinBox *m_spinServoLimitMin = nullptr;
    QDoubleSpinBox *m_spinServoLimitMax = nullptr;

    // 解析式 Tab
    QTextEdit *m_textFormula = nullptr;
    QTextEdit *m_textFit = nullptr;
};

#endif // MAINWINDOW_H
