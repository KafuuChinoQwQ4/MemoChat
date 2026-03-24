#ifndef HTTPMGR_H
#define HTTPMGR_H
#include "singleton.h"
#include <QString>
#include <QUrl>
#include <QObject>
#include <QByteArray>
#include <QNetworkAccessManager>
#include "global.h"


class HttpMgr:public QObject, public Singleton<HttpMgr>,
        public std::enable_shared_from_this<HttpMgr>
{
    Q_OBJECT

public:
    ~HttpMgr();
    void PostHttpReq(QUrl url, QJsonObject json, ReqId req_id, Modules mod, const QString &module = QStringLiteral("http"));
private:
    friend class Singleton<HttpMgr>;
    HttpMgr();
    void postHttpReqInternal(const QUrl &url,
        const QByteArray &data,
        ReqId req_id,
        Modules mod,
        const QString &module,
        bool allowPlaintextFallback);
    QNetworkAccessManager _manager;
public slots:
    void slot_http_finish(ReqId id, QString res, ErrorCodes err, Modules mod);
signals:
    void sig_http_finish(ReqId id, QString res, ErrorCodes err, Modules mod);

    void sig_reg_mod_finish(ReqId id, QString res, ErrorCodes err);
   void sig_reset_mod_finish(ReqId id, QString res, ErrorCodes err);
    void sig_login_mod_finish(ReqId id, QString res, ErrorCodes err);
    void sig_settings_mod_finish(ReqId id, QString res, ErrorCodes err);
    void sig_call_mod_finish(ReqId id, QString res, ErrorCodes err);
    void sig_moments_mod_finish(ReqId id, QString res, ErrorCodes err);
};

#endif // HTTPMGR_H
