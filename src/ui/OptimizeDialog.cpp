#include "OptimizeDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QApplication>

OptimizeDialog::OptimizeDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("参数优化器"));
    setMinimumSize(600, 500);
    setupUi();
}

void OptimizeDialog::setFixedParams(double baseDistance, double servoMin, double servoMax)
{
    m_spinBaseDist->setValue(baseDistance);
    m_spinServoMin->setValue(servoMin);
    m_spinServoMax->setValue(servoMax);
}

void OptimizeDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // ── 约束输入 ──
    QGroupBox *constraintGroup = new QGroupBox(QStringLiteral("优化约束"));
    QFormLayout *formLayout = new QFormLayout(constraintGroup);

    m_spinBaseDist = new QDoubleSpinBox;
    m_spinBaseDist->setRange(10, 200);
    m_spinBaseDist->setValue(45);
    m_spinBaseDist->setSuffix(QStringLiteral(" mm"));
    formLayout->addRow(QStringLiteral("基距 d:"), m_spinBaseDist);

    m_spinServoMin = new QDoubleSpinBox;
    m_spinServoMin->setRange(-120, 0);
    m_spinServoMin->setValue(-60);
    m_spinServoMin->setSuffix(QStringLiteral(" °"));
    formLayout->addRow(QStringLiteral("伺服角 最小:"), m_spinServoMin);

    m_spinServoMax = new QDoubleSpinBox;
    m_spinServoMax->setRange(0, 120);
    m_spinServoMax->setValue(60);
    m_spinServoMax->setSuffix(QStringLiteral(" °"));
    formLayout->addRow(QStringLiteral("伺服角 最大:"), m_spinServoMax);

    m_spinTargetRange = new QDoubleSpinBox;
    m_spinTargetRange->setRange(10, 180);
    m_spinTargetRange->setValue(80);
    m_spinTargetRange->setSuffix(QStringLiteral(" °"));
    formLayout->addRow(QStringLiteral("目标偏转范围:"), m_spinTargetRange);

    m_spinMinTransAngle = new QDoubleSpinBox;
    m_spinMinTransAngle->setRange(10, 60);
    m_spinMinTransAngle->setValue(30);
    m_spinMinTransAngle->setSuffix(QStringLiteral(" °"));
    formLayout->addRow(QStringLiteral("最小传动角:"), m_spinMinTransAngle);

    m_spinTopN = new QDoubleSpinBox;
    m_spinTopN->setRange(1, 20);
    m_spinTopN->setValue(5);
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
        QStringLiteral("r₂ (mm)"),
        QStringLiteral("L (mm)"),
        QStringLiteral("偏转 (°)"),
        QStringLiteral("传动角 (°)"),
        QStringLiteral("评分")
    });
    m_tableResults->horizontalHeader()->setStretchLastSection(true);
    m_tableResults->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableResults->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableResults->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mainLayout->addWidget(m_tableResults, 1);

    // ── 按钮 ──
    QHBoxLayout *buttonRow = new QHBoxLayout;
    m_btnOptimize = new QPushButton(QStringLiteral("开始优化"));
    m_btnApply = new QPushButton(QStringLiteral("应用选中参数"));
    m_btnApply->setEnabled(false);
    QPushButton *btnCancel = new QPushButton(QStringLiteral("关闭"));

    buttonRow->addStretch();
    buttonRow->addWidget(m_btnOptimize);
    buttonRow->addWidget(m_btnApply);
    buttonRow->addWidget(btnCancel);
    mainLayout->addLayout(buttonRow);

    connect(m_btnOptimize, &QPushButton::clicked, this, &OptimizeDialog::onOptimize);
    connect(m_btnApply, &QPushButton::clicked, this, &OptimizeDialog::onApplySelected);
    connect(btnCancel, &QPushButton::clicked, this, &OptimizeDialog::onCancel);
}

ParameterOptimizer::Constraints OptimizeDialog::readConstraints() const
{
    ParameterOptimizer::Constraints c;
    c.baseDistance = m_spinBaseDist->value();
    c.servoAngleMin = m_spinServoMin->value();
    c.servoAngleMax = m_spinServoMax->value();
    c.minDeflectionRange = m_spinTargetRange->value();
    c.minTransmissionAngle = m_spinMinTransAngle->value();
    c.maxHornRadius = 30.0;
    c.maxServoArmRadius = 30.0;
    c.maxLinkLength = 120.0;
    return c;
}

void OptimizeDialog::onOptimize()
{
    auto constraints = readConstraints();
    m_optimizer.setConstraints(constraints);

    m_btnOptimize->setEnabled(false);
    m_labelStatus->setText(QStringLiteral("正在优化..."));
    m_progressBar->setValue(10);
    QApplication::processEvents();

    int topN = static_cast<int>(m_spinTopN->value());
    m_results = m_optimizer.optimize(topN, 15);

    m_progressBar->setValue(100);
    m_labelStatus->setText(QStringLiteral("找到 %1 个候选方案").arg(m_results.size()));

    displayResults(m_results);
    m_btnOptimize->setEnabled(true);
    m_btnApply->setEnabled(!m_results.isEmpty());
}

void OptimizeDialog::displayResults(
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
        setCell(1, QString::number(r.params.hornRadius, 'f', 1));
        setCell(2, QString::number(r.params.linkLength, 'f', 1));
        setCell(3, QString::number(r.analysis.deflectionRange, 'f', 1));
        setCell(4, QString::number(r.analysis.minTransmissionAngle, 'f', 1));
        setCell(5, QString::number(r.score, 'f', 3), true);
    }

    m_tableResults->resizeColumnsToContents();
}

void OptimizeDialog::onApplySelected()
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

void OptimizeDialog::onCancel()
{
    reject();
}
