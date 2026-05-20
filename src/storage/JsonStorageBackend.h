#ifndef JSONSTORAGEBACKEND_H
#define JSONSTORAGEBACKEND_H

#include <QString>
#include "IStorageBackend.h"

/**
 * @brief JSON 文件存储后端
 *
 * 实现 IStorageBackend 接口，将项目快照读写为 JSON 文件。
 *
 * 存储路径：%LOCALAPPDATA%\ServoLink\ServoLink\servolink.json
 * （遵循 MindPath ADR 0003 策略：使用 AppLocalDataLocation）
 */
class JsonStorageBackend : public IStorageBackend
{
public:
    explicit JsonStorageBackend(const QString &filePath);
    ~JsonStorageBackend() override = default;

    OperationResult saveAll(const QJsonObject &snapshot) override;
    std::optional<QJsonObject> loadAll() override;

    QString filePath() const { return m_filePath; }

    /**
     * @brief 检查文件是否存在
     */
    bool fileExists() const;

private:
    QString m_filePath;
};

#endif // JSONSTORAGEBACKEND_H
