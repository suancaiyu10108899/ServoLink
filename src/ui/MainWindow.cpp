#include "MainWindow.h"
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QCloseEvent>
#include <QMessageBox>
#include <QScrollArea>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QDebug>
#include "JsonStorageBackend.h"
#include "UnitSystem.h"

MainWindow::MainWindow(IStorageBackend *storage, QWidget *parent)
    : QMainWindow(parent)
    , m_storage(storage)
{
    setupUi();
    setupConnections();
    applyStyle();
    populatePresets();
    loadStoredParams();
    pushHistory();
}

void MainWindow::setupUi()
{
    // ── 菜单栏 ──
    QMenu *fileMenu = menuBar()->addMenu("文件(&F)");
    QAction *saveAction = fileMenu->addAction("保存项目(&S)");
    QAction *loadAction = fileMenu->addAction("加载项目(&L)");
    QAction *exportAction = fileMenu->addAction("导出 CSV(&E)");
    fileMenu->addSeparator();
    QAction *exitAction = fileMenu->addAction("退出(&X)");

    connect(saveAction, &QAction::triggered, this, &MainWindow::onSaveProject);
    connect(loadAction, &QAction::triggered, this, &MainWindow::onLoadProject);
    connect(exportAction, &QAction::triggered, this, &MainWindow::onExportCsv);
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);

    QMenu *editMenu = menuBar()->addMenu("编辑(&E)");
    m_btnUndo = new QPushButton("撤销"); m_btnUndo->setEnabled(false);
    m_btnRedo = new QPushButton("重做"); m_btnRedo->setEnabled(false);
    QAction *undoAction = editMenu->addAction("撤销(&Z)");
    QAction *redoAction = editMenu->addAction("重做(&Y)");
    connect(undoAction, &QAction::triggered, this, &MainWindow::onUndo);
    connect(redoAction, &QAction::triggered, this, &MainWindow::onRedo);

    QMenu *calcMenu = menuBar()->addMenu("计算(&C)");
    QAction *calcAction = calcMenu->addAction("执行计算(&E)");
    QAction *quickAction = calcMenu->addAction("快速评估(&Q)");
    connect(calcAction, &QAction::triggered, this, &MainWindow::onCalculate);
    connect(quickAction, &QAction::triggered, this, &MainWindow::onQuickAnalyze);

    QMenu *helpMenu = menuBar()->addMenu("帮助(&H)");
    QAction *aboutAction = helpMenu->addAction("关于(&A)");
    connect(aboutAction, &QAction::triggered, [this]() {
        QMessageBox::about(this, "关于 ServoLink",
            "ServoLink — 舵连设计台 v0.2.0\n\n"
            "航模舵面连杆机构工程计算与参数优化工具\n\n"
            "技术栈：C++17 + Qt 6 + CMake\n\n"
            "机构学引擎：Newton-Raphson 迭代法正解\n"
            "优化算法：网格搜索 + 局部细化\n\n"
            "数据存储：%LOCALAPPDATA%\\ServoLink\\ServoLink\\");
    });

    // ── 中央部件 ──
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // ── 左侧：参数输入面板 ──
    QWidget *leftPanel = new QWidget;
    leftPanel->setFixedWidth(340);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(6);

    // 预设库
    QGroupBox *presetGroup = new QGroupBox("预设库");
    QHBoxLayout *presetLayout = new QHBoxLayout(presetGroup);
    m_comboPresets = new QComboBox;
    m_comboPresets->addItem("— 选择预设 —");
    m_comboPresets->setMinimumWidth(200);
    presetLayout->addWidget(m_comboPresets);
    leftLayout->addWidget(presetGroup);

    // 参数输入组
    QGroupBox *paramGroup = new QGroupBox("机构参数");
    QVBoxLayout *paramLayout = new QVBoxLayout(paramGroup);
    paramLayout->setSpacing(4);

    auto makeParamRow = [this](const QString &label, const QString &unit,
                           double min, double max, double value, double step,
                           int sliderMin, int sliderMax,
                           QDoubleSpinBox *&spinBox, QSlider *&slider,
                           QVBoxLayout *layout)
    {
        QHBoxLayout *rowLabel = new QHBoxLayout;
        QLabel *lbl = new QLabel(label);
        lbl->setObjectName("sectionTitle");
        rowLabel->addWidget(lbl);
        rowLabel->addStretch();
        layout->addLayout(rowLabel);

        QHBoxLayout *rowInput = new QHBoxLayout;
        spinBox = new QDoubleSpinBox;
        spinBox->setRange(min, max);
        spinBox->setValue(value);
        spinBox->setSingleStep(step);
        spinBox->setDecimals(1);
        spinBox->setSuffix(QString(" %1").arg(unit));
        spinBox->setFixedWidth(120);

        slider = new QSlider(Qt::Horizontal);
        slider->setRange(sliderMin, sliderMax);
        slider->setValue(static_cast<int>(value * (sliderMax - sliderMin) / (max - min)));
        slider->setFixedWidth(120);

        rowInput->addWidget(spinBox);
        rowInput->addWidget(slider);
        rowInput->addStretch();
        layout->addLayout(rowInput);
        return spinBox;
    };

    makeParamRow("伺服臂半径 r₁", "mm", 1.0, 50.0, 15.0, 0.5, 10, 500,
                 m_spinServoArm, m_sliderServoArm, paramLayout);
    makeParamRow("舵角半径 r₂", "mm", 1.0, 50.0, 18.0, 0.5, 10, 500,
                 m_spinHornRadius, m_sliderHornRadius, paramLayout);
    makeParamRow("连杆长度 L", "mm", 2.0, 150.0, 35.0, 0.5, 20, 1500,
                 m_spinLinkLength, m_sliderLinkLength, paramLayout);
    makeParamRow("基距 d", "mm", 5.0, 200.0, 45.0, 0.5, 50, 2000,
                 m_spinBaseDistance, m_sliderBaseDistance, paramLayout);

    QLabel *rangeLabel = new QLabel("伺服臂转角范围");
    rangeLabel->setStyleSheet("font-weight: bold; margin-top: 6px;");
    paramLayout->addWidget(rangeLabel);

    QHBoxLayout *rangeRow = new QHBoxLayout;
    m_spinServoMin = new QDoubleSpinBox;
    m_spinServoMin->setRange(-180.0, 180.0);
    m_spinServoMin->setValue(-60.0);
    m_spinServoMin->setSingleStep(1.0);
    m_spinServoMin->setSuffix(" °");
    m_spinServoMax = new QDoubleSpinBox;
    m_spinServoMax->setRange(-180.0, 180.0);
    m_spinServoMax->setValue(60.0);
    m_spinServoMax->setSingleStep(1.0);
    m_spinServoMax->setSuffix(" °");

    rangeRow->addWidget(new QLabel("最小:"));
    rangeRow->addWidget(m_spinServoMin);
    rangeRow->addWidget(new QLabel("最大:"));
    rangeRow->addWidget(m_spinServoMax);
    rangeRow->addStretch();
    paramLayout->addLayout(rangeRow);

    // 装配模式
    QLabel *asmLabel = new QLabel("装配模式");
    asmLabel->setStyleSheet("font-weight: bold; margin-top: 6px;");
    paramLayout->addWidget(asmLabel);
    m_comboAssembly = new QComboBox;
    m_comboAssembly->addItem("同侧（闭式装配）", static_cast<int>(KinematicSolver::AssemblyMode::Closed));
    m_comboAssembly->addItem("对侧（开式装配）", static_cast<int>(KinematicSolver::AssemblyMode::Open));
    paramLayout->addWidget(m_comboAssembly);

    // 基距倾角
    QHBoxLayout *baseAngleRow = new QHBoxLayout;
    QLabel *baLabel = new QLabel("基距倾角");
    baLabel->setStyleSheet("font-weight: bold; margin-top: 6px;");
    m_spinBaseAngle = new QDoubleSpinBox;
    m_spinBaseAngle->setRange(-90.0, 90.0);
    m_spinBaseAngle->setValue(0.0);
    m_spinBaseAngle->setSingleStep(1.0);
    m_spinBaseAngle->setSuffix(" °");
    baseAngleRow->addWidget(baLabel);
    baseAngleRow->addWidget(m_spinBaseAngle);
    baseAngleRow->addStretch();
    paramLayout->addLayout(baseAngleRow);
    connect(m_spinBaseAngle, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this]() { onCalculate(); });

    leftLayout->addWidget(paramGroup);

    // 定位标注
    QGroupBox *posGroup = new QGroupBox("定位标注");
    QVBoxLayout *posLayout = new QVBoxLayout(posGroup);
    posLayout->setSpacing(3);
    auto addPosRow = [&](const QString &label, double min, double max, double def, const QString &unit, QDoubleSpinBox *&spin) {
        QHBoxLayout *row = new QHBoxLayout;
        QLabel *lbl = new QLabel(label);
        lbl->setFixedWidth(100);
        spin = new QDoubleSpinBox;
        spin->setRange(min, max);
        spin->setValue(def);
        spin->setSingleStep(1.0);
        spin->setSuffix(QString(" %1").arg(unit));
        row->addWidget(lbl);
        row->addWidget(spin);
        row->addStretch();
        posLayout->addLayout(row);
        connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, [this]() { onCalculate(); });
    };
    addPosRow("舵面原点 θ₂", 0, 360, 155.0, "°", m_spinHornOrigin);
    addPosRow("舵面上限 θ₂", 0, 360, 120.0, "°", m_spinHornLimitUp);
    addPosRow("舵面下限 θ₂", 0, 360, 180.0, "°", m_spinHornLimitLo);
    addPosRow("伺服上限 θ₁", -180, 180, 90.0, "°", m_spinServoLimitMax);
    addPosRow("伺服下限 θ₁", -180, 180, -90.0, "°", m_spinServoLimitMin);
    leftLayout->addWidget(posGroup);

    // 操作按钮组
    QGroupBox *actionGroup = new QGroupBox("操作");
    QVBoxLayout *actionLayout = new QVBoxLayout(actionGroup);
    actionLayout->setSpacing(4);

    QPushButton *btnCalc = new QPushButton("执行计算");
    QPushButton *btnQuick = new QPushButton("快速评估");
    QHBoxLayout *undoRow = new QHBoxLayout;
    undoRow->addWidget(m_btnUndo);
    undoRow->addWidget(m_btnRedo);
    QPushButton *btnReset = new QPushButton("恢复默认");
    QPushButton *btnSave = new QPushButton("保存项目");
    QPushButton *btnExport = new QPushButton("导出 CSV");
    QPushButton *btnOptimize = new QPushButton("⚡ 参数优化");
    QPushButton *btnReverse = new QPushButton("🎯 输出优先");

    actionLayout->addWidget(btnCalc);
    actionLayout->addWidget(btnQuick);
    actionLayout->addLayout(undoRow);
    actionLayout->addWidget(btnReset);
    actionLayout->addWidget(btnSave);
    actionLayout->addWidget(btnExport);
    actionLayout->addWidget(btnOptimize);
    actionLayout->addWidget(btnReverse);
    leftLayout->addWidget(actionGroup);

    connect(btnCalc, &QPushButton::clicked, this, &MainWindow::onCalculate);
    connect(btnQuick, &QPushButton::clicked, this, &MainWindow::onQuickAnalyze);
    connect(btnReset, &QPushButton::clicked, this, &MainWindow::onResetToDefault);
    connect(btnSave, &QPushButton::clicked, this, &MainWindow::onSaveProject);
    connect(btnExport, &QPushButton::clicked, this, &MainWindow::onExportCsv);
    connect(m_btnUndo, &QPushButton::clicked, this, &MainWindow::onUndo);
    connect(m_btnRedo, &QPushButton::clicked, this, &MainWindow::onRedo);
    connect(btnOptimize, &QPushButton::clicked, this, &MainWindow::onOpenOptimizer);
    connect(btnReverse, &QPushButton::clicked, this, &MainWindow::onOpenReverse);

    // 快速评估摘要
    QGroupBox *quickGroup = new QGroupBox("传动评估");
    QVBoxLayout *quickLayout = new QVBoxLayout(quickGroup);
    m_labelQuickSummary = new QLabel("点击「快速评估」查看");
    m_labelQuickSummary->setWordWrap(true);
    m_labelQuickSummary->setObjectName("resultValue");
    quickLayout->addWidget(m_labelQuickSummary);
    leftLayout->addWidget(quickGroup);

    leftLayout->addStretch();
    mainLayout->addWidget(leftPanel);

    // ── 右侧：标签页面板 ──
    m_tabWidget = new QTabWidget;

    // Tab 1: 计算结果
    QWidget *resultTab = new QWidget;
    QVBoxLayout *rightLayout = new QVBoxLayout(resultTab);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(6);

    QGroupBox *resultGroup = new QGroupBox("计算结果");
    QVBoxLayout *resultLayout = new QVBoxLayout(resultGroup);
    resultLayout->setSpacing(4);

    auto makeResultRow = [](const QString &label, QLabel *&valueLabel,
                            QVBoxLayout *layout)
    {
        QHBoxLayout *row = new QHBoxLayout;
        QLabel *lbl = new QLabel(label);
        lbl->setFixedWidth(140);
        valueLabel = new QLabel("—");
        valueLabel->setObjectName("resultValue");
        valueLabel->setStyleSheet("font-weight: bold; color: #2563EB;");
        row->addWidget(lbl);
        row->addWidget(valueLabel);
        row->addStretch();
        layout->addLayout(row);
    };

    makeResultRow("偏转范围:", m_labelDeflectionRange, resultLayout);
    makeResultRow("最大偏转角:", m_labelMaxDeflection, resultLayout);
    makeResultRow("最小传动角:", m_labelMinTransAngle, resultLayout);
    makeResultRow("线性度 R²:", m_labelLinearity, resultLayout);
    makeResultRow("综合评价:", m_labelGrade, resultLayout);
    makeResultRow("机构类型:", m_labelGrashof, resultLayout);
    makeResultRow("死点检测:", m_labelDeadPoint, resultLayout);

    rightLayout->addWidget(resultGroup);

    QGroupBox *suggestGroup = new QGroupBox("改进建议");
    QVBoxLayout *suggestLayout = new QVBoxLayout(suggestGroup);
    m_textSuggestions = new QTextEdit;
    m_textSuggestions->setReadOnly(true);
    m_textSuggestions->setMaximumHeight(100);
    m_textSuggestions->setPlaceholderText("点击「执行计算」后将在此显示改进建议...");
    suggestLayout->addWidget(m_textSuggestions);
    rightLayout->addWidget(suggestGroup);

    rightLayout->addStretch();
    m_tabWidget->addTab(resultTab, "📊 计算结果");

    // Tab 2: 机构可视化
    QWidget *vizTab = new QWidget;
    QVBoxLayout *vizLayout = new QVBoxLayout(vizTab);
    vizLayout->setContentsMargins(0, 0, 0, 0);
    vizLayout->setSpacing(4);

    m_kinematicsView = new KinematicsView;
    vizLayout->addWidget(m_kinematicsView, 1);

    // 动画控制栏
    QHBoxLayout *animBar = new QHBoxLayout;
    m_btnPlay = new QPushButton("▶ 播放");
    m_btnPause = new QPushButton("⏸ 暂停");
    m_btnStop = new QPushButton("⏹ 停止");
    m_btnPause->setEnabled(false);
    m_btnStop->setEnabled(false);

    animBar->addStretch();
    animBar->addWidget(m_btnPlay);
    animBar->addWidget(m_btnPause);
    animBar->addWidget(m_btnStop);
    animBar->addStretch();
    vizLayout->addLayout(animBar);

    connect(m_btnPlay, &QPushButton::clicked, [this]() {
        m_kinematicsView->play(30);
        m_btnPlay->setEnabled(false);
        m_btnPause->setEnabled(true);
        m_btnStop->setEnabled(true);
    });
    connect(m_btnPause, &QPushButton::clicked, [this]() {
        m_kinematicsView->pause();
        m_btnPlay->setEnabled(true);
        m_btnPause->setEnabled(false);
    });
    connect(m_btnStop, &QPushButton::clicked, [this]() {
        m_kinematicsView->stop();
        m_btnPlay->setEnabled(true);
        m_btnPause->setEnabled(false);
        m_btnStop->setEnabled(false);
    });

    // 动画进度条
    m_animSlider = new QSlider(Qt::Horizontal);
    m_animSlider->setRange(0, 200);
    m_animSlider->setValue(0);
    vizLayout->addWidget(m_animSlider);
    connect(m_animSlider, &QSlider::valueChanged, [this](int v) {
        if (m_kinematicsView && !m_kinematicsView->isPlaying())
            m_kinematicsView->setFrame(v);
    });
    connect(m_kinematicsView, &KinematicsView::frameChanged, [this](int frame) {
        m_animSlider->blockSignals(true);
        m_animSlider->setValue(frame);
        m_animSlider->setMaximum(m_kinematicsView->frameCount() - 1);
        m_animSlider->blockSignals(false);
    });

    m_tabWidget->addTab(vizTab, "🔧 机构视图");

    // Tab 3: 曲线图
    QWidget *curveTab = new QWidget;
    QVBoxLayout *curveLayout = new QVBoxLayout(curveTab);
    curveLayout->setContentsMargins(0, 0, 0, 0);
    curveLayout->setSpacing(4);

    QHBoxLayout *curveBar = new QHBoxLayout;
    m_curveTypeCombo = new QComboBox;
    m_curveTypeCombo->addItem("θ₁ → θ₂ 映射曲线", CurvePlot::InputOutput);
    m_curveTypeCombo->addItem("传动角 μ 变化曲线", CurvePlot::TransmissionAngle);
    curveBar->addStretch();
    curveBar->addWidget(new QLabel("曲线类型:"));
    curveBar->addWidget(m_curveTypeCombo);
    curveBar->addStretch();
    curveLayout->addLayout(curveBar);

    m_curvePlot = new CurvePlot;
    curveLayout->addWidget(m_curvePlot, 1);

    connect(m_curveTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this](int idx) {
        auto type = static_cast<CurvePlot::CurveType>(
            m_curveTypeCombo->itemData(idx).toInt());
        m_curvePlot->setCurveType(type);
    });

     m_tabWidget->addTab(curveTab, "📈 特性曲线");

    // Tab 4: 解析式
    QWidget *formulaTab = new QWidget;
    QVBoxLayout *formulaLayout = new QVBoxLayout(formulaTab);
    formulaLayout->setContentsMargins(0, 0, 0, 0);
    formulaLayout->setSpacing(4);

    QGroupBox *fGroup = new QGroupBox("解析公式 (代入当前参数)");
    QVBoxLayout *fLayout = new QVBoxLayout(fGroup);
    m_textFormula = new QTextEdit;
    m_textFormula->setReadOnly(true);
    m_textFormula->setFont(QFont("Cascadia Code", 10));
    fLayout->addWidget(m_textFormula);
    formulaLayout->addWidget(fGroup);

    QGroupBox *fitGroup = new QGroupBox("多项式拟合系数 (θ₂ ≈ f(θ₁))");
    QVBoxLayout *fitLayout = new QVBoxLayout(fitGroup);
    m_textFit = new QTextEdit;
    m_textFit->setReadOnly(true);
    m_textFit->setFont(QFont("Cascadia Code", 10));
    fitLayout->addWidget(m_textFit);
    formulaLayout->addWidget(fitGroup);

    m_tabWidget->addTab(formulaTab, "📐 解析式");

    mainLayout->addWidget(m_tabWidget, 1);
}

void MainWindow::setupConnections()
{
    // SpinBox → Slider (valueChanged)
    connect(m_spinServoArm, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onSpinServoArmChanged);
    connect(m_spinHornRadius, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onSpinHornChanged);
    connect(m_spinLinkLength, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onSpinLinkChanged);
    connect(m_spinBaseDistance, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onSpinBaseChanged);

    // SpinBox → Push History
    connect(m_spinServoMin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onSpinServoMinChanged);
    connect(m_spinServoMax, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onSpinServoMaxChanged);

    // Slider → SpinBox
    connect(m_sliderServoArm, &QSlider::valueChanged,
            this, &MainWindow::onSliderServoArmChanged);
    connect(m_sliderHornRadius, &QSlider::valueChanged,
            this, &MainWindow::onSliderHornChanged);
    connect(m_sliderLinkLength, &QSlider::valueChanged,
            this, &MainWindow::onSliderLinkChanged);
    connect(m_sliderBaseDistance, &QSlider::valueChanged,
            this, &MainWindow::onSliderBaseChanged);

    // Preset combo
    connect(m_comboPresets, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onPresetChanged);

    // Assembly mode combo
    connect(m_comboAssembly, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this]() {
        onCalculate();
    });
}

void MainWindow::applyStyle()
{
    setStyleSheet(StyleConstants::GlobalStyleSheet);
}

void MainWindow::populatePresets()
{
    struct Preset {
        QString name;
        LinkageParams params;
    };

    QVector<Preset> presets = {
        {"9g 微型舵机 (标准)", {10, 12, 30, 35, -60, 60}},
        {"9g 微型舵机 (大偏转)", {12, 15, 38, 40, -60, 60}},
        {"5g 超微型舵机", {8, 10, 25, 30, -50, 50}},
        {"标准舵机 30mm基距", {15, 18, 35, 45, -60, 60}},
        {"标准舵机 40mm基距", {15, 18, 40, 50, -60, 60}},
        {"标准舵机 (保守)", {12, 16, 42, 45, -45, 45}},
        {"大舵面偏转 (±50°)", {14, 20, 38, 45, -60, 60}},
        {"线性优先设计", {10, 18, 45, 45, -55, 55}},
        {"紧凑布局", {8, 10, 22, 28, -55, 55}},
    };

    for (const auto &preset : presets) {
        m_comboPresets->addItem(preset.name);
        m_presetParams.append(preset.params);
    }
}

// SpinBox → Slider sync
void MainWindow::onSpinServoArmChanged(double v) {
    double frac = (v - 1.0) / (50.0 - 1.0);
    m_sliderServoArm->blockSignals(true);
    m_sliderServoArm->setValue(static_cast<int>(10 + frac * (500 - 10)));
    m_sliderServoArm->blockSignals(false);
    pushHistory();
}
void MainWindow::onSpinHornChanged(double v) {
    double frac = (v - 1.0) / (50.0 - 1.0);
    m_sliderHornRadius->blockSignals(true);
    m_sliderHornRadius->setValue(static_cast<int>(10 + frac * (500 - 10)));
    m_sliderHornRadius->blockSignals(false);
    pushHistory();
}
void MainWindow::onSpinLinkChanged(double v) {
    double frac = (v - 2.0) / (150.0 - 2.0);
    m_sliderLinkLength->blockSignals(true);
    m_sliderLinkLength->setValue(static_cast<int>(20 + frac * (1500 - 20)));
    m_sliderLinkLength->blockSignals(false);
    pushHistory();
}
void MainWindow::onSpinBaseChanged(double v) {
    double frac = (v - 5.0) / (200.0 - 5.0);
    m_sliderBaseDistance->blockSignals(true);
    m_sliderBaseDistance->setValue(static_cast<int>(50 + frac * (2000 - 50)));
    m_sliderBaseDistance->blockSignals(false);
    pushHistory();
}
void MainWindow::onSpinServoMinChanged(double) { pushHistory(); }
void MainWindow::onSpinServoMaxChanged(double) { pushHistory(); }

// Slider → SpinBox sync
void MainWindow::onSliderServoArmChanged(int v) {
    double frac = (v - 10) / (500.0 - 10);
    double val = 1.0 + frac * (50.0 - 1.0);
    m_spinServoArm->blockSignals(true);
    m_spinServoArm->setValue(val);
    m_spinServoArm->blockSignals(false);
}
void MainWindow::onSliderHornChanged(int v) {
    double frac = (v - 10) / (500.0 - 10);
    double val = 1.0 + frac * (50.0 - 1.0);
    m_spinHornRadius->blockSignals(true);
    m_spinHornRadius->setValue(val);
    m_spinHornRadius->blockSignals(false);
}
void MainWindow::onSliderLinkChanged(int v) {
    double frac = (v - 20) / (1500.0 - 20);
    double val = 2.0 + frac * (150.0 - 2.0);
    m_spinLinkLength->blockSignals(true);
    m_spinLinkLength->setValue(val);
    m_spinLinkLength->blockSignals(false);
}
void MainWindow::onSliderBaseChanged(int v) {
    double frac = (v - 50) / (2000.0 - 50);
    double val = 5.0 + frac * (200.0 - 5.0);
    m_spinBaseDistance->blockSignals(true);
    m_spinBaseDistance->setValue(val);
    m_spinBaseDistance->blockSignals(false);
}

// Preset selection
void MainWindow::onPresetChanged(int index)
{
    if (index <= 0) return; // skip placeholder
    int presetIdx = index - 1;
    if (presetIdx < m_presetParams.size()) {
        writeParamsToUi(m_presetParams[presetIdx]);
        pushHistory();
    }
}

// Undo / Redo
void MainWindow::pushHistory()
{
    // Remove forward history
    while (m_history.size() > m_historyIndex + 1) {
        m_history.removeLast();
    }
    m_history.append(readParamsFromUi());
    m_historyIndex = m_history.size() - 1;
    updateUndoRedoState();
}

void MainWindow::updateUndoRedoState()
{
    m_btnUndo->setEnabled(m_historyIndex > 0);
    m_btnRedo->setEnabled(m_historyIndex < m_history.size() - 1);
}

void MainWindow::onUndo()
{
    if (m_historyIndex > 0) {
        m_historyIndex--;
        writeParamsToUi(m_history[m_historyIndex]);
        updateUndoRedoState();
        onCalculate();
    }
}

void MainWindow::onRedo()
{
    if (m_historyIndex < m_history.size() - 1) {
        m_historyIndex++;
        writeParamsToUi(m_history[m_historyIndex]);
        updateUndoRedoState();
        onCalculate();
    }
}

// Open Reverse (输出优先) Dialog
void MainWindow::onOpenReverse()
{
    auto params = readParamsFromUi();
    ReverseDialog dlg(this);
    dlg.setFixedParams(params.hornRadius, params.baseDistance, 0);
    connect(&dlg, &ReverseDialog::paramsSelected, this, [this](const LinkageParams &p) {
        writeParamsToUi(p);
        pushHistory();
        onCalculate();
    });
    dlg.exec();
}

// Open Optimizer Dialog
void MainWindow::onOpenOptimizer()
{
    auto params = readParamsFromUi();
    OptimizeDialog dlg(this);
    dlg.setFixedParams(params.baseDistance, params.servoAngleMin, params.servoAngleMax);
    connect(&dlg, &OptimizeDialog::paramsSelected, this, [this](const LinkageParams &p) {
        writeParamsToUi(p);
        pushHistory();
        onCalculate();
    });
    dlg.exec();
}

// CSV Export
void MainWindow::onExportCsv()
{
    auto params = readParamsFromUi();
    auto asmMode = static_cast<KinematicSolver::AssemblyMode>(
        m_comboAssembly->currentData().toInt());
    m_solver.setAssemblyMode(asmMode);
    m_solver.setParams(params);
    auto analysis = m_solver.sweepRange(200);

    auto qa = TransmissionAnalyzer::quickAssess(params);

    QString fileName = QFileDialog::getSaveFileName(
        this, "导出完整工程报告", "servolink_report.csv",
        "CSV Files (*.csv);;All Files (*)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "导出失败",
            QString("无法写入文件: %1\n%2").arg(fileName, file.errorString()));
        return;
    }

    QTextStream out(&file);
    auto modeName = m_comboAssembly->currentText();

    // ═══════════════════════════════════════════
    // 区块 1: 文件头
    // ═══════════════════════════════════════════
    out << "# ServoLink 工程报告\n";
    out << "# 导出时间: " << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n";
    out << "# 软件版本: v0.6.0\n";
    out << "#\n";

    // ═══════════════════════════════════════════
    // 区块 2: 机构参数
    // ═══════════════════════════════════════════
    out << "## 机构参数\n";
    out << "r1_伺服臂半径_mm," << params.servoArmRadius << "\n";
    out << "r2_舵角半径_mm," << params.hornRadius << "\n";
    out << "L_连杆长度_mm," << params.linkLength << "\n";
    out << "d_基距_mm," << params.baseDistance << "\n";
    out << "基距倾角_deg," << params.baseAngle << "\n";
    out << "theta1_min_deg," << params.servoAngleMin << "\n";
    out << "theta1_max_deg," << params.servoAngleMax << "\n";
    out << "装配模式," << modeName << "\n";
    out << "\n";
    out << "## 定位标注\n";
    out << "舵面原点_theta2_deg," << params.hornOriginDeg << "\n";
    out << "舵面上限_theta2_deg," << params.hornLimitUpDeg << "\n";
    out << "舵面下限_theta2_deg," << params.hornLimitLoDeg << "\n";
    out << "伺服上限_theta1_deg," << params.servoLimitMaxDeg << "\n";
    out << "伺服下限_theta1_deg," << params.servoLimitMinDeg << "\n";
    out << "\n";

    // ═══════════════════════════════════════════
    // 区块 3: 汇总结果
    // ═══════════════════════════════════════════
    out << "## 汇总结果\n";
    out << "偏转范围_deg," << analysis.deflectionRange << "\n";
    out << "最大偏转角_deg," << analysis.maxDeflection << "\n";
    out << "最小传动角_deg," << analysis.minTransmissionAngle << "\n";
    out << "线性度_R2," << analysis.linearityScore << "\n";
    out << "质量等级," << analysis.gradeDescription() << "\n";
    out << "机构类型," << qa.summary << "\n";
    out << "死点检测," << (analysis.hasDeadPoint ? "是" : "否") << "\n";
    out << "\n";

    // ═══════════════════════════════════════════
    // 区块 4: 解析解 — 左中右三点
    // ═══════════════════════════════════════════
    out << "## 解析解（闭式/开式对比）\n";
    out << "theta1_deg,闭式_theta2_deg,闭式_传动角_deg,开式_theta2_deg,开式_传动角_deg\n";
    double points[] = {params.servoAngleMin, 0.0, params.servoAngleMax};
    for (double t1 : points) {
        auto an = m_solver.solveForwardAnalytic(t1);
        if (an.has_value()) {
            out << t1 << ","
                << an->first.outputAngle << "," << an->first.transmissionAngle << ","
                << an->second.outputAngle << "," << an->second.transmissionAngle << "\n";
        }
    }
    out << "\n";

    // ═══════════════════════════════════════════
    // 区块 5: 多项式拟合系数
    // ═══════════════════════════════════════════
    out << "## 多项式拟合系数 (θ₂ ≈ f(θ₁))\n";
    {
        auto &sweep = analysis.sweepResults;
        if (sweep.size() >= 3) {
            int n = sweep.size();
            double sx=0,sy=0,sxy=0,sx2=0;
            for (auto &s : sweep) { double x=s.inputAngle,y=s.outputAngle; sx+=x;sy+=y;sxy+=x*y;sx2+=x*x; }
            double den = n*sx2-sx*sx;
            double a1 = (n*sxy-sx*sy)/den;
            double b1 = (sy-a1*sx)/n;
            double ssRes=0,ssTot=0,meanY=sy/n;
            for (auto &s : sweep) { double yp=a1*s.inputAngle+b1; ssRes+=(s.outputAngle-yp)*(s.outputAngle-yp); ssTot+=(s.outputAngle-meanY)*(s.outputAngle-meanY); }
            double r2_1 = ssTot>1e-10 ? 1.0-ssRes/ssTot : 1.0;
            out << "线性_a0," << b1 << "\n";
            out << "线性_a1," << a1 << "\n";
            out << "线性_R2," << r2_1 << "\n";
            out << "\n";
        }
    }
    out << "\n";

    // ═══════════════════════════════════════════
    // 区块 6: 全范围扫描数据 (200行)
    // ═══════════════════════════════════════════
    out << "## 全范围扫描数据\n";
    out << "theta1_deg,theta2_deg,传动角_deg,力放大比,Ax_mm,Ay_mm,Bx_mm,By_mm\n";
    for (const auto &s : analysis.sweepResults) {
        out << s.inputAngle << ","
            << s.outputAngle << ","
            << s.transmissionAngle << ","
            << s.mechanicalAdvantage << ","
            << s.jointA.x() << ","
            << s.jointA.y() << ","
            << s.jointB.x() << ","
            << s.jointB.y() << "\n";
    }
    file.close();

    QMessageBox::information(this, "导出成功",
        QString("已将完整工程报告 (%1 行) 导出到:\n%2")
            .arg(analysis.sweepResults.size() + 20)
            .arg(fileName));
}

// ── 参数读写 ──

LinkageParams MainWindow::readParamsFromUi() const
{
    LinkageParams p;
    p.servoArmRadius = m_spinServoArm->value();
    p.hornRadius = m_spinHornRadius->value();
    p.linkLength = m_spinLinkLength->value();
    p.baseDistance = m_spinBaseDistance->value();
    p.servoAngleMin = m_spinServoMin->value();
    p.servoAngleMax = m_spinServoMax->value();
    p.baseAngle = m_spinBaseAngle->value();
    p.hornOriginDeg = m_spinHornOrigin->value();
    p.hornLimitUpDeg = m_spinHornLimitUp->value();
    p.hornLimitLoDeg = m_spinHornLimitLo->value();
    p.servoLimitMinDeg = m_spinServoLimitMin->value();
    p.servoLimitMaxDeg = m_spinServoLimitMax->value();
    return p;
}

void MainWindow::writeParamsToUi(const LinkageParams &params, bool updateSliders)
{
    m_spinServoArm->setValue(params.servoArmRadius);
    m_spinHornRadius->setValue(params.hornRadius);
    m_spinLinkLength->setValue(params.linkLength);
    m_spinBaseDistance->setValue(params.baseDistance);
    m_spinServoMin->setValue(params.servoAngleMin);
    m_spinServoMax->setValue(params.servoAngleMax);
    m_spinBaseAngle->setValue(params.baseAngle);
    m_spinHornOrigin->setValue(params.hornOriginDeg);
    m_spinHornLimitUp->setValue(params.hornLimitUpDeg);
    m_spinHornLimitLo->setValue(params.hornLimitLoDeg);
    m_spinServoLimitMin->setValue(params.servoLimitMinDeg);
    m_spinServoLimitMax->setValue(params.servoLimitMaxDeg);
}

void MainWindow::displayAnalysis(const LinkageAnalysis &analysis)
{
    if (!analysis.isAssemblyable) {
        m_labelDeflectionRange->setText("机构不可装配");
        m_labelGrade->setText("不良");
        return;
    }

    m_labelDeflectionRange->setText(
        QString("%1 °").arg(analysis.deflectionRange, 0, 'f', 1));
    m_labelMaxDeflection->setText(
        QString("%1 °").arg(analysis.maxDeflection, 0, 'f', 1));
    m_labelMinTransAngle->setText(
        QString("%1 °").arg(analysis.minTransmissionAngle, 0, 'f', 1));
    m_labelLinearity->setText(
        QString::number(analysis.linearityScore, 'f', 4));
    m_labelGrade->setText(analysis.gradeDescription());

    if (analysis.minTransmissionAngle >= 30.0) {
        m_labelMinTransAngle->setStyleSheet("font-weight: bold; color: #10B981;");
    } else if (analysis.minTransmissionAngle >= 20.0) {
        m_labelMinTransAngle->setStyleSheet("font-weight: bold; color: #F59E0B;");
    } else {
        m_labelMinTransAngle->setStyleSheet("font-weight: bold; color: #EF4444;");
    }
}

void MainWindow::displayQuickAssessment(
    const TransmissionAnalyzer::QuickAssessment &qa)
{
    m_labelGrashof->setText(TransmissionAnalyzer::grashofTypeName(qa.grashofType));
    m_labelDeadPoint->setText(qa.likelyHasDeadPoint ? "⚠ 是" : "否");
    m_labelQuickSummary->setText(qa.summary);
}

void MainWindow::onCalculate()
{
    auto params = readParamsFromUi();
    auto vr = params.validate();
    if (!vr.valid) {
        QMessageBox::warning(this, "参数错误", vr.message);
        return;
    }

    // Read assembly mode
    auto asmMode = static_cast<KinematicSolver::AssemblyMode>(
        m_comboAssembly->currentData().toInt());
    m_solver.setAssemblyMode(asmMode);

    m_solver.setParams(params);
    auto analysis = m_solver.sweepRange(200);
    displayAnalysis(analysis);

    auto qa = TransmissionAnalyzer::quickAssess(params);
    displayQuickAssessment(qa);

    // 解析解对比（取中点角度的解析解，用于验证标签）
    double midAngle = (params.servoAngleMin + params.servoAngleMax) * 0.5;
    auto analytic = m_solver.solveForwardAnalytic(midAngle);
    if (analytic.has_value()) {
        double analyticClosed = analytic->first.outputAngle;
        double analyticOpen   = analytic->second.outputAngle;
        // 比较迭代解与解析解在两种模式下的误差
        auto iterClosed = m_solver.solveForward(midAngle);  // 使用当前装配模式
        m_solver.setAssemblyMode(
            m_solver.assemblyMode() == KinematicSolver::AssemblyMode::Closed
                ? KinematicSolver::AssemblyMode::Open
                : KinematicSolver::AssemblyMode::Closed);
        auto iterOpen = m_solver.solveForward(midAngle);
        m_solver.setAssemblyMode(asmMode); // 恢复原模式
        // 验证：解析+闭式应与迭代+闭式一致（偏差 < 0.01°）
        double errClosed = iterClosed.has_value()
            ? qAbs(iterClosed->outputAngle - analyticClosed) : 999.0;
        double errOpen   = iterOpen.has_value()
            ? qAbs(iterOpen->outputAngle - analyticOpen) : 999.0;
        if (errClosed < 0.01 && errOpen < 0.01) {
            // 标签正确，无需修正
        }
    }

    auto suggestions = TransmissionAnalyzer::improvementSuggestions(params, analysis);
    m_textSuggestions->setText(suggestions.join("\n• "));

    // Phase 3: Update visual components
    m_kinematicsView->setParams(params);
    m_kinematicsView->setSweepData(analysis.sweepResults);
    m_curvePlot->setAnalysisData(analysis);

    // ── 填充解析式 Tab ──
    {
        double r1 = params.servoArmRadius, r2 = params.hornRadius;
        double L = params.linkLength, d = params.baseDistance;
        double baseRad = UnitSystem::degToRad(params.baseAngle);
        double ox2 = d * cos(baseRad), oy2 = d * sin(baseRad);
        QString f;
        f += "═══ 闭环方程解析解 ═══\n\n";
        f += QString("θ₂ = atan2(-(2·r₂·(o_y - r₁·sinθ₁)), -(2·r₂·(o_x - r₁·cosθ₁)))\n");
        f += QString("      ± arccos(-A / √(B² + C²))\n\n");
        f += QString("代入参数 (r₁=%1, r₂=%2, L=%3, d=%4, α=%5°):\n\n")
            .arg(r1,0,'f',1).arg(r2,0,'f',1).arg(L,0,'f',1).arg(d,0,'f',1)
            .arg(params.baseAngle,0,'f',1);
        f += QString("A = o_x²+o_y²+r₁²+r₂²-L² - 2·r₁(o_x·cosθ₁ + o_y·sinθ₁)\n");
        f += QString("  = %1²+%2²+%3²+%4²-%5² - 2·%3(%1·cosθ₁ + %2·sinθ₁)\n\n")
            .arg(ox2,0,'f',2).arg(oy2,0,'f',2).arg(r1,0,'f',1).arg(r2,0,'f',1).arg(L,0,'f',1);
        f += QString("B = 2·r₂·(o_x - r₁·cosθ₁)\n");
        f += QString("C = 2·r₂·(o_y - r₁·sinθ₁)\n\n");

        f += "═══ 三关键点解析值 ═══\n\n";
        f += "θ₁       闭式 θ₂    闭式 μ    |  开式 θ₂    开式 μ\n";
        f += "─────────────────────────────────────────────────\n";
        double pts[] = {params.servoAngleMin, 0.0, params.servoAngleMax};
        for (double t1 : pts) {
            auto an = m_solver.solveForwardAnalytic(t1);
            if (an.has_value()) {
                f += QString("%1°  %2°  %3°  |  %4°  %5°\n")
                    .arg(t1,5,'f',1)
                    .arg(an->first.outputAngle,6,'f',2)
                    .arg(an->first.transmissionAngle,6,'f',2)
                    .arg(an->second.outputAngle,6,'f',2)
                    .arg(an->second.transmissionAngle,6,'f',2);
            }
        }
        m_textFormula->setText(f);

        // 多项式拟合
        auto &sweep = analysis.sweepResults;
        if (sweep.size() >= 3) {
            int n = sweep.size();
            // 1阶 linear
            double sx=0,sy=0,sxy=0,sx2=0,sy2=0;
            for (auto &s : sweep) { double x=s.inputAngle,y=s.outputAngle; sx+=x;sy+=y;sxy+=x*y;sx2+=x*x;sy2+=y*y; }
            double den = n*sx2-sx*sx;
            double a1 = (n*sxy-sx*sy)/den;
            double b1 = (sy-a1*sx)/n;
            double ssRes=0,ssTot=0,meanY=sy/n;
            for (auto &s : sweep) { double yp=a1*s.inputAngle+b1; ssRes+=(s.outputAngle-yp)*(s.outputAngle-yp); ssTot+=(s.outputAngle-meanY)*(s.outputAngle-meanY); }
            double r2_1 = ssTot>1e-10 ? 1.0-ssRes/ssTot : 1.0;
            // 2阶 quadratic
            double sxx=0,sxxx=0,sxxxx=0,sxxy=0;
            for (auto &s : sweep) { double x=s.inputAngle,y=s.outputAngle,x2=x*x; sxx+=x2;sxxx+=x*x2;sxxxx+=x2*x2;sxxy+=x2*y; }
            double m2[3][3]={{1.*n,sx,sxx},{sx,sxx,sxxx},{sxx,sxxx,sxxxx}};
            double rhs2[3]={sy,sxy,sxxy};
            // Simple Gaussian elimination for 3x3
            for(int i=0;i<3;i++){
                double piv=m2[i][i];
                for(int j=i;j<3;j++) m2[i][j]/=piv; rhs2[i]/=piv;
                for(int k=i+1;k<3;k++){
                    double fk=m2[k][i];
                    for(int j=i;j<3;j++) m2[k][j]-=fk*m2[i][j]; rhs2[k]-=fk*rhs2[i];
                }
            }
            double c2[3]; c2[2]=rhs2[2]; c2[1]=rhs2[1]-m2[1][2]*c2[2]; c2[0]=rhs2[0]-m2[0][1]*c2[1]-m2[0][2]*c2[2];
            double ssRes2=0;
            for(auto &s:sweep){ double yp=c2[0]+c2[1]*s.inputAngle+c2[2]*s.inputAngle*s.inputAngle; ssRes2+=(s.outputAngle-yp)*(s.outputAngle-yp); }
            double r2_2 = ssTot>1e-10 ? 1.0-ssRes2/ssTot : 1.0;

            QString fit;
            fit += QString("1次 (线性): θ₂ = %1 + %2·θ₁   R²=%3\n")
                .arg(b1,0,'f',2).arg(a1,0,'f',4).arg(r2_1,0,'f',4);
            fit += QString("2次 (二次): θ₂ = %1 + %2·θ₁ + %3·θ₁²   R²=%4\n")
                .arg(c2[0],0,'f',2).arg(c2[1],0,'f',4).arg(c2[2],0,'f',6).arg(r2_2,0,'f',4);
            fit += QString("\n推荐: %1次多项式 (R²=%2)")
                .arg(r2_2>r2_1?"2":"1").arg(QString::number(qMax(r2_1,r2_2),'f',4));
            m_textFit->setText(fit);
        }
    }
}

void MainWindow::onQuickAnalyze()
{
    auto params = readParamsFromUi();
    auto vr = params.validate();
    if (!vr.valid) {
        QMessageBox::warning(this, "参数错误", vr.message);
        return;
    }
    auto qa = TransmissionAnalyzer::quickAssess(params);
    displayQuickAssessment(qa);
}

void MainWindow::onResetToDefault()
{
    writeParamsToUi(LinkageParams());
    m_labelDeflectionRange->setText("—");
    m_labelMaxDeflection->setText("—");
    m_labelMinTransAngle->setText("—");
    m_labelLinearity->setText("—");
    m_labelGrade->setText("—");
    m_labelGrashof->setText("—");
    m_labelDeadPoint->setText("—");
    m_labelQuickSummary->setText("点击「快速评估」查看");
    m_textSuggestions->clear();
    pushHistory();
}

void MainWindow::onSaveProject()
{
    if (!m_storage) { QMessageBox::warning(this, "错误", "存储后端未初始化"); return; }
    auto params = readParamsFromUi();
    QJsonObject snapshot;
    snapshot["version"] = 1;
    snapshot["appName"] = "ServoLink";
    snapshot["savedAt"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    snapshot["currentParams"] = params.toJson();
    auto result = m_storage->saveAll(snapshot);
    if (result.isSuccess()) {
        QMessageBox::information(this, "保存成功",
            QString("项目已保存至:\n%1").arg(
                dynamic_cast<JsonStorageBackend*>(m_storage)->filePath()));
    } else {
        QMessageBox::critical(this, "保存失败", result.joinMessages());
    }
}

void MainWindow::onLoadProject()
{
    if (!m_storage) { QMessageBox::warning(this, "错误", "存储后端未初始化"); return; }
    auto snapshot = m_storage->loadAll();
    if (!snapshot.has_value()) {
        QMessageBox::information(this, "加载", "未找到已保存的项目数据。");
        return;
    }
    auto obj = snapshot.value();
    auto paramsObj = obj["currentParams"].toObject();
    if (paramsObj.isEmpty()) {
        QMessageBox::information(this, "加载", "文件中无有效参数数据"); return;
    }
    LinkageParams params = LinkageParams::fromJson(paramsObj);
    writeParamsToUi(params);
    pushHistory();
    onCalculate();
    QMessageBox::information(this, "加载成功",
        QString("已加载参数（保存于 %1）").arg(obj["savedAt"].toString()));
}

void MainWindow::loadStoredParams()
{
    if (!m_storage || !dynamic_cast<JsonStorageBackend*>(m_storage)->fileExists()) return;
    auto snapshot = m_storage->loadAll();
    if (!snapshot.has_value()) return;
    auto obj = snapshot.value();
    auto paramsObj = obj["currentParams"].toObject();
    if (paramsObj.isEmpty()) return;
    LinkageParams params = LinkageParams::fromJson(paramsObj);
    writeParamsToUi(params);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_storage) {
        auto params = readParamsFromUi();
        QJsonObject snapshot;
        snapshot["version"] = 1;
        snapshot["appName"] = "ServoLink";
        snapshot["savedAt"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        snapshot["currentParams"] = params.toJson();
        m_storage->saveAll(snapshot);
    }
    QMainWindow::closeEvent(event);
}
