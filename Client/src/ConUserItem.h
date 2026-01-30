#pragma once
#include "ListItemBase.h"
#include "userdata.h"
#include <memory>

class QLabel;

class ConUserItem : public ListItemBase
{
    Q_OBJECT

public:
    explicit ConUserItem(QWidget *parent = nullptr);
    ~ConUserItem();
    QSize sizeHint() const override;
    
    // 适配文档的接口
    void SetInfo(std::shared_ptr<AuthInfo> auth_info);
    void SetInfo(std::shared_ptr<AuthRsp> auth_rsp);
    void SetInfo(int uid, QString name, QString icon);
    void ShowRedPoint(bool show = false);

private:
    void initUI();
    
    QLabel *_icon_lb;
    QLabel *_user_name_lb;
    QLabel *_red_point;
    
    std::shared_ptr<UserInfo> _info;
};