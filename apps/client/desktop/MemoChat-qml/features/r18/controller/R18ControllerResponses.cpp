#include "R18Controller.h"

#include <QJsonArray>
#include <QJsonObject>

void R18Controller::handleResponse(const QString& op, const QJsonObject& root)
{
    if (root.value(QStringLiteral("error")).toInt() != 0)
    {
        const QString message = root.value(QStringLiteral("message")).toString(QStringLiteral("R18 请求失败"));
        setError(message);
        return;
    }

    const auto data = root.value(QStringLiteral("data")).toObject();
    if (op == QStringLiteral("access") || op == QStringLiteral("access_attest"))
    {
        const bool allowed = data.value(QStringLiteral("allowed")).toBool(false);
        const bool revoked = data.value(QStringLiteral("state")).toString() == QStringLiteral("revoked");
        setAccessState(true, allowed, revoked);
        if (allowed)
        {
            refreshSources();
            refreshHistory();
        }
    }
    else if (op == QStringLiteral("sources"))
    {
        _sources.setItems(data.value(QStringLiteral("sources")).toArray().toVariantList());
    }
    else if (op == QStringLiteral("search"))
    {
        const int page = _pending_search_page < 1 ? 1 : _pending_search_page;
        const auto items = data.value(QStringLiteral("items")).toArray().toVariantList();
        if (page <= 1)
        {
            _comics.setItems(items);
        }
        else
        {
            _comics.appendItems(items);
        }
        const int maxPage = data.value(QStringLiteral("max_page")).toInt();
        const bool hasMore = maxPage > 0 ? page < maxPage : items.size() >= 40;
        setSearchState(page, hasMore);
    }
    else if (op == QStringLiteral("history"))
    {
        _history.setItems(data.value(QStringLiteral("items")).toArray().toVariantList());
    }
    else if (op == QStringLiteral("detail"))
    {
        _current_comic = data.toVariantMap();
        emit currentComicChanged();
        _chapters.setItems(data.value(QStringLiteral("chapters")).toArray().toVariantList());
    }
    else if (op == QStringLiteral("pages"))
    {
        _pages.setItems(data.value(QStringLiteral("pages")).toArray().toVariantList());
        updateHistory(
            _current_source_id,
            _current_comic.value(QStringLiteral("comic_id")).toString(), _current_chapter_id, _current_page_index);
    }
    else if (op == QStringLiteral("favorite"))
    {
        setCurrentFavorite(data.value(QStringLiteral("favorited")).toBool());
    }
}
