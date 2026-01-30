#pragma once
#include <QDialog>
#include <memory>
#include "global.h"

class ClickedBtn;
class QLabel;

class FindSuccessDlg : public QDialog
{
    Q_OBJECT

public:
    explicit FindSuccessDlg(QWidget *parent = nullptr);
    ~FindSuccessDlg();
    void SetSearchInfo(std::shared_ptr<SearchInfo> si);

private slots:
    void on_add_friend_btn_clicked();

private:
    void initUI(); // 手动初始化UI

    QLabel * _head_lb;
    QLabel * _name_lb;
    ClickedBtn * _add_friend_btn; // 使用自定义按钮
    
    std::shared_ptr<SearchInfo> _si;
};