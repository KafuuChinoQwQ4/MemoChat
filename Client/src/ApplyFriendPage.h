#pragma once
#include <QWidget>
#include <unordered_map>
#include <memory>
#include "userdata.h"

class ApplyFriendList;
class ApplyFriendItem;

class ApplyFriendPage : public QWidget
{
    Q_OBJECT

public:
    explicit ApplyFriendPage(QWidget *parent = nullptr);
    ~ApplyFriendPage();
    void AddNewApply(std::shared_ptr<AddFriendApply> apply);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void initUI();
    void loadApplyList();
    
    ApplyFriendList *_apply_friend_list;
    std::unordered_map<int, ApplyFriendItem*> _unauth_items;

public slots:
    void slot_auth_rsp(std::shared_ptr<AuthRsp> );

signals:
    void sig_show_search(bool);
};