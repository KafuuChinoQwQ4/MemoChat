#pragma once
#include "ListItemBase.h"
#include "userdata.h"
#include <memory>

class QLabel;
class ClickedBtn; // 使用之前的 ClickedBtn

class ApplyFriendItem : public ListItemBase
{
    Q_OBJECT

public:
    explicit ApplyFriendItem(QWidget *parent = nullptr);
    ~ApplyFriendItem();
    void SetInfo(std::shared_ptr<ApplyInfo> apply_info);
    void ShowAddBtn(bool bshow);
    QSize sizeHint() const override;
    int GetUid();

private:
    void initUI();
    
    QLabel *_icon_lb;
    QLabel *_user_name_lb;
    QLabel *_user_chat_lb; // 申请信息
    ClickedBtn *_addBtn;
    QLabel *_already_add_lb;
    
    std::shared_ptr<ApplyInfo> _apply_info;
    bool _added;

signals:
    void sig_auth_friend(std::shared_ptr<ApplyInfo> apply_info);
};