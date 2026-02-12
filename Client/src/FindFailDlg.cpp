#include "FindFailDlg.h"
#include <QVBoxLayout>
#include <QLabel>
#include "ClickedBtn.h"
#include <QDebug>

FindFailDlg::FindFailDlg(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("搜索失败");
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    setModal(true);
    initUI();
    
    if(_fail_sure_btn) _fail_sure_btn->SetState("normal","hover","press");
}

FindFailDlg::~FindFailDlg() {
    qDebug() << "FindFailDlg destruct";
}

void FindFailDlg::initUI() {
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    
    QWidget *bgWidget = new QWidget(this);
    bgWidget->setObjectName("find_fail_bg");
    bgWidget->setFixedSize(280, 200);

    QVBoxLayout *bgLayout = new QVBoxLayout(bgWidget);
    bgLayout->setContentsMargins(20, 20, 20, 20);

    _fail_tip_lb = new QLabel(bgWidget);
    _fail_tip_lb->setText("未找到该用户");
    _fail_tip_lb->setAlignment(Qt::AlignCenter);
    _fail_tip_lb->setObjectName("fail_tip");
    QFont font;
    font.setPointSize(14);
    _fail_tip_lb->setFont(font);
    
    _fail_sure_btn = new ClickedBtn(bgWidget);
    _fail_sure_btn->setFixedSize(100, 30);
    _fail_sure_btn->setText("确定");
    _fail_sure_btn->setObjectName("fail_sure_btn");

    bgLayout->addStretch();
    bgLayout->addWidget(_fail_tip_lb, 0, Qt::AlignHCenter);
    bgLayout->addSpacing(20);
    bgLayout->addWidget(_fail_sure_btn, 0, Qt::AlignHCenter);
    bgLayout->addStretch();

    layout->addWidget(bgWidget);

    connect(_fail_sure_btn, &ClickedBtn::clicked, this, &FindFailDlg::on_fail_sure_btn_clicked);
}

void FindFailDlg::on_fail_sure_btn_clicked()
{
    this->close(); // 使用 close() 会触发 deleteOnClose 如果设置了的话，或者只是隐藏
}