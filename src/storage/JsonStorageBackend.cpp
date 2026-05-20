#include "JsonStorageBackend.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QDebug>

JsonStorageBackend::JsonStorageBackend(const QString &filePath)
    : m_filePath(filePath)
{
}

bool JsonStorageBackend::fileExists() const
{
    return QFile::exists(m_filePath);
}

OperationResult JsonStorageBackend::saveAll(const QJsonObject &snapshot)
{
    // 确保目录存在
    QFileInfo fi(m_filePath);
    QDir dir = fi.absoluteDir();
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            return OperationResult::failure(
                QString("无法创建存储目录: %1").arg(dir.absolutePath()));
        }
    }

    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return OperationResult::failure(
            QString("无法写入文件: %1 (%2)").arg(m_filePath, file.errorString()));
    }

    QJsonDocument doc(snapshot);
    qint64 bytesWritten = file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    if (bytesWritten <= 0) {
        return OperationResult::failure("写入文件时发生错误");
    }

    qDebug() << "[JsonStorageBackend] Saved" << bytesWritten
             << "bytes to" << m_filePath;

    OperationResult result = OperationResult::success();
    result.addInfo(QString("已保存至 %1 (%2 bytes)").arg(m_filePath).arg(bytesWritten));
    return result;
}

std::optional<QJsonObject> JsonStorageBackend::loadAll()
{
    if (!fileExists()) {
        qDebug() << "[JsonStorageBackend] File not found:" << m_filePath;
        return std::nullopt;
    }

    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[JsonStorageBackend] Cannot open file:" << m_filePath
                    << file.errorString();
        return std::nullopt;
    }

    QByteArray data = file.readAll();
    file.close();

    if (data.isEmpty()) {
        qWarning() << "[JsonStorageBackend] File is empty:" << m_filePath;
        return std::nullopt;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "[JsonStorageBackend] JSON parse error:"
                    << parseError.errorString();
        return std::nullopt;
    }

    if (!doc.isObject()) {
        qWarning() << "[JsonStorageBackend] JSON root is not an object";
        return std::nullopt;
    }

    qDebug() << "[JsonStorageBackend] Loaded" << data.size()
             << "bytes from" << m_filePath;

    return doc.object();
}
