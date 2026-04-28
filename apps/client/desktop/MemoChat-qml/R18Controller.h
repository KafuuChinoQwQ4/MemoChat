#pragma once

#include "R18ListModel.h"

#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QVariantMap>

class ClientGateway;

class R18Controller : public QObject
{
    Q_OBJECT
    Q_PROPERTY(R18ListModel* sourceModel READ sourceModel CONSTANT)
    Q_PROPERTY(R18ListModel* comicModel READ comicModel CONSTANT)
    Q_PROPERTY(R18ListModel* chapterModel READ chapterModel CONSTANT)
    Q_PROPERTY(R18ListModel* pageModel READ pageModel CONSTANT)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString currentSourceId READ currentSourceId NOTIFY currentSourceChanged)
    Q_PROPERTY(QVariantMap currentComic READ currentComic NOTIFY currentComicChanged)

public:
    explicit R18Controller(ClientGateway* gateway, QObject* parent = nullptr);

    R18ListModel* sourceModel() { return &_sources; }
    R18ListModel* comicModel() { return &_comics; }
    R18ListModel* chapterModel() { return &_chapters; }
    R18ListModel* pageModel() { return &_pages; }
    bool loading() const { return _loading; }
    QString error() const { return _error; }
    QString currentSourceId() const { return _current_source_id; }
    QVariantMap currentComic() const { return _current_comic; }

    Q_INVOKABLE void refreshSources();
    Q_INVOKABLE void search(const QString& keyword, int page = 1);
    Q_INVOKABLE void openComic(const QString& sourceId, const QString& comicId);
    Q_INVOKABLE void openChapter(const QString& sourceId, const QString& chapterId);
    Q_INVOKABLE void enableSource(const QString& sourceId, bool enabled);
    Q_INVOKABLE void importSourcePackage(const QString& filePath, const QString& manifestJson = QString());

signals:
    void loadingChanged();
    void errorChanged();
    void currentSourceChanged();
    void currentComicChanged();

private:
    void postJson(const QString& path, const QJsonObject& payload, const QString& op);
    void getJson(const QUrl& url, const QString& op);
    QJsonObject authPayload() const;
    void setLoading(bool loading);
    void setError(const QString& error);
    void handleResponse(const QString& op, const QJsonObject& root);

    ClientGateway* _gateway = nullptr;
    QNetworkAccessManager _network;
    R18ListModel _sources;
    R18ListModel _comics;
    R18ListModel _chapters;
    R18ListModel _pages;
    bool _loading = false;
    QString _error;
    QString _current_source_id = QStringLiteral("mock");
    QVariantMap _current_comic;
};
