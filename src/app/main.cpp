#include <QApplication>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

#include "MainWindow.h"
#include "JsonStorageBackend.h"
#include "KinematicSolver.h"
#include "TransmissionAnalyzer.h"
#include "ParameterOptimizer.h"

/**
 * @brief 应用入口
 *
 * 职责：
 * 1. 初始化 QApplication
 * 2. 确定数据存储路径（AppLocalDataLocation）
 * 3. 组装各层依赖
 * 4. 加载已有项目数据
 * 5. 启动主窗口
 *
 * 设计原则：
 * - main.cpp 只做"装配"（Wiring），不包含业务逻辑
 * - 所有依赖在此处注入，不引入全局变量
 */
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 设置应用信息（QStandardPaths 会用到）
    app.setApplicationName("ServoLink");
    app.setOrganizationName("ServoLink");
    app.setApplicationVersion("0.5.0");

    // ── 确定数据存储路径 ──
    // 遵循 ADR 0003 策略：使用 AppLocalDataLocation
    // Windows: C:/Users/<用户>/AppData/Local/ServoLink/ServoLink/
    QString dataDir = QStandardPaths::writableLocation(
        QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(dataDir);

    QString dataFilePath = dataDir + "/servolink.json";

    qDebug() << "[ServoLink] Data path:" << dataFilePath;

    // ── 组装依赖 ──
    JsonStorageBackend storage(dataFilePath);

    // 启动时加载已有数据
    if (storage.fileExists()) {
        auto snapshot = storage.loadAll();
        if (snapshot.has_value()) {
            qDebug() << "[ServoLink] Loaded existing project data from"
                     << dataFilePath;
        }
    }

    // 创建主窗口并注入依赖
    MainWindow window(&storage);
    window.setWindowTitle("ServoLink — 舵连设计台 v0.5.0");
    window.resize(1200, 750);
    window.show();

    return app.exec();
}
