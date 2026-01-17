#pragma once
#include "Singleton.h"
#include "global.h"
#include <QObject>
#include <QNetworkAccessManager>
#include <QJsonObject>
#include <QJsonDocument>

// CRTP (Curiously Recurring Template Pattern) 继承单例
class HttpMgr : public QObject, public Singleton<HttpMgr>, 
                public std::enable_shared_from_this<HttpMgr>
{
    Q_OBJECT

public:
    ~HttpMgr();
    
    // 发送 POST 请求的接口
    void PostHttpReq(QUrl url, QJsonObject json, ReqId req_id, Modules mod);

private:
    friend class Singleton<HttpMgr>;
    HttpMgr();

    QNetworkAccessManager *m_manager; // Qt6 建议用指针管理

signals:
    // HTTP 请求完成信号
    void sig_http_finish(ReqId id, QString res, ErrorCodes err, Modules mod);
    
    // 注册模块专用的完成信号 (文档里的设计)
    void sig_reg_mod_finish(ReqId id, QString res, ErrorCodes err);

private slots:
    // 内部槽：监听自己发出的信号，分发给不同模块
    void slot_http_finish(ReqId id, QString res, ErrorCodes err, Modules mod);
};