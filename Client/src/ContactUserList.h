#pragma once
#include <QListWidget>
#include <QEvent>
#include <QWheelEvent>
#include <QScrollBar>
#include <memory>
#include "global.h"

class ConUserItem;

class ContactUserList : public QListWidget
{
    Q_OBJECT
public:
    ContactUserList(QWidget *parent = nullptr);
    void ShowRedPoint(bool bshow = true);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void addContactUserList();

public slots:
    void slot_item_clicked(QListWidgetItem *item);

signals:
    void sig_loading_contact_user();
    void sig_switch_apply_friend_page();
    void sig_switch_friend_info_page();

private:
    ConUserItem* _add_friend_item;
    QListWidgetItem * _groupitem;
};