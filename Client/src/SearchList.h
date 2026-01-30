#pragma once
#include <QListWidget>
#include <QEvent>
#include <QWheelEvent>
#include <QScrollBar>
#include <memory>
#include "global.h"

class LoadingDlg; // 前向声明

class SearchList: public QListWidget
{
    Q_OBJECT
public:
    SearchList(QWidget *parent = nullptr);
    void CloseFindDlg();
    void SetSearchEdit(QWidget* edit);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void waitPending(bool pending = true);
    void addTipItem();
    
    bool _send_pending;
    QWidget* _search_edit;
    LoadingDlg * _loadingDialog;

private slots:
    void slot_item_clicked(QListWidgetItem *item);
    void slot_user_search(std::shared_ptr<SearchInfo> si);
};