#include "MomentsController.h"
#include "IconPathUtils.h"
#include "httpmgr.h"
#include "usermgr.h"

#include <QJsonObject>
#include <QtGlobal>

MomentsController::MomentsController(QObject* parent)
    : QObject(parent)
    , _model(new MomentsModel(this))
{
    auto httpMgr = HttpMgr::GetInstance();
    if (httpMgr)
    {
        QObject::connect(httpMgr.get(), &HttpMgr::sig_moments_mod_finish, this, &MomentsController::onLoadFeedRsp);
        QObject::connect(httpMgr.get(), &HttpMgr::sig_moments_mod_finish, this, &MomentsController::onPublishRsp);
        QObject::connect(httpMgr.get(), &HttpMgr::sig_moments_mod_finish, this, &MomentsController::onDeleteRsp);
        QObject::connect(httpMgr.get(), &HttpMgr::sig_moments_mod_finish, this, &MomentsController::onLikeRsp);
        QObject::connect(httpMgr.get(), &HttpMgr::sig_moments_mod_finish, this, &MomentsController::onCommentRsp);
        QObject::connect(httpMgr.get(), &HttpMgr::sig_moments_mod_finish, this, &MomentsController::onCommentLikeRsp);
        QObject::connect(httpMgr.get(), &HttpMgr::sig_moments_mod_finish, this, &MomentsController::onDetailRsp);
        QObject::connect(httpMgr.get(), &HttpMgr::sig_moments_mod_finish, this, &MomentsController::onCommentListRsp);
    }
}

MomentsController::~MomentsController()
{
}

void MomentsController::setLoading(bool v)
{
    if (_loading != v)
    {
        _loading = v;
        emit loadingChanged();
    }
}

void MomentsController::setHasMore(bool v)
{
    if (_has_more != v)
    {
        _has_more = v;
        emit hasMoreChanged();
    }
}

void MomentsController::setErrorText(const QString& text)
{
    if (_error_text != text)
    {
        _error_text = text;
        emit errorTextChanged();
    }
}

void MomentsController::setProgressText(const QString& text)
{
    if (_progress_text != text)
    {
        _progress_text = text;
        emit progressTextChanged();
    }
}

void MomentsController::setAuthorFilterUid(int uid)
{
    const int normalizedUid = qMax(0, uid);
    if (_author_filter_uid != normalizedUid)
    {
        _author_filter_uid = normalizedUid;
        emit authorFilterChanged();
    }
}

QString MomentsController::mediaUrlForKey(const QString& mediaKey) const
{
    const QString trimmed = mediaKey.trimmed();
    if (trimmed.isEmpty())
    {
        return QString();
    }
    return mediaKeyDownloadUrl(trimmed);
}
