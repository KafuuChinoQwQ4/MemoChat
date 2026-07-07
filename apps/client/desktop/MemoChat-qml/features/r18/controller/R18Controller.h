#pragma once

#include "R18ListModel.h"

#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QUrl>
#include <QVariantMap>

class ClientGateway;
class QNetworkRequest;

class R18Controller : public QObject
{
    Q_OBJECT
    Q_PROPERTY(R18ListModel* sourceModel READ sourceModel CONSTANT)
    Q_PROPERTY(R18ListModel* comicModel READ comicModel CONSTANT)
    Q_PROPERTY(R18ListModel* chapterModel READ chapterModel CONSTANT)
    Q_PROPERTY(R18ListModel* pageModel READ pageModel CONSTANT)
    Q_PROPERTY(R18ListModel* historyModel READ historyModel CONSTANT)
    Q_PROPERTY(R18ListModel* officialSourceModel READ officialSourceModel CONSTANT)
    Q_PROPERTY(QString officialSourceCatalogUrl READ officialSourceCatalogUrl WRITE setOfficialSourceCatalogUrl NOTIFY
                   officialSourceCatalogUrlChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString currentSourceId READ currentSourceId NOTIFY currentSourceChanged)
    Q_PROPERTY(QVariantMap currentComic READ currentComic NOTIFY currentComicChanged)
    Q_PROPERTY(bool currentFavorite READ currentFavorite NOTIFY currentFavoriteChanged)
    Q_PROPERTY(int currentPageIndex READ currentPageIndex NOTIFY currentPageChanged)
    Q_PROPERTY(int currentSearchPage READ currentSearchPage NOTIFY searchStateChanged)
    Q_PROPERTY(bool currentSearchHasMore READ currentSearchHasMore NOTIFY searchStateChanged)
    Q_PROPERTY(QString pendingDeleteSourceId READ pendingDeleteSourceId NOTIFY pendingDeleteSourceChanged)

public:
    explicit R18Controller(ClientGateway* gateway, QObject* parent = nullptr);

    R18ListModel* sourceModel()
    {
        return &_sources;
    }
    R18ListModel* comicModel()
    {
        return &_comics;
    }
    R18ListModel* chapterModel()
    {
        return &_chapters;
    }
    R18ListModel* pageModel()
    {
        return &_pages;
    }
    R18ListModel* historyModel()
    {
        return &_history;
    }
    R18ListModel* officialSourceModel()
    {
        return &_official_sources;
    }
    QString officialSourceCatalogUrl() const
    {
        return _official_source_catalog_url;
    }
    QString statusText() const
    {
        return _status_text;
    }
    bool loading() const
    {
        return _loading;
    }
    QString error() const
    {
        return _error;
    }
    QString currentSourceId() const
    {
        return _current_source_id;
    }
    QVariantMap currentComic() const
    {
        return _current_comic;
    }
    bool currentFavorite() const
    {
        return _current_favorite;
    }
    int currentPageIndex() const
    {
        return _current_page_index;
    }
    int currentSearchPage() const
    {
        return _current_search_page;
    }
    bool currentSearchHasMore() const
    {
        return _current_search_has_more;
    }
    QString pendingDeleteSourceId() const
    {
        return _pending_delete_source_id;
    }

    Q_INVOKABLE void refreshSources();
    Q_INVOKABLE void refreshHistory();
    Q_INVOKABLE void refreshOfficialSources(const QString& catalogUrl = QString());
    Q_INVOKABLE void importOfficialSource(int row);
    Q_INVOKABLE void importSourceUrl(const QString& sourceUrl);
    Q_INVOKABLE QString pickSourcePackage();
    Q_INVOKABLE QString pickSourceCatalogPath();
    Q_INVOKABLE void selectSource(const QString& sourceId);
    Q_INVOKABLE void search(const QString& keyword, int page = 1);
    Q_INVOKABLE void openComic(const QString& sourceId, const QString& comicId);
    Q_INVOKABLE void openChapter(const QString& sourceId, const QString& chapterId);
    Q_INVOKABLE void enableSource(const QString& sourceId, bool enabled);
    Q_INVOKABLE void deleteSource(const QString& sourceId);
    Q_INVOKABLE void importSourcePackage(const QString& filePath, const QString& manifestJson = QString());
    Q_INVOKABLE void toggleFavorite(const QString& sourceId, const QString& comicId, bool favorited);
    Q_INVOKABLE void
    updateHistory(const QString& sourceId, const QString& comicId, const QString& chapterId, int pageIndex);

signals:
    void loadingChanged();
    void errorChanged();
    void statusTextChanged();
    void currentSourceChanged();
    void currentComicChanged();
    void currentFavoriteChanged();
    void currentPageChanged();
    void officialSourceCatalogUrlChanged();
    void searchStateChanged();
    void pendingDeleteSourceChanged();

private:
    void postJson(const QString& path, const QJsonObject& payload, const QString& op);
    void getJson(const QUrl& url, const QString& op);
    void applyAuthHeader(QNetworkRequest& request) const;
    void setOfficialSourceCatalogUrl(const QString& url);
    QJsonObject authPayload() const;
    QJsonObject officialSourceManifest(const QVariantMap& item, const QUrl& scriptUrl) const;
    QJsonObject sourceUrlManifest(const QUrl& scriptUrl) const;
    QUrl resolveOfficialSourceUrl(const QVariantMap& item) const;
    void downloadAndImportSource(const QUrl& scriptUrl, const QVariantMap& item);
    void setLoading(bool loading);
    void setError(const QString& error);
    void setStatusText(const QString& statusText);
    void setCurrentFavorite(bool favorite);
    void setCurrentPageIndex(int pageIndex);
    void setSearchState(int page, bool hasMore);
    void setPendingDeleteSourceId(const QString& sourceId);
    void handleResponse(const QString& op, const QJsonObject& root);

    ClientGateway* _gateway = nullptr;
    QNetworkAccessManager _network;
    R18ListModel _sources;
    R18ListModel _comics;
    R18ListModel _chapters;
    R18ListModel _pages;
    R18ListModel _history;
    R18ListModel _official_sources;
    QString _official_source_catalog_url;
    QString _status_text;
    bool _loading = false;
    QString _error;
    QString _current_source_id;
    QString _current_chapter_id;
    QVariantMap _current_comic;
    bool _current_favorite = false;
    int _current_page_index = 1;
    int _current_search_page = 0;
    bool _current_search_has_more = false;
    int _pending_search_page = 0;
    QString _pending_delete_source_id;
};
