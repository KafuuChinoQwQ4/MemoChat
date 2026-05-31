#include "R18Controller.h"

#include <QJsonArray>
#include <QJsonObject>

void R18Controller::handleResponse(const QString& op, const QJsonObject& root)
{
    if (root.value(QStringLiteral("error")).toInt() != 0)
    {
        setError(root.value(QStringLiteral("message")).toString(QStringLiteral("R18 请求失败")));
        return;
    }

    const auto data = root.value(QStringLiteral("data")).toObject();
    if (op == QStringLiteral("sources"))
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
    else if (op == QStringLiteral("source_state") || op == QStringLiteral("import"))
    {
        const auto source = data.value(QStringLiteral("source")).toObject();
        const QString sourceId = source.value(QStringLiteral("id")).toString();
        if (!sourceId.isEmpty())
        {
            _current_source_id = sourceId;
            emit currentSourceChanged();
            if (op == QStringLiteral("import"))
            {
                setStatusText(
                    QStringLiteral("已导入漫画源: %1").arg(source.value(QStringLiteral("name")).toString(sourceId)));
            }
        }
        else if (op == QStringLiteral("source_state"))
        {
            setStatusText(QStringLiteral("漫画源状态已更新"));
        }
        refreshSources();
    }
}
