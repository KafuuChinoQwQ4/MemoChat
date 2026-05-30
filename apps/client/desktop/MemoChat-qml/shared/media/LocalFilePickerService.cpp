#include "LocalFilePickerService.h"

#include <QDesktopServices>
#include <QUrl>

bool LocalFilePickerService::openUrl(const QString& urlText, QString* errorText)
{
    const QUrl directUrl(urlText);
    QUrl targetUrl = directUrl;
    if (!directUrl.isValid() || directUrl.scheme().isEmpty())
    {
        targetUrl = QUrl::fromLocalFile(urlText);
    }

    if (!targetUrl.isValid())
    {
        if (errorText)
        {
            *errorText = "无效的资源地址";
        }
        return false;
    }

    if (!QDesktopServices::openUrl(targetUrl))
    {
        if (errorText)
        {
            *errorText = "打开资源失败";
        }
        return false;
    }
    return true;
}
