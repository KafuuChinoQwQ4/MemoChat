#include "HttpMgr.h"
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QDebug>

HttpMgr::HttpMgr() {
    m_manager = new QNetworkAccessManager(this);

    // 连接内部信号流转
    connect(this, &HttpMgr::sig_http_finish, this, &HttpMgr::slot_http_finish);
}

HttpMgr::~HttpMgr() {
    // QObject 树机制会自动删除 m_manager
}

void HttpMgr::PostHttpReq(QUrl url, QJsonObject json, ReqId req_id, Modules mod) {
    // 1. 构造数据
    QByteArray data = QJsonDocument(json).toJson();
    
    // 2. 构造请求
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(data.length()));

    // 3. 发送请求
    // 获取 shared_from_this 防止在回调执行时对象已被销毁
    auto self = shared_from_this();
    QNetworkReply *reply = m_manager->post(request, data);

    // 4. 异步处理回包
    connect(reply, &QNetworkReply::finished, [reply, self, req_id, mod]() {
        // 错误处理
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "HTTP Error:" << reply->errorString();
            emit self->sig_http_finish(req_id, "", ErrorCodes::ERR_NETWORK, mod);
            reply->deleteLater(); // 务必清理内存
            return;
        }

        // 成功读取
        QString res = reply->readAll();
        emit self->sig_http_finish(req_id, res, ErrorCodes::SUCCESS, mod);
        reply->deleteLater();
    });
}

void HttpMgr::slot_http_finish(ReqId id, QString res, ErrorCodes err, Modules mod) {
    if (mod == Modules::REGISTERMOD) {
        emit sig_reg_mod_finish(id, res, err);
    }
    else if (mod == Modules::RESETMOD) {
        emit sig_reset_mod_finish(id, res, err);
    }
    else if (mod == Modules::LOGINMOD) { // [新增]
        emit sig_login_mod_finish(id, res, err);
    }
}