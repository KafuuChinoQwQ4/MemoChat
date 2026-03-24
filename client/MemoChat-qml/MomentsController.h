#ifndef MOMENTSCONTROLLER_H
#define MOMENTSCONTROLLER_H

#include <QObject>
#include <QString>
#include <QJsonArray>
#include <QVector>
#include <memory>
#include "MomentsModel.h"
#include "../MemoChatShared/httpmgr.h"
#include "../MemoChatShared/usermgr.h"

class MomentsController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(MomentsModel* model READ model CONSTANT)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(bool hasMore READ hasMore NOTIFY hasMoreChanged)
    Q_PROPERTY(QString errorText READ errorText NOTIFY errorTextChanged)

public:
    explicit MomentsController(QObject* parent = nullptr);
    ~MomentsController();

    MomentsModel* model() const { return _model; }
    bool loading() const { return _loading; }
    bool hasMore() const { return _has_more; }
    QString errorText() const { return _error_text; }

    Q_INVOKABLE void loadFeed();
    Q_INVOKABLE void loadMore();
    Q_INVOKABLE void publishMoment(const QString& location, int visibility,
                                   const QVariantList& items);
    Q_INVOKABLE void deleteMoment(qint64 momentId);
    Q_INVOKABLE void toggleLike(qint64 momentId);
    Q_INVOKABLE void addComment(qint64 momentId, const QString& content, int replyUid = 0);
    Q_INVOKABLE void deleteComment(qint64 momentId, qint64 commentId);
    Q_INVOKABLE void refreshMoment(qint64 momentId);

    static QVariantList buildTextItem(const QString& content);
    static QVariantList buildImageItem(const QString& mediaKey, const QString& thumbKey,
                                      int width, int height);
    static QVariantList buildVideoItem(const QString& mediaKey, const QString& thumbKey,
                                      int width, int height, int durationMs);

signals:
    void loadingChanged();
    void hasMoreChanged();
    void errorTextChanged();
    void publishSuccess(qint64 momentId);
    void publishError(const QString& msg);
    void likeToggled(qint64 momentId, bool liked, int likeCount);
    void commentAdded(qint64 momentId);

private slots:
    void onLoadFeedRsp(ReqId id, const QString& res, ErrorCodes err);
    void onPublishRsp(ReqId id, const QString& res, ErrorCodes err);
    void onDeleteRsp(ReqId id, const QString& res, ErrorCodes err);
    void onLikeRsp(ReqId id, const QString& res, ErrorCodes err);
    void onCommentRsp(ReqId id, const QString& res, ErrorCodes err);
    void onDetailRsp(ReqId id, const QString& res, ErrorCodes err);

private:
    void setLoading(bool v);
    void setHasMore(bool v);
    void setErrorText(const QString& text);
    QJsonObject buildAuthJson() const;
    MomentEntry parseMomentEntry(const QJsonObject& obj) const;

    MomentsModel* _model;
    bool _loading = false;
    bool _has_more = false;
    QString _error_text;
    qint64 _last_moment_id = 0;
    static constexpr int kPageSize = 20;
};

#endif  // MOMENTSCONTROLLER_H
