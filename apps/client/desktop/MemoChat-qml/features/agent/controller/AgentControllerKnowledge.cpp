#include "AgentController.h"
#include "LocalFilePickerService.h"
#include "global.h"
#include "httpmgr.h"
#include "usermgr.h"
#include "ClientGateway.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QUrlQuery>

namespace
{
QString aiHttpModule()
{
    return QStringLiteral("ai");
}
} // namespace

void AgentController::setKnowledgeBusy(bool busy, const QString& statusText)
{
    bool changed = _knowledge_busy != busy || _knowledge_status_text != statusText;
    _knowledge_busy = busy;
    _knowledge_status_text = statusText;
    if (changed)
    {
        emit knowledgeStateChanged();
    }
}

void AgentController::setKnowledgeError(const QString& error)
{
    if (_knowledge_error == error)
    {
        return;
    }
    _knowledge_error = error;
    emit knowledgeStateChanged();
}

void AgentController::clearKnowledgeError()
{
    if (_knowledge_error.isEmpty())
    {
        return;
    }
    _knowledge_error.clear();
    emit knowledgeStateChanged();
}

void AgentController::uploadDocument(const QString& filePath)
{
    auto uid = _gateway->userMgr()->GetUid();
    clearErrorState();
    clearKnowledgeError();
    setKnowledgeBusy(true, "正在准备上传文档...");

    QString localPath = filePath;
    const QUrl maybeUrl(filePath);
    if (maybeUrl.isLocalFile())
    {
        localPath = maybeUrl.toLocalFile();
    }

    QFile file(localPath);
    if (!file.open(QIODevice::ReadOnly))
    {
        const QString errorText = QString("无法打开文件: %1").arg(localPath);
        setKnowledgeBusy(false);
        setKnowledgeError(errorText);
        setErrorState(errorText);
        return;
    }
    QByteArray fileData = file.readAll();
    file.close();

    QFileInfo info(localPath);
    QString fileName = info.fileName();
    QString suffix = info.suffix().toLower();

    QString fileType;
    if (suffix == "pdf")
        fileType = "pdf";
    else if (suffix == "txt")
        fileType = "txt";
    else if (suffix == "md" || suffix == "markdown")
        fileType = "md";
    else if (suffix == "docx")
        fileType = "docx";
    else
    {
        const QString errorText = QString("不支持的文件类型: %1").arg(suffix);
        setKnowledgeBusy(false);
        setKnowledgeError(errorText);
        setErrorState(errorText);
        return;
    }

    QJsonObject payload;
    payload["uid"] = uid;
    payload["file_name"] = fileName;
    payload["file_type"] = fileType;
    payload["content"] = QString::fromLatin1(fileData.toBase64());

    emit kbUploadProgress(0);
    setKnowledgeBusy(true, QString("正在上传 %1...").arg(fileName));

    ReqId reqId = ID_AI_KB_UPLOAD;
    _pending_requests.track(reqId, AgentRequestKind::KnowledgeUpload);
    HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix + "/ai/kb/upload"),
                                        payload,
                                        reqId,
                                        Modules::LOGINMOD,
                                        aiHttpModule());
}

void AgentController::chooseAndUploadDocument()
{
    QString fileUrl;
    QString displayName;
    QString errorText;
    if (!LocalFilePickerService::pickFileUrl(&fileUrl, &displayName, &errorText))
    {
        if (!errorText.trimmed().isEmpty())
        {
            setKnowledgeBusy(false);
            setKnowledgeError(errorText);
            setErrorState(errorText);
        }
        return;
    }
    uploadDocument(fileUrl);
}

void AgentController::searchKnowledgeBase(const QString& query)
{
    auto uid = _gateway->userMgr()->GetUid();
    clearErrorState();
    clearKnowledgeError();
    setKnowledgeBusy(true, "正在检索知识库...");
    _knowledge_search_result.clear();
    emit knowledgeSearchResultChanged();

    QJsonObject payload;
    payload["uid"] = uid;
    payload["query"] = query;
    payload["top_k"] = 5;

    ReqId reqId = ID_AI_KB_SEARCH;
    _pending_requests.track(reqId, AgentRequestKind::KnowledgeSearch);
    HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix + "/ai/kb/search"),
                                        payload,
                                        reqId,
                                        Modules::LOGINMOD,
                                        aiHttpModule());
}

void AgentController::listKnowledgeBases()
{
    auto uid = _gateway->userMgr()->GetUid();
    clearErrorState();
    clearKnowledgeError();
    setKnowledgeBusy(true, "正在加载知识库...");
    QUrl url(gate_url_prefix + "/ai/kb/list");
    QUrlQuery query;
    query.addQueryItem("uid", QString::number(uid));
    url.setQuery(query);

    ReqId reqId = ID_AI_KB_LIST;
    _pending_requests.track(reqId, AgentRequestKind::KnowledgeList);

    HttpMgr::GetInstance()->GetHttpReq(url, reqId, Modules::LOGINMOD, aiHttpModule());
}

void AgentController::deleteKnowledgeBase(const QString& kbId)
{
    auto uid = _gateway->userMgr()->GetUid();
    clearErrorState();
    clearKnowledgeError();
    setKnowledgeBusy(true, "正在删除知识库...");

    QJsonObject payload;
    payload["uid"] = uid;
    payload["kb_id"] = kbId;

    ReqId reqId = ID_AI_KB_DELETE;
    _pending_requests.track(reqId, AgentRequestKind::KnowledgeDelete);
    HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix + "/ai/kb/delete"),
                                        payload,
                                        reqId,
                                        Modules::LOGINMOD,
                                        aiHttpModule());
}

void AgentController::handleKbRsp(ReqId id, const QString& res, ErrorCodes err, AgentRequestKind kind)
{
    Q_UNUSED(id);
    Q_UNUSED(err);
    QJsonDocument doc = QJsonDocument::fromJson(res.toUtf8());
    QJsonObject root = doc.object();

    if (kind == AgentRequestKind::KnowledgeUpload)
    {
        Q_UNUSED(root["chunks"].toInt());
        emit kbUploadProgress(100);
        clearErrorState();
        setKnowledgeBusy(false, "文档上传完成，正在刷新知识库...");
        clearKnowledgeError();
        emit knowledgeBasesChanged();
        listKnowledgeBases();
    }
    else if (kind == AgentRequestKind::KnowledgeList)
    {
        QJsonArray bases = root["knowledge_bases"].toArray();
        _knowledge_bases.clear();
        for (const auto& base : bases)
        {
            _knowledge_bases.append(base.toObject());
        }
        clearErrorState();
        setKnowledgeBusy(false,
                         _knowledge_bases.isEmpty() ? "当前还没有上传文档。"
                                                    : QString("已加载 %1 个知识库条目。").arg(_knowledge_bases.size()));
        clearKnowledgeError();
        emit knowledgeBasesChanged();
    }
    else if (kind == AgentRequestKind::KnowledgeSearch)
    {
        QJsonArray chunks = root["chunks"].toArray();
        QString summary;
        for (const auto& c : chunks)
        {
            QJsonObject chunk = c.toObject();
            summary += chunk["content"].toString() + "\n\n";
        }
        _knowledge_search_result = summary.trimmed();
        clearErrorState();
        setKnowledgeBusy(false,
                         _knowledge_search_result.isEmpty() ? "没有找到相关知识片段。"
                                                            : QString("已返回 %1 条候选结果。").arg(chunks.size()));
        clearKnowledgeError();
        emit knowledgeSearchResultChanged();
        emit aiResponseReceived(summary);
    }
    else if (kind == AgentRequestKind::KnowledgeDelete)
    {
        clearErrorState();
        setKnowledgeBusy(false, "知识库已删除，正在刷新列表...");
        clearKnowledgeError();
        emit knowledgeBasesChanged();
        listKnowledgeBases();
    }
}
