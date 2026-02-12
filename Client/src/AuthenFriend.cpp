#include "AuthenFriend.h"
#include "CustomizeEdit.h"
#include "ClickedLabel.h"
#include "FriendLabel.h"
#include "ClickedOnceLabel.h"
#include "ClickedBtn.h"
#include "UserMgr.h"
#include "TcpMgr.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>

AuthenFriend::AuthenFriend(QWidget *parent) :
    QDialog(parent), _label_point(2,6)
{
    // 隐藏对话框标题栏
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    this->setObjectName("AuthenFriend");
    this->setModal(true);

    initUI();

    _lb_ed->setPlaceholderText("搜索、添加标签");
    _back_ed->setPlaceholderText("燃烧的胸毛");
    _name_ed->setPlaceholderText("申请人");
    _name_ed->setReadOnly(true);

    _lb_ed->SetMaxLength(21);
    _lb_ed->move(2, 2);
    _lb_ed->setFixedHeight(20);
    _lb_ed->setMaxLength(10);
    _input_tip_wid->hide();

    _tip_cur_point = QPoint(5, 5);

    _tip_data = { "同学","家人","菜鸟教程","C++ Primer","Rust 程序设计",
                             "父与子学Python","nodejs开发指南","go 语言开发指南",
                                "游戏伙伴","金融投资","微信读书","拼多多拼友" };

    connect(_more_lb, &ClickedOnceLabel::clicked, this, &AuthenFriend::ShowMoreLabel);
    InitTipLbs();
    
    //链接输入标签回车事件
    connect(_lb_ed, &CustomizeEdit::returnPressed, this, &AuthenFriend::SlotLabelEnter);
    connect(_lb_ed, &CustomizeEdit::textChanged, this, &AuthenFriend::SlotLabelTextChange);
    connect(_lb_ed, &CustomizeEdit::editingFinished, this, &AuthenFriend::SlotLabelEditFinished);
    connect(_tip_lb, &ClickedOnceLabel::clicked, this, &AuthenFriend::SlotAddFirendLabelByClickTip);

    _scrollArea->horizontalScrollBar()->setHidden(true);
    _scrollArea->verticalScrollBar()->setHidden(true);
    _scrollArea->installEventFilter(this);
    
    _sure_btn->SetState("normal","hover","press");
    _cancel_btn->SetState("normal","hover","press");
    
    //连接确认和取消按钮的槽函数
    connect(_cancel_btn, &QPushButton::clicked, this, &AuthenFriend::SlotApplyCancel);
    connect(_sure_btn, &QPushButton::clicked, this, &AuthenFriend::SlotApplySure);
}

AuthenFriend::~AuthenFriend()
{
    qDebug()<< "AuthenFriend destruct";
}

void AuthenFriend::initUI() {
    this->setFixedSize(600, 450);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->setSpacing(0);

    // 顶部区域 Widget
    QWidget *apply_wid = new QWidget(this);
    apply_wid->setObjectName("apply_wid");
    QVBoxLayout *applyLayout = new QVBoxLayout(apply_wid);
    applyLayout->setContentsMargins(20,20,20,20);
    applyLayout->setSpacing(10);

    // Row 1: 申请人
    QWidget *row1 = new QWidget(apply_wid);
    QHBoxLayout *row1Layout = new QHBoxLayout(row1);
    row1Layout->setContentsMargins(0,0,0,0);
    QLabel *l1 = new QLabel("申请人:", row1);
    l1->setFixedWidth(60);
    _name_ed = new CustomizeEdit(row1);
    _name_ed->setObjectName("name_ed");
    _name_ed->setFixedHeight(30);
    row1Layout->addWidget(l1);
    row1Layout->addWidget(_name_ed);

    // Row 1: 备注
    QWidget *row2 = new QWidget(apply_wid);
    QHBoxLayout *row2Layout = new QHBoxLayout(row2);
    row2Layout->setContentsMargins(0,0,0,0);
    QLabel *l2 = new QLabel("备注:", row2);
    l2->setFixedWidth(60);
    _back_ed = new CustomizeEdit(row2);
    _back_ed->setObjectName("back_ed");
    row2Layout->addWidget(l2);
    row2Layout->addWidget(_back_ed);

    // Row 2: 标签输入区
    QWidget *row3 = new QWidget(apply_wid);
    QHBoxLayout *row3Layout = new QHBoxLayout(row3);
    row3Layout->setContentsMargins(0,0,0,0);
    QLabel *l3 = new QLabel("标签:", row3);
    l3->setFixedWidth(60);
    l3->setAlignment(Qt::AlignTop);

    _gridWidget = new QWidget(row3);
    _gridWidget->setObjectName("gridWidget");
    _gridWidget->setMinimumHeight(50);
    
    _lb_ed = new CustomizeEdit(_gridWidget);
    _lb_ed->setObjectName("lb_ed");
    _lb_ed->move(2, 2);

    row3Layout->addWidget(l3);
    row3Layout->addWidget(_gridWidget);

    // Row 3: 标签展示列表
    QWidget *row4 = new QWidget(apply_wid);
    QVBoxLayout *row4Layout = new QVBoxLayout(row4);
    row4Layout->setContentsMargins(0,10,0,0);
    
    QWidget *headerWid = new QWidget(row4);
    QHBoxLayout *hl = new QHBoxLayout(headerWid);
    hl->setContentsMargins(0,0,0,0);
    QLabel *tagTitle = new QLabel("添加标签", headerWid);
    _more_lb_wid = new QWidget(headerWid);
    QHBoxLayout *moreLayout = new QHBoxLayout(_more_lb_wid);
    moreLayout->setContentsMargins(0,0,0,0);
    _more_lb = new ClickedOnceLabel(_more_lb_wid);
    _more_lb->setObjectName("more_lb");
    _more_lb->setFixedSize(20,20);
    moreLayout->addStretch();
    moreLayout->addWidget(_more_lb);
    
    hl->addWidget(tagTitle);
    hl->addWidget(_more_lb_wid);

    _scrollArea = new QScrollArea(row4);
    _scrollArea->setObjectName("scrollArea");
    _scrollArea->setWidgetResizable(true);
    _scrollcontent = new QWidget();
    _scrollcontent->setObjectName("scrollcontent");
    _lb_list = new QWidget(_scrollcontent);
    _lb_list->setObjectName("lb_list");
    _lb_list->setGeometry(0, 0, 500, 1000);
    _scrollArea->setWidget(_scrollcontent);
    
    // 自动补全提示框
    _input_tip_wid = new QWidget(apply_wid);
    _input_tip_wid->setObjectName("input_tip_wid");
    _input_tip_wid->hide();
    
    QHBoxLayout *tipLayout = new QHBoxLayout(_input_tip_wid);
    tipLayout->setContentsMargins(5,0,0,0);
    _tip_lb = new ClickedOnceLabel(_input_tip_wid);
    _tip_lb->setObjectName("tip_lb");
    tipLayout->addWidget(_tip_lb);

    row4Layout->addWidget(headerWid);
    row4Layout->addWidget(_scrollArea);

    applyLayout->addWidget(row1);
    applyLayout->addWidget(row2);
    applyLayout->addWidget(row3);
    applyLayout->addWidget(row4);
    
    // 底部按钮区域
    QWidget *sure_wid = new QWidget(this);
    sure_wid->setObjectName("apply_sure_wid");
    QHBoxLayout *btnLayout = new QHBoxLayout(sure_wid);
    btnLayout->setContentsMargins(20, 10, 20, 10);
    btnLayout->addStretch();
    
    _sure_btn = new ClickedBtn(sure_wid);
    _sure_btn->setObjectName("sure_btn");
    _sure_btn->setText("确认");
    _sure_btn->setFixedSize(80, 30);
    
    _cancel_btn = new ClickedBtn(sure_wid);
    _cancel_btn->setObjectName("cancel_btn");
    _cancel_btn->setText("取消");
    _cancel_btn->setFixedSize(80, 30);
    
    btnLayout->addWidget(_sure_btn);
    btnLayout->addSpacing(20);
    btnLayout->addWidget(_cancel_btn);

    mainLayout->addWidget(apply_wid);
    mainLayout->addWidget(sure_wid);
}

void AuthenFriend::InitTipLbs()
{
    int lines = 1;
    for(int i = 0; i < _tip_data.size(); i++){
        auto* lb = new ClickedLabel(_lb_list);
        lb->SetState("normal", "hover", "pressed", "selected_normal",
            "selected_hover", "selected_pressed");
        lb->setObjectName("tipslb");
        lb->setText(_tip_data[i]);
        connect(lb, &ClickedLabel::clicked, this, &AuthenFriend::SlotChangeFriendLabelByTip);

        QFontMetrics fontMetrics(lb->font());
        int textWidth = fontMetrics.horizontalAdvance(lb->text());
        int textHeight = fontMetrics.height();

        if (_tip_cur_point.x() + textWidth + tip_offset > _lb_list->width()) {
            lines++;
            if (lines > 2) {
                delete lb;
                return;
            }
            _tip_cur_point.setX(tip_offset);
            _tip_cur_point.setY(_tip_cur_point.y() + textHeight + 15);
        }

       auto next_point = _tip_cur_point;
       AddTipLbs(lb, _tip_cur_point,next_point, textWidth, textHeight);
       _tip_cur_point = next_point;
    }
}

void AuthenFriend::AddTipLbs(ClickedLabel* lb, QPoint cur_point, QPoint& next_point, int text_width, int text_height)
{
    lb->move(cur_point);
    lb->show();
    _add_labels.insert(lb->text(), lb);
    _add_label_keys.push_back(lb->text());
    next_point.setX(lb->pos().x() + text_width + 15);
    next_point.setY(lb->pos().y());
}

bool AuthenFriend::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == _scrollArea && event->type() == QEvent::Enter)
    {
        _scrollArea->verticalScrollBar()->setHidden(false);
    }
    else if (obj == _scrollArea && event->type() == QEvent::Leave)
    {
        _scrollArea->verticalScrollBar()->setHidden(true);
    }
    return QObject::eventFilter(obj, event);
}

void AuthenFriend::SetApplyInfo(std::shared_ptr<ApplyInfo> apply_info)
{
    _apply_info = apply_info;
    _name_ed->setText(apply_info->_name);
    _back_ed->setPlaceholderText(apply_info->_name);
}

void AuthenFriend::ShowMoreLabel()
{
    qDebug()<< "receive more label clicked";
    _more_lb_wid->hide();

    _lb_list->setFixedWidth(325);
    _tip_cur_point = QPoint(5, 5);
    auto next_point = _tip_cur_point;
    int textWidth;
    int textHeight;
    //重排现有的label
    for(auto & added_key : _add_label_keys){
        auto added_lb = _add_labels[added_key];

        QFontMetrics fontMetrics(added_lb->font());
        textWidth = fontMetrics.horizontalAdvance(added_lb->text());
        textHeight = fontMetrics.height();

        if(_tip_cur_point.x() +textWidth + tip_offset > _lb_list->width()){
            _tip_cur_point.setX(tip_offset);
            _tip_cur_point.setY(_tip_cur_point.y()+textHeight+15);
        }
        added_lb->move(_tip_cur_point);

        next_point.setX(added_lb->pos().x() + textWidth + 15);
        next_point.setY(_tip_cur_point.y());

        _tip_cur_point = next_point;
    }

    //添加未添加的
    for(int i = 0; i < _tip_data.size(); i++){
        auto iter = _add_labels.find(_tip_data[i]);
        if(iter != _add_labels.end()){
            continue;
        }

        auto* lb = new ClickedLabel(_lb_list);
        lb->SetState("normal", "hover", "pressed", "selected_normal",
            "selected_hover", "selected_pressed");
        lb->setObjectName("tipslb");
        lb->setText(_tip_data[i]);
        connect(lb, &ClickedLabel::clicked, this, &AuthenFriend::SlotChangeFriendLabelByTip);

        QFontMetrics fontMetrics(lb->font());
        textWidth = fontMetrics.horizontalAdvance(lb->text());
        textHeight = fontMetrics.height();

        if (_tip_cur_point.x() + textWidth + tip_offset > _lb_list->width()) {
            _tip_cur_point.setX(tip_offset);
            _tip_cur_point.setY(_tip_cur_point.y() + textHeight + 15);
        }

         next_point = _tip_cur_point;
        AddTipLbs(lb, _tip_cur_point, next_point, textWidth, textHeight);
        _tip_cur_point = next_point;
    }

   int diff_height = next_point.y() + textHeight + tip_offset - _lb_list->height();
   _lb_list->setFixedHeight(next_point.y() + textHeight + tip_offset);
   _scrollcontent->setMinimumHeight(_lb_list->height());
}

void AuthenFriend::resetLabels()
{
    auto max_width = _gridWidget->width();
    auto label_height = 0;
    for(auto iter = _friend_labels.begin(); iter != _friend_labels.end(); iter++){
        if( _label_point.x() + iter.value()->width() > max_width) {
            _label_point.setY(_label_point.y()+iter.value()->height()+6);
            _label_point.setX(2);
        }

        iter.value()->move(_label_point);
        iter.value()->show();

        _label_point.setX(_label_point.x()+iter.value()->width()+2);
        _label_point.setY(_label_point.y());
        label_height = iter.value()->height();
    }

    if(_friend_labels.isEmpty()){
         _lb_ed->move(_label_point);
         return;
    }

    if(_label_point.x() + MIN_APPLY_LABEL_ED_LEN > _gridWidget->width()){
        _lb_ed->move(2,_label_point.y()+label_height+6);
    }else{
         _lb_ed->move(_label_point);
    }
}

void AuthenFriend::addLabel(QString name)
{
    if (_friend_labels.find(name) != _friend_labels.end()) {
        return;
    }

    auto tmplabel = new FriendLabel(_gridWidget);
    tmplabel->SetText(name);
    tmplabel->setObjectName("FriendLabel");
    tmplabel->show();
    tmplabel->adjustSize();

    auto max_width = _gridWidget->width();
    if (_label_point.x() + tmplabel->width() > max_width) {
        _label_point.setY(_label_point.y() + tmplabel->height() + 6);
        _label_point.setX(2);
    }

    tmplabel->move(_label_point);
    _friend_labels[tmplabel->Text()] = tmplabel;
    _friend_label_keys.push_back(tmplabel->Text());

    connect(tmplabel, &FriendLabel::sig_close, this, &AuthenFriend::SlotRemoveFriendLabel);

    _label_point.setX(_label_point.x() + tmplabel->width() + 2);

    if (_label_point.x() + MIN_APPLY_LABEL_ED_LEN > _gridWidget->width()) {
        _lb_ed->move(2, _label_point.y() + tmplabel->height() + 2);
    }
    else {
        _lb_ed->move(_label_point);
    }

    _lb_ed->clear();

    if (_gridWidget->height() < _label_point.y() + tmplabel->height() + 2) {
        _gridWidget->setFixedHeight(_label_point.y() + tmplabel->height() * 2 + 2);
    }
}

void AuthenFriend::SlotLabelEnter()
{
    if(_lb_ed->text().isEmpty()){
        return;
    }
    addLabel(_lb_ed->text());
    _input_tip_wid->hide();
}

void AuthenFriend::SlotRemoveFriendLabel(QString name)
{
    _label_point.setX(2);
    _label_point.setY(6);

   auto find_iter = _friend_labels.find(name);
   if(find_iter == _friend_labels.end()){
       return;
   }

   auto find_key = _friend_label_keys.end();
   for(auto iter = _friend_label_keys.begin(); iter != _friend_label_keys.end(); iter++){
       if(*iter == name){
           find_key = iter;
           break;
       }
   }
   if(find_key != _friend_label_keys.end()){
      _friend_label_keys.erase(find_key);
   }

   delete find_iter.value();
   _friend_labels.erase(find_iter);
   resetLabels();

   auto find_add = _add_labels.find(name);
   if(find_add == _add_labels.end()){
        return;
   }
   find_add.value()->ResetNormalState();
}

void AuthenFriend::SlotChangeFriendLabelByTip(QString lbtext, ClickLbState state)
{
    auto find_iter = _add_labels.find(lbtext);
    if(find_iter == _add_labels.end()){
        return;
    }
    if(state == ClickLbState::Selected){
        addLabel(lbtext);
        return;
    }
    if(state == ClickLbState::Normal){
        SlotRemoveFriendLabel(lbtext);
        return;
    }
}

void AuthenFriend::SlotLabelTextChange(const QString& text)
{
    if (text.isEmpty()) {
        _tip_lb->setText("");
        _input_tip_wid->hide();
        return;
    }

    auto iter = std::find(_tip_data.begin(), _tip_data.end(), text);
    if (iter == _tip_data.end()) {
        auto new_text = add_prefix + text;
        _tip_lb->setText(new_text);
    } else {
        _tip_lb->setText(text);
    }
    
    QPoint point = _lb_ed->pos(); 
    QPoint globalPoint = _gridWidget->mapTo(this, point);
    _input_tip_wid->move(globalPoint.x(), globalPoint.y() + _lb_ed->height() + 2);
    _input_tip_wid->raise(); 
    _input_tip_wid->show();
}

void AuthenFriend::SlotLabelEditFinished()
{
    _input_tip_wid->hide();
}

void AuthenFriend::SlotAddFirendLabelByClickTip(QString text)
{
    int index = text.indexOf(add_prefix);
    if (index != -1) {
        text = text.mid(index + add_prefix.length());
    }
    addLabel(text);
    if (index != -1) {
        _tip_data.push_back(text);
    }

    auto* lb = new ClickedLabel(_lb_list);
    lb->SetState("normal", "hover", "pressed", "selected_normal",
        "selected_hover", "selected_pressed");
    lb->setObjectName("tipslb");
    lb->setText(text);
    connect(lb, &ClickedLabel::clicked, this, &AuthenFriend::SlotChangeFriendLabelByTip);

    QFontMetrics fontMetrics(lb->font());
    int textWidth = fontMetrics.horizontalAdvance(lb->text());
    int textHeight = fontMetrics.height();

    if (_tip_cur_point.x() + textWidth+ tip_offset+3 > _lb_list->width()) {
        _tip_cur_point.setX(5);
        _tip_cur_point.setY(_tip_cur_point.y() + textHeight + 15);
    }

    auto next_point = _tip_cur_point;
    AddTipLbs(lb, _tip_cur_point, next_point, textWidth,textHeight);
    _tip_cur_point = next_point;

    int diff_height = next_point.y() + textHeight + tip_offset - _lb_list->height();
    _lb_list->setFixedHeight(next_point.y() + textHeight + tip_offset);
    lb->SetCurState(ClickLbState::Selected);
    _scrollcontent->setMinimumHeight(_lb_list->height());
}

void AuthenFriend::SlotApplySure()
{
    qDebug() << "Slot Apply Sure ";
    QJsonObject jsonObj;
    auto uid = UserMgr::GetInstance()->GetUid();
    jsonObj["fromuid"] = uid;
    jsonObj["touid"] = _apply_info->_uid;
    QString back_name = "";
    if(_back_ed->text().isEmpty()){
        back_name = _back_ed->placeholderText();
    }else{
        back_name = _back_ed->text();
    }
    jsonObj["back"] = back_name;

    QJsonDocument doc(jsonObj);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    emit TcpMgr::GetInstance()->sig_send_data(ReqId::ID_AUTH_FRIEND_REQ, jsonData);

    this->hide();
    deleteLater();
}

void AuthenFriend::SlotApplyCancel()
{
    this->hide();
    deleteLater();
}