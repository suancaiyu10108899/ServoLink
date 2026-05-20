#ifndef PROJECTFILE_H
#define PROJECTFILE_H

#include <QString>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>
#include "LinkageParams.h"
#include "LinkageAnalysis.h"

/**
 * @brief 项目文件封装
 *
 * 一个项目包含：
 * - 机构参数
 * - 最近一次分析结果
 * - 元数据（名称、创建时间等）
 *
 * 这是"文档模型"（Document Model），用于 UI 层的打开/保存/另存为。
 */
struct ProjectFile
{
    QString id;                          // 唯一标识
    QString name;                        // 项目名称
    QDateTime createdAt;                 // 创建时间
    QDateTime lastModifiedAt;            // 最后修改时间
    LinkageParams params;                // 机构参数
    QString notes;                       // 备注（可选）

    ProjectFile()
        : id(QUuid::createUuid().toString(QUuid::WithoutBraces))
        , name("未命名项目")
        , createdAt(QDateTime::currentDateTime())
        , lastModifiedAt(QDateTime::currentDateTime())
    {
    }

    ProjectFile(const QString &projectName)
        : id(QUuid::createUuid().toString(QUuid::WithoutBraces))
        , name(projectName)
        , createdAt(QDateTime::currentDateTime())
        , lastModifiedAt(QDateTime::currentDateTime())
    {
    }

    // ── 序列化 ──
    QJsonObject toJson() const
    {
        QJsonObject obj;
        obj["id"] = id;
        obj["name"] = name;
        obj["createdAt"] = createdAt.toString(Qt::ISODate);
        obj["lastModifiedAt"] = lastModifiedAt.toString(Qt::ISODate);
        obj["params"] = params.toJson();
        if (!notes.isEmpty()) {
            obj["notes"] = notes;
        }
        return obj;
    }

    static ProjectFile fromJson(const QJsonObject &obj)
    {
        ProjectFile pf;
        pf.id = obj["id"].toString(pf.id);
        pf.name = obj["name"].toString("未命名项目");
        pf.createdAt = QDateTime::fromString(
            obj["createdAt"].toString(), Qt::ISODate);
        pf.lastModifiedAt = QDateTime::fromString(
            obj["lastModifiedAt"].toString(), Qt::ISODate);
        pf.params = LinkageParams::fromJson(obj["params"].toObject());
        pf.notes = obj["notes"].toString();
        return pf;
    }

    void touch()
    {
        lastModifiedAt = QDateTime::currentDateTime();
    }
};

#endif // PROJECTFILE_H
