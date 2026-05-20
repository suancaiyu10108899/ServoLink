#include "ReverseDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QApplication>

ReverseDialog::ReverseDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("输出优先设计 — 舵面偏转 → 伺服参数"));
    setMinimumSize(620, 520);
    setupUi();
}

void ReverseDialog::setFixedParams(double hornR2, double baseD, double hornR)
{
    m_spinHornR2->setValue(hornR2);
    m_spinBaseDist->setValue(baseD);
}

void ReverseDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // ── 约束输入 ──
    QGroupBox *constraintGroup = new QGroupBox(QStringLiteral("设计约束"));
    QFormLayout *formLayout = new QFormLayout(constraintGroup);

    m_spinHornR2 = new QDoubleSpinBox;
    m_spinHornR2->setRange(2, 50);
    m_spinHornR2->setValue(15);
    m_spinHornR2->setSuffix(QStringLiteral(" mm"));
    formLayout->addRow(QStringLiteral("舵角半径 r₂:"), m_spinHornR2);

    m_spinBaseDist = new QDoubleSpinBox;
    m_spinBaseDist->setRange(10, 200);
    m_spinBaseDist->setValue(45);
    m_spinBaseDist->setSuffix(QStringLiteral(" mm"));
    formLayout->addRow(QStringLiteral("基距 d:"), m_spinBaseDist);

    m_spinTargetRange = new QDoubleSpinBox;
    m_spinTargetRange->setRange(10, 180);
    m_spinTargetRange->setValue(80);
    m_spinTargetRange->setSuffix(QStringLiteral(" °"));
    formLayout->addRow(QStringLiteral("目标偏转范围:"), m_spinTargetRange);

    m_spinTargetCenter = new QDoubleSpinBox;
    m_spinTargetCenter->setRange(-30, 30);
    m_spinTargetCenter->setValue(0);
    m_spinTargetCenter->setSuffix(QStringLiteral(" °"));
    formLayout->addRow(QStringLiteral("目标中心偏移:"), m_spinTargetCenter);

    m_spinMinTransAngle = new QDoubleSpinBox;
    m_spinMinTransAngle->setRange(10, 60);
    m_spinMinTransAngle->setValue(28);
    m_spinMinTransAngle->setSuffix(QStringLiteral(" °"));
    formLayout->addRow(QStringLiteral("最小传动角:"), m_spinMinTransAngle);

    m_spinTopN = new QDoubleSpinBox;
    m_spinTopN->setRange(1, 20);
    m_spinTopN->setValue(8);
    m_spinTopN->setDecimals(0);
    formLayout->addRow(QStringLiteral("显示 Top N:"), m_spinTopN);

    mainLayout->addWidget(constraintGroup);

    // ── 进度和状态 ──
    QHBoxLayout *statusRow = new QHBoxLayout;
    m_progressBar = new QProgressBar;
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_labelStatus = new QLabel(QStringLiteral("就绪"));

    statusRow->addWidget(m_progressBar);
    statusRow->addWidget(m_labelStatus);
    mainLayout->addLayout(statusRow);

    // ── 结果表格 ──
    m_tableResults = new QTableWidget;
    m_tableResults->setColumnCount(6);
    m_tableResults->setHorizontalHeaderLabels({
        QStringLiteral("r₁ (mm)"),
        QStringLiteral("L (mm)"),
        QStringLiteral("偏转 (°)"),
        QStringLiteral("传动角 (°)"),
        QStringLiteral("伺服范围 (°)"),
        QStringLiteral("评分")
    });
    m_tableResults->horizontalHeader()->setStretchLastSection(true);
    m_tableResults->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableResults->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableResults->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mainLayout->addWidget(m_tableResults, 1);

    // ── 按钮 ──
    QHBoxLayout *buttonRow = new QHBoxLayout;
    m_btnSearch = new QPushButton(QStringLiteral("开始搜索"));
    m_btnApply = new QPushButton(QStringLiteral("应用选中参数"));
    m_btnApply->setEnabled(false);
    QPushButton *btnCancel = new QPushButton(QStringLiteral("关闭"));

    buttonRow->addStretch();
    buttonRow->addWidget(m_btnSearch);
    buttonRow->addWidget(m_btnApply);
    buttonRow->addWidget(btnCancel);
    mainLayout->addLayout(buttonRow);

    connect(m_btnSearch, &QPushButton::clicked, this, &ReverseDialog::onSearch);
    connect(m_btnApply, &QPushButton::clicked, this, &ReverseDialog::onApplySelected);
    connect(btnCancel, &QPushButton::clicked, this, &ReverseDialog::onCancel);
}

void ReverseDialog::onSearch()
{
    double r2 = m_spinHornR2->value();
    double d = m_spinBaseDist->value();
    double targetRange = m_spinTargetRange->value();
    double targetCenter = m_spinTargetCenter->value();
    double minTrans = m_spinMinTransAngle->value();
    int topN = static_cast<int>(m_spinTopN->value());

    m_btnSearch->setEnabled(false);
    m_labelStatus->setText(QStringLiteral("正在搜索最优伺服参数..."));
    m_progressBar->setValue(10);
    QApplication::processEvents();

    m_results.clear();

    KinematicSolver solver;
    ParameterOptimizer::OptimizationResult best;
    best.score = -1;

    double r1Step = 1.0;
    double lStep  = 1.0;

    for (double r1 = 3; r1 <= 25; r1 += r1Step) {
        for (double L = 5; L <= 100; L += lStep) {
            // 猜 θ₁ 范围：需要满足闭环且传动角达标
            // 遍历 θ₁ 范围搜索
            int searchSteps = 180;
            double theta2Min = 1e9, theta2Max = -1e9;
            double minTransFound = 90;
            bool anyValid = false;

            LinkageParams p;
            p.servoArmRadius = r1;
            p.hornRadius = r2;
            p.linkLength = L;
            p.baseDistance = d;
            p.servoAngleMin = -90;
            p.servoAngleMax = 90;

            auto vr = p.validate();
            if (!vr.valid) continue;

            solver.setParams(p);

            for (int i = 0; i <= searchSteps; ++i) {
                double t1 = -90 + 180.0 * i / searchSteps;
                auto state = solver.solveForward(t1);
                if (state.has_value()) {
                    if (state->transmissionAngle >= minTrans) {
                        anyValid = true;
                        if (state->outputAngle < theta2Min) theta2Min = state->outputAngle;
                        if (state->outputAngle > theta2Max) theta2Max = state->outputAngle;
                        if (state->transmissionAngle < minTransFound) minTransFound = state->transmissionAngle;
                    }
                }
            }

            if (!anyValid || theta2Max - theta2Min < targetRange * 0.9) continue;

            // 评分
            double rangeScore = 1.0 - qAbs(theta2Max - theta2Min - targetRange) / targetRange;
            if (rangeScore < 0) rangeScore = 0;
            double transScore = (minTransFound - 15.0) / (45.0);
            if (transScore > 1) transScore = 1;
            if (transScore < 0) transScore = 0;
            double score = 0.6 * rangeScore + 0.4 * transScore;

            ParameterOptimizer::OptimizationResult result;
            result.params = p;
            result.score = score;
            result.description = QString("r₁=%1 L=%2").arg(r1, 0, 'f', 1).arg(L, 0, 'f', 1);

            // 需要分析对象
            solver.setParams(p);
            auto analysis = solver.sweepRange(50);
            result.analysis = analysis;

            if (score > best.score) {
                best = result;
            }
            m_results.append(result);
        }
        m_progressBar->setValue(10 + static_cast<int>(80.0 * (r1 - 3) / (25 - 3)));
        QApplication::processEvents();
    }

    // 排序取 topN
    std::sort(m_results.begin(), m_results.end());
    if (m_results.size() > topN) m_results.resize(topN);

    m_progressBar->setValue(100);
    m_labelStatus->setText(QStringLiteral("找到 %1 个候选方案").arg(m_results.size()));

    displayResults(m_results);
    m_btnSearch->setEnabled(true);
    m_btnApply->setEnabled(!m_results.isEmpty());
}

void ReverseDialog::displayResults(
    const QVector<ParameterOptimizer::OptimizationResult> &results)
{
    m_tableResults->setRowCount(0);
    m_tableResults->setRowCount(results.size());

    for (int i = 0; i < results.size(); ++i) {
        const auto &r = results[i];

        auto setCell = [&](int col, const QString &text, bool bold = false) {
            auto *item = new QTableWidgetItem(text);
            if (bold) {
                QFont f = item->font();
                f.setBold(true);
                item->setFont(f);
            }
            item->setTextAlignment(Qt::AlignCenter);
            m_tableResults->setItem(i, col, item);
        };

        setCell(0, QString::number(r.params.servoArmRadius, 'f', 1));
        setCell(1, QString::number(r.params.linkLength, 'f', 1));
        setCell(2, QString::number(r.analysis.deflectionRange, 'f', 1));
        setCell(3, QString::number(r.analysis.minTransmissionAngle, 'f', 1));
        setCell(4, QString("%1 ~ %2 °").arg(r.analysis.params.servoAngleMin).arg(r.analysis.params.servoAngleMax));
        setCell(5, QString::number(r.score, 'f', 3), true);
    }

    m_tableResults->resizeColumnsToContents();
}

void ReverseDialog::onApplySelected()
{
    int row = m_tableResults->currentRow();
    if (row < 0 || row >= m_results.size()) {
        QMessageBox::information(this, QStringLiteral("提示"),
            QStringLiteral("请先在表格中选中一行"));
        return;
    }

    emit paramsSelected(m_results[row].params);
    accept();
}

void ReverseDialog::onCancel()
{
    reject();
}
