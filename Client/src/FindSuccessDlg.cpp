#include "FindSuccessDlg.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QCoreApplication>
#include <QDir>
#include <QPixmap>
#include <QDebug>
#include "ClickedBtn.h"

FindSuccessDlg::FindSuccessDlg(QWidget *parent) : QDialog(parent)
{
    // 设置对话框属性
    setWindowTitle("添加");
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    setModal(true);

    initUI();

    // 加载头像逻辑 (尽可能复原文档逻辑)
    QString app_path = QCoreApplication::applicationDirPath();
    QString pix_path = QDir::toNativeSeparators(app_path +
                             QDir::separator() + "static"+QDir::separator()+"head_1.jpg");
    
    QPixmap head_pix(pix_path);
    // 如果本地并没有static文件夹，为了防止头像空白，我们做一个fallback，用资源里的图片代替
    if(head_pix.isNull()){
        head_pix.load(":/res/head_1.jpg"); // 假设资源里有这个
        if(head_pix.isNull()) head_pix.load(":/res/KafuuChino.png"); // 再不行就用智乃
    }

    if(!head_pix.isNull()){
        head_pix = head_pix.scaled(_head_lb->size(),
                Qt::KeepAspectRatio, Qt::SmoothTransformation);
        _head_lb->setPixmap(head_pix);
    }

    _add_friend_btn->SetState("normal","hover","press");
}

FindSuccessDlg::~FindSuccessDlg()
{
    qDebug()<<"FindSuccessDlg destruct";
}

void FindSuccessDlg::initUI() {
    // 设置主布局
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);
    
    // 这里的布局只是简单的垂直排列，为了美观，我们加一个容器 Widget
    QWidget *bgWidget = new QWidget(this);
    bgWidget->setObjectName("find_success_bg"); // 用于 QSS
    QVBoxLayout *bgLayout = new QVBoxLayout(bgWidget);
    bgLayout->setContentsMargins(20, 20, 20, 20);

    // 头像
    _head_lb = new QLabel(bgWidget);
    _head_lb->setFixedSize(60, 60); // 假设大小
    _head_lb->setAlignment(Qt::AlignCenter);
    
    // 名字
    _name_lb = new QLabel(bgWidget);
    _name_lb->setAlignment(Qt::AlignCenter);
    _name_lb->setObjectName("find_user_name");
    
    // 添加按钮
    _add_friend_btn = new ClickedBtn(bgWidget);
    _add_friend_btn->setFixedSize(100, 30);
    _add_friend_btn->setText("添加好友"); // 为了能看清文字
    _add_friend_btn->setObjectName("find_add_btn");

    // 布局组装
    bgLayout->addWidget(_head_lb, 0, Qt::AlignHCenter);
    bgLayout->addWidget(_name_lb, 0, Qt::AlignHCenter);
    bgLayout->addSpacing(10);
    bgLayout->addWidget(_add_friend_btn, 0, Qt::AlignHCenter);
    bgLayout->addStretch();

    layout->addWidget(bgWidget);

    // 连接信号
    connect(_add_friend_btn, &ClickedBtn::clicked, this, &FindSuccessDlg::on_add_friend_btn_clicked);
}

void FindSuccessDlg::SetSearchInfo(std::shared_ptr<SearchInfo> si)
{
    _name_lb->setText(si->_name);
    _si = si;
}

void FindSuccessDlg::on_add_friend_btn_clicked()
{
    //todo... 添加好友界面弹出
    qDebug() << "Add friend clicked!";
    this->accept(); // 暂时先关闭窗口
}