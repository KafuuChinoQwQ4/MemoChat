#pragma once
#include "Singleton.h"
#include "global.h"
#include <QObject>
#include <QNetworkAccessManager>
#include <QJsonObject>
#include <QJsonDocument>

class HttpMgr : public QObject, public Singleton<HttpMgr>, 
                public std::enable_shared_from_this<HttpMgr>
{
    Q_OBJECT

public:
    ~HttpMgr();
    void PostHttpReq(QUrl url, QJsonObject json, ReqId req_id, Modules mod);

private:
    friend class Singleton<HttpMgr>;
    HttpMgr();

    QNetworkAccessManager *m_manager;

signals:
    void sig_http_finish(ReqId id, QString res, ErrorCodes err, Modules mod);
    void sig_reg_mod_finish(ReqId id, QString res, ErrorCodes err);
    void sig_reset_mod_finish(ReqId id, QString res, ErrorCodes err); // [新增]

private slots:
    void slot_http_finish(ReqId id, QString res, ErrorCodes err, Modules mod);
};