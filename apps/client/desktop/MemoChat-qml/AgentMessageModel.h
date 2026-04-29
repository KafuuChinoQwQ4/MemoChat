#ifndef AGENTMESSAGEMODEL_H
#define AGENTMESSAGEMODEL_H

#include <QAbstractListModel>
#include <QVector>
#include <QString>
#include <QDateTime>
#include <QTimer>
#include <QtGlobal>

class AgentMessageModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        MsgIdRole = Qt::UserRole + 1,
        ContentRole,
        RoleRole,
        CreatedAtRole,
        IsUserRole,
        IsAssistantRole,
        IsStreamingRole,
        StreamingContentRole,
        ThinkingContentRole,
        TokensUsedRole,
        ModelNameRole,
        ErrorMessageRole,
        SourcesRole,
    };

    explicit AgentMessageModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void clear();

    void appendUserMessage(const QString& content);
    void appendAIMessage(const QString& msgId, const QString& model);
    void updateStreamingContent(const QString& msgId, const QString& chunk);
    void finalizeAIMessage(const QString& msgId);
    void setError(const QString& msgId, const QString& error);
    void setSources(const QString& msgId, const QString& sourcesJson);

signals:
    void countChanged();

private:
    struct MessageEntry {
        QString msgId;
        QString content;
        QString streamingContent;
        QString thinkingContent;
        QString role;
        QString modelName;
        qint64 createdAt = 0;
        bool isStreaming = false;
        int tokensUsed = 0;
        QString errorMessage;
        QString sourcesJson;

        bool operator==(const MessageEntry& other) const = default;
    };

    int indexOfMessage(const QString& msgId) const;
    MessageEntry* findEntry(const QString& msgId);

    QVector<MessageEntry> _items;
};

#endif  // AGENTMESSAGEMODEL_H
