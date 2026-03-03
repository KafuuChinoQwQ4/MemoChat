#include "httpmgr.h"
#include <QTimer>

namespace {
constexpr int kHttpTimeoutMs = 10000;
}

HttpMgr::~HttpMgr()
{

}

void HttpMgr::PostHttpReq(QUrl url, QJsonObject json, ReqId req_id, Modules mod)
{

    QByteArray data = QJsonDocument(json).toJson();

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(data.length()));
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    request.setTransferTimeout(kHttpTimeoutMs);
#endif

    auto self = shared_from_this();
    QNetworkReply * reply = _manager.post(request, data);

    QTimer *timeoutTimer = new QTimer(reply);
    timeoutTimer->setSingleShot(true);
    QObject::connect(timeoutTimer, &QTimer::timeout, reply, [reply]() {
        if (reply->isRunning()) {
            qWarning() << "HTTP request timeout, aborting:" << reply->url();
            reply->abort();
        }
    });
    timeoutTimer->start(kHttpTimeoutMs);


    QObject::connect(reply, &QNetworkReply::finished, [reply, self, req_id, mod, timeoutTimer](){
        timeoutTimer->stop();

        if(reply->error() != QNetworkReply::NoError){
            qDebug() << reply->errorString();

            emit self->sig_http_finish(req_id, "", ErrorCodes::ERR_NETWORK, mod);
            reply->deleteLater();
            return;
        }


        QString res = reply->readAll();


        emit self->sig_http_finish(req_id, res, ErrorCodes::SUCCESS,mod);
        reply->deleteLater();
        return;
    });
}

HttpMgr::HttpMgr()
{

    connect(this, &HttpMgr::sig_http_finish, this, &HttpMgr::slot_http_finish);
}

void HttpMgr::slot_http_finish(ReqId id, QString res, ErrorCodes err, Modules mod)
{
    if(mod == Modules::REGISTERMOD){

        emit sig_reg_mod_finish(id, res, err);
    }

    if(mod == Modules::RESETMOD){

        emit sig_reset_mod_finish(id, res, err);
    }

    if(mod == Modules::LOGINMOD){
        emit sig_login_mod_finish(id, res, err);
    }

    if (mod == Modules::SETTINGSMOD) {
        emit sig_settings_mod_finish(id, res, err);
    }
}
