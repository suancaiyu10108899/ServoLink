#ifndef ISTORAGEBACKEND_H
#define ISTORAGEBACKEND_H

#include <QJsonObject>
#include <optional>
#include "OperationResult.h"

/**
 * @brief 存储后端接口
 *
 * 纯虚类，定义持久化的合同。
 * 实现类可以是 JSON 文件、SQLite、或其他后端。
 * 当前默认使用 JSON 文件存储（对齐 MindPath）。
 */
class IStorageBackend
{
public:
    virtual ~IStorageBackend() = default;

    /**
     * @brief 保存完整项目快照
     *
     * 快照格式 (v1)：
     * {
     *   "version": 1,
     *   "appName": "ServoLink",
     *   "savedAt": "2026-05-20T12:00:00",
     *   "currentParams": { ... LinkageParams ... },
     *   "projects": [
     *     {
     *       "name": "...",
     *       "params": { ... },
     *       "analysis": { ... }
     *     }
     *   ]
     * }
     */
    virtual OperationResult saveAll(const QJsonObject &snapshot) = 0;
    virtual std::optional<QJsonObject> loadAll() = 0;
};

#endif // ISTORAGEBACKEND_H
