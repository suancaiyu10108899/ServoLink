#ifndef OPERATIONRESULT_H
#define OPERATIONRESULT_H

#include <QString>
#include <QList>

/**
 * @brief 操作结果
 *
 * 用于函数返回值，统一表达"成功/失败"语义。
 * 设计原则：
 * - 值类型，可自由拷贝
 * - 不抛异常，用 success/failure 区分
 * - 可携带多条消息（警告 + 错误）
 *
 * 与 MindPath 的 OperationResult 完全一致。
 */
class OperationResult
{
public:
    enum Level {
        Info    = 0,
        Warning = 1,
        Error   = 2
    };

    struct Message {
        Level level;
        QString text;

        Message(Level lvl, const QString &txt)
            : level(lvl), text(txt) {}
    };

    static OperationResult success()
    {
        return OperationResult(true);
    }

    static OperationResult failure(const QString &reason)
    {
        OperationResult r(false);
        r.addMessage(Error, reason);
        return r;
    }

    explicit OperationResult(bool ok)
        : m_success(ok) {}

    bool isSuccess() const { return m_success; }
    bool isFailure() const { return !m_success; }

    void addInfo(const QString &text)
    {
        m_messages.append(Message(Info, text));
    }

    void addWarning(const QString &text)
    {
        m_messages.append(Message(Warning, text));
    }

    void addError(const QString &text)
    {
        m_messages.append(Message(Error, text));
    }

    void addMessage(Level level, const QString &text)
    {
        m_messages.append(Message(level, text));
    }

    QList<Message> messages() const { return m_messages; }

    bool hasErrors() const
    {
        for (const auto &m : m_messages) {
            if (m.level == Error) return true;
        }
        return false;
    }

    bool hasWarnings() const
    {
        for (const auto &m : m_messages) {
            if (m.level == Warning) return true;
        }
        return false;
    }

    QString joinMessages(const QString &separator = "; ") const
    {
        QStringList parts;
        for (const auto &m : m_messages) {
            parts.append(m.text);
        }
        return parts.join(separator);
    }

private:
    bool m_success;
    QList<Message> m_messages;
};

#endif // OPERATIONRESULT_H
