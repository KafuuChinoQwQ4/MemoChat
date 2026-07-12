#include "R18Controller.h"

#include <QTimer>

void R18Controller::setOfficialSourceCatalogUrl(const QString& url)
{
    if (_official_source_catalog_url == url)
        return;
    _official_source_catalog_url = url;
    emit officialSourceCatalogUrlChanged();
}

void R18Controller::setLoading(bool loading)
{
    if (_loading == loading)
        return;
    _loading = loading;
    emit loadingChanged();
}

void R18Controller::setError(const QString& error)
{
    if (_error == error)
        return;
    _error = error;
    emit errorChanged();
}

void R18Controller::setStatusText(const QString& statusText)
{
    if (_status_text == statusText)
        return;
    _status_text = statusText;
    emit statusTextChanged();
    if (_status_text.isEmpty())
    {
        return;
    }
    QTimer::singleShot(2500,
                       this,
                       [this, statusText]()
                       {
                           if (_error.isEmpty() && _status_text == statusText)
                           {
                               _status_text.clear();
                               emit statusTextChanged();
                           }
                       });
}

void R18Controller::setCurrentFavorite(bool favorite)
{
    if (_current_favorite == favorite)
        return;
    _current_favorite = favorite;
    emit currentFavoriteChanged();
}

void R18Controller::setCurrentPageIndex(int pageIndex)
{
    const int normalized = pageIndex < 1 ? 1 : pageIndex;
    if (_current_page_index == normalized)
        return;
    _current_page_index = normalized;
    emit currentPageChanged();
}

void R18Controller::setSearchState(int page, bool hasMore)
{
    const int normalizedPage = page < 0 ? 0 : page;
    if (_current_search_page == normalizedPage && _current_search_has_more == hasMore)
    {
        return;
    }
    _current_search_page = normalizedPage;
    _current_search_has_more = hasMore;
    emit searchStateChanged();
}

void R18Controller::setAccessState(bool resolved, bool allowed, bool revoked)
{
    if (_access_resolved == resolved && _access_allowed == allowed && _access_revoked == revoked)
    {
        return;
    }
    _access_resolved = resolved;
    _access_allowed = allowed;
    _access_revoked = revoked;
    if (!allowed)
    {
        _sources.clear();
        _comics.clear();
        _chapters.clear();
        _pages.clear();
        _history.clear();
    }
    emit accessChanged();
}
