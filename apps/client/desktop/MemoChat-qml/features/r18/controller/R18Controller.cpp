#include "R18Controller.h"

#include "global.h"

#include <QJsonObject>
#include <QUrl>
#include <QUrlQuery>

R18Controller::R18Controller(ClientGateway* gateway, QObject* parent)
    : QObject(parent)
    , _gateway(gateway)
{
}

void R18Controller::refreshAccess()
{
    QUrl url(gate_url_prefix + QStringLiteral("/api/r18/access"));
    getJson(url, QStringLiteral("access"));
}

void R18Controller::attestAdult()
{
    postJson(QStringLiteral("/api/r18/access/attest"), QJsonObject{}, QStringLiteral("access_attest"));
}

void R18Controller::refreshSources()
{
    QUrl url(gate_url_prefix + QStringLiteral("/api/r18/sources"));
    getJson(url, QStringLiteral("sources"));
}

void R18Controller::refreshHistory()
{
    QUrl url(gate_url_prefix + QStringLiteral("/api/r18/history"));
    getJson(url, QStringLiteral("history"));
}

void R18Controller::selectSource(const QString& sourceId)
{
    const QString nextSource = sourceId.trimmed();
    if (_current_source_id == nextSource)
    {
        return;
    }
    _current_source_id = nextSource;
    _comics.clear();
    setSearchState(0, false);
    if (_current_source_id.isEmpty())
    {
        setStatusText(QStringLiteral("未选择漫画源"));
    }
    else
    {
        setStatusText(QStringLiteral("已选择漫画源: %1").arg(_current_source_id));
    }
    emit currentSourceChanged();
}

void R18Controller::search(const QString& keyword, int page)
{
    const int normalizedPage = page < 1 ? 1 : page;
    if (_current_source_id.trimmed().isEmpty())
    {
        if (normalizedPage == 1)
        {
            _comics.clear();
            setSearchState(0, false);
        }
        setError(QStringLiteral("请先导入或选择漫画源"));
        return;
    }

    QJsonObject payload;
    payload[QStringLiteral("source_id")] = _current_source_id;
    payload[QStringLiteral("keyword")] = keyword;
    payload[QStringLiteral("page")] = normalizedPage;
    _pending_search_page = normalizedPage;
    if (normalizedPage == 1)
    {
        _comics.clear();
        setSearchState(0, false);
    }
    postJson(QStringLiteral("/api/r18/search"), payload, QStringLiteral("search"));
}

void R18Controller::openComic(const QString& sourceId, const QString& comicId)
{
    const QString nextSource = sourceId.trimmed();
    if (_current_source_id != nextSource)
    {
        _current_source_id = nextSource;
        emit currentSourceChanged();
    }
    setCurrentFavorite(false);

    QJsonObject payload;
    payload[QStringLiteral("source_id")] = _current_source_id;
    payload[QStringLiteral("comic_id")] = comicId;
    postJson(QStringLiteral("/api/r18/comic/detail"), payload, QStringLiteral("detail"));
}

void R18Controller::openChapter(const QString& sourceId, const QString& chapterId)
{
    _current_chapter_id = chapterId;
    setCurrentPageIndex(1);
    QJsonObject payload;
    payload[QStringLiteral("source_id")] = sourceId.isEmpty() ? _current_source_id : sourceId;
    payload[QStringLiteral("chapter_id")] = chapterId;
    postJson(QStringLiteral("/api/r18/chapter/pages"), payload, QStringLiteral("pages"));
}

void R18Controller::toggleFavorite(const QString& sourceId, const QString& comicId, bool favorited)
{
    QJsonObject payload;
    payload[QStringLiteral("source_id")] = sourceId.isEmpty() ? _current_source_id : sourceId;
    payload[QStringLiteral("comic_id")] = comicId.isEmpty()
            ? _current_comic.value(QStringLiteral("comic_id")).toString() : comicId;
    payload[QStringLiteral("favorited")] = favorited;
    postJson(QStringLiteral("/api/r18/favorite/toggle"), payload, QStringLiteral("favorite"));
}

void R18Controller::updateHistory(const QString& sourceId,
                                  const QString& comicId,
                                  const QString& chapterId,
                                  int pageIndex)
{
    setCurrentPageIndex(pageIndex < 1 ? 1 : pageIndex);
    QJsonObject payload;
    payload[QStringLiteral("source_id")] = sourceId.isEmpty() ? _current_source_id : sourceId;
    payload[QStringLiteral("comic_id")] = comicId.isEmpty()
            ? _current_comic.value(QStringLiteral("comic_id")).toString() : comicId;
    payload[QStringLiteral("chapter_id")] = chapterId.isEmpty() ? _current_chapter_id : chapterId;
    payload[QStringLiteral("page_index")] = _current_page_index;
    postJson(QStringLiteral("/api/r18/history/update"), payload, QStringLiteral("history_update"));
}
