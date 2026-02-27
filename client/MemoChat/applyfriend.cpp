#include "applyfriend.h"
#include "ui_applyfriend.h"
#include "clickedlabel.h"
#include "friendlabel.h"
#include <QScrollBar>
#include "usermgr.h"
#include "tcpmgr.h"

ApplyFriend::ApplyFriend(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ApplyFriend),_label_point(2,6)
{
    ui->setupUi(this);
    // 闅愯棌瀵硅瘽妗嗘爣棰樻爮
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    this->setObjectName("ApplyFriend");
    this->setModal(true);
    ui->name_ed->setPlaceholderText(tr("鎭嬫亱椋庤景"));
    ui->lb_ed->setPlaceholderText("Search or add labels");
    ui->back_ed->setPlaceholderText("Remark");

	ui->lb_ed->SetMaxLength(21);
	ui->lb_ed->move(2, 2);
	ui->lb_ed->setFixedHeight(20);
	ui->lb_ed->setMaxLength(10);
	ui->input_tip_wid->hide();

    _tip_cur_point = QPoint(5, 5);

    _tip_data = {
        "Classmates", "Family", "Guide", "C++ Primer", "Rust",
        "Python", "Node.js", "Go", "Game", "Investment",
        "WeChatRead", "Shopping"
    };

    connect(ui->more_lb, &ClickedOnceLabel::clicked, this, &ApplyFriend::ShowMoreLabel);
    InitTipLbs();
    //閾炬帴杈撳叆鏍囩鍥炶溅浜嬩欢
    connect(ui->lb_ed, &CustomizeEdit::returnPressed, this, &ApplyFriend::SlotLabelEnter);
    connect(ui->lb_ed, &CustomizeEdit::textChanged, this, &ApplyFriend::SlotLabelTextChange);
    connect(ui->lb_ed, &CustomizeEdit::editingFinished, this, &ApplyFriend::SlotLabelEditFinished);
    connect(ui->tip_lb, &ClickedOnceLabel::clicked, this, &ApplyFriend::SlotAddFirendLabelByClickTip);

    ui->scrollArea->horizontalScrollBar()->setHidden(true);
    ui->scrollArea->verticalScrollBar()->setHidden(true);
    ui->scrollArea->installEventFilter(this);
    ui->sure_btn->SetState("normal","hover","press");
    ui->cancel_btn->SetState("normal","hover","press");
    connect(ui->cancel_btn, &QPushButton::clicked, this, &ApplyFriend::SlotApplyCancel);
    connect(ui->sure_btn, &QPushButton::clicked, this, &ApplyFriend::SlotApplySure);
}

ApplyFriend::~ApplyFriend()
{
    qDebug()<< "ApplyFriend destruct";
    delete ui;
}

void ApplyFriend::InitTipLbs()
{
    int lines = 1;
    for(int i = 0; i < _tip_data.size(); i++){

		auto* lb = new ClickedLabel(ui->lb_list);
		lb->SetState("normal", "hover", "pressed", "selected_normal",
			"selected_hover", "selected_pressed");
		lb->setObjectName("tipslb");
		lb->setText(_tip_data[i]);
		connect(lb, &ClickedLabel::clicked, this, &ApplyFriend::SlotChangeFriendLabelByTip);

        QFontMetrics fontMetrics(lb->font());
        int textWidth = fontMetrics.horizontalAdvance(lb->text());
        int textHeight = fontMetrics.height();
		if (_tip_cur_point.x() + textWidth + tip_offset > ui->lb_list->width()) {
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


void ApplyFriend::AddTipLbs(ClickedLabel* lb, QPoint cur_point, QPoint& next_point, int text_width, int text_height)
{
	lb->move(cur_point);
	lb->show();
	_add_labels.insert(lb->text(), lb);
	_add_label_keys.push_back(lb->text());
	next_point.setX(lb->pos().x() + text_width + 15);
    next_point.setY(lb->pos().y());
}

bool ApplyFriend::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->scrollArea && event->type() == QEvent::Enter)
    {
        ui->scrollArea->verticalScrollBar()->setHidden(false);
    }
    else if (obj == ui->scrollArea && event->type() == QEvent::Leave)
    {
        ui->scrollArea->verticalScrollBar()->setHidden(true);
    }
    return QObject::eventFilter(obj, event);
}

void ApplyFriend::SetSearchInfo(std::shared_ptr<SearchInfo> si)
{
    _si = si;
    auto applyname = UserMgr::GetInstance()->GetName();
    auto bakname = si->_name;
    ui->name_ed->setText(applyname);
    ui->back_ed->setText(bakname);
}

void ApplyFriend::ShowMoreLabel()
{
    qDebug()<< "receive more label clicked";
    ui->more_lb_wid->hide();

    ui->lb_list->setFixedWidth(325);
    _tip_cur_point = QPoint(5, 5);
    auto next_point = _tip_cur_point;
    int textWidth;
    int textHeight;
    //閲嶆媿鐜版湁鐨刲abel
    for(auto & added_key : _add_label_keys){
        auto added_lb = _add_labels[added_key];

        QFontMetrics fontMetrics(added_lb->font());
        textWidth = fontMetrics.horizontalAdvance(added_lb->text());
        textHeight = fontMetrics.height();
        if(_tip_cur_point.x() +textWidth + tip_offset > ui->lb_list->width()){
            _tip_cur_point.setX(tip_offset);
            _tip_cur_point.setY(_tip_cur_point.y()+textHeight+15);
        }
        added_lb->move(_tip_cur_point);

        next_point.setX(added_lb->pos().x() + textWidth + 15);
        next_point.setY(_tip_cur_point.y());

        _tip_cur_point = next_point;

    }

    //娣诲姞鏈坊鍔犵殑
    for(int i = 0; i < _tip_data.size(); i++){
        auto iter = _add_labels.find(_tip_data[i]);
        if(iter != _add_labels.end()){
            continue;
        }

		auto* lb = new ClickedLabel(ui->lb_list);
		lb->SetState("normal", "hover", "pressed", "selected_normal",
			"selected_hover", "selected_pressed");
		lb->setObjectName("tipslb");
		lb->setText(_tip_data[i]);
		connect(lb, &ClickedLabel::clicked, this, &ApplyFriend::SlotChangeFriendLabelByTip);

        QFontMetrics fontMetrics(lb->font());
        int textWidth = fontMetrics.horizontalAdvance(lb->text());
        int textHeight = fontMetrics.height();
		if (_tip_cur_point.x() + textWidth + tip_offset > ui->lb_list->width()) {

			_tip_cur_point.setX(tip_offset);
			_tip_cur_point.setY(_tip_cur_point.y() + textHeight + 15);

		}

		 next_point = _tip_cur_point;

		AddTipLbs(lb, _tip_cur_point, next_point, textWidth, textHeight);

		_tip_cur_point = next_point;

    }

   int diff_height = next_point.y() + textHeight + tip_offset - ui->lb_list->height();
   ui->lb_list->setFixedHeight(next_point.y() + textHeight + tip_offset);

    //qDebug()<<"after resize ui->lb_list size is " <<  ui->lb_list->size();
    ui->scrollcontent->setFixedHeight(ui->scrollcontent->height()+diff_height);
}

void ApplyFriend::resetLabels()
{
    auto max_width = ui->gridWidget->width();
    auto label_height = 0;
    for(auto iter = _friend_labels.begin(); iter != _friend_labels.end(); iter++){
        //todo... 娣诲姞瀹藉害缁熻
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
         ui->lb_ed->move(_label_point);
         return;
    }

    if(_label_point.x() + MIN_APPLY_LABEL_ED_LEN > ui->gridWidget->width()){
        ui->lb_ed->move(2,_label_point.y()+label_height+6);
    }else{
         ui->lb_ed->move(_label_point);
    }
}

void ApplyFriend::addLabel(QString name)
{
    if (_friend_labels.find(name) != _friend_labels.end()) {
        ui->lb_ed->clear();
        return;
    }

	auto tmplabel = new FriendLabel(ui->gridWidget);
	tmplabel->SetText(name);
	tmplabel->setObjectName("FriendLabel");

	auto max_width = ui->gridWidget->width();
	//todo... 娣诲姞瀹藉害缁熻
	if (_label_point.x() + tmplabel->width() > max_width) {
		_label_point.setY(_label_point.y() + tmplabel->height() + 6);
		_label_point.setX(2);
	}
	else {

	}


	tmplabel->move(_label_point);
	tmplabel->show();
	_friend_labels[tmplabel->Text()] = tmplabel;
	_friend_label_keys.push_back(tmplabel->Text());

	connect(tmplabel, &FriendLabel::sig_close, this, &ApplyFriend::SlotRemoveFriendLabel);

	_label_point.setX(_label_point.x() + tmplabel->width() + 2);

	if (_label_point.x() + MIN_APPLY_LABEL_ED_LEN > ui->gridWidget->width()) {
		ui->lb_ed->move(2, _label_point.y() + tmplabel->height() + 2);
	}
	else {
		ui->lb_ed->move(_label_point);
	}

	ui->lb_ed->clear();

	if (ui->gridWidget->height() < _label_point.y() + tmplabel->height() + 2) {
		ui->gridWidget->setFixedHeight(_label_point.y() + tmplabel->height() * 2 + 2);
	}
}

void ApplyFriend::SlotLabelEnter()
{
    if(ui->lb_ed->text().isEmpty()){
        return;
    }

    auto text = ui->lb_ed->text();

    addLabel(ui->lb_ed->text());

    ui->input_tip_wid->hide();

    auto find_it = std::find(_tip_data.begin(), _tip_data.end(), text);
    //鎵惧埌浜嗗氨鍙渶璁剧疆鐘舵€佷负閫変腑鍗冲彲
    if (find_it == _tip_data.end()) {
        _tip_data.push_back(text);
    }

    auto find_add = _add_labels.find(text);
    if (find_add != _add_labels.end()) {
        find_add.value()->SetCurState(ClickLbState::Selected);
        return;
    }

    //鏍囩灞曠ず鏍忎篃澧炲姞涓€涓爣绛? 骞惰缃豢鑹查€変腑
    auto* lb = new ClickedLabel(ui->lb_list);
    lb->SetState("normal", "hover", "pressed", "selected_normal",
        "selected_hover", "selected_pressed");
    lb->setObjectName("tipslb");
    lb->setText(text);
    connect(lb, &ClickedLabel::clicked, this, &ApplyFriend::SlotChangeFriendLabelByTip);
    qDebug() << "ui->lb_list->width() is " << ui->lb_list->width();
    qDebug() << "_tip_cur_point.x() is " << _tip_cur_point.x();

    QFontMetrics fontMetrics(lb->font());
    int textWidth = fontMetrics.horizontalAdvance(lb->text());
    int textHeight = fontMetrics.height();
    qDebug() << "textWidth is " << textWidth;

    if (_tip_cur_point.x() + textWidth + tip_offset + 3 > ui->lb_list->width()) {

        _tip_cur_point.setX(5);
        _tip_cur_point.setY(_tip_cur_point.y() + textHeight + 15);

    }

    auto next_point = _tip_cur_point;

    AddTipLbs(lb, _tip_cur_point, next_point, textWidth, textHeight);
    _tip_cur_point = next_point;

    int diff_height = next_point.y() + textHeight + tip_offset - ui->lb_list->height();
    ui->lb_list->setFixedHeight(next_point.y() + textHeight + tip_offset);

    lb->SetCurState(ClickLbState::Selected);

    ui->scrollcontent->setFixedHeight(ui->scrollcontent->height() + diff_height);
}

void ApplyFriend::SlotRemoveFriendLabel(QString name)
{
    qDebug() << "receive close signal";

    _label_point.setX(2);
    _label_point.setY(6);

   auto find_iter = _friend_labels.find(name);

   if(find_iter == _friend_labels.end()){
       return;
   }

   auto find_key = _friend_label_keys.end();
   for(auto iter = _friend_label_keys.begin(); iter != _friend_label_keys.end();
       iter++){
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

//鐐瑰嚮鏍囧凡鏈夌娣诲姞鎴栧垹闄ゆ柊鑱旂郴浜虹殑鏍囩
void ApplyFriend::SlotChangeFriendLabelByTip(QString lbtext, ClickLbState state)
{
    auto find_iter = _add_labels.find(lbtext);
    if(find_iter == _add_labels.end()){
        return;
    }

    if(state == ClickLbState::Selected){
        //缂栧啓娣诲姞閫昏緫
        addLabel(lbtext);
        return;
    }

    if(state == ClickLbState::Normal){
        //缂栧啓鍒犻櫎閫昏緫
        SlotRemoveFriendLabel(lbtext);
        return;
    }

}

void ApplyFriend::SlotLabelTextChange(const QString& text)
{
    if (text.isEmpty()) {
        ui->tip_lb->setText("");
        ui->input_tip_wid->hide();
        return;
    }

    auto iter = std::find(_tip_data.begin(), _tip_data.end(), text);
    if (iter == _tip_data.end()) {
        auto new_text = add_prefix + text;
        ui->tip_lb->setText(new_text);
        ui->input_tip_wid->show();
        return;
    }
    ui->tip_lb->setText(text);
    ui->input_tip_wid->show();
}

void ApplyFriend::SlotLabelEditFinished()
{
    ui->input_tip_wid->hide();
}

void ApplyFriend::SlotAddFirendLabelByClickTip(QString text)
{
    int index = text.indexOf(add_prefix);
    if (index != -1) {
        text = text.mid(index + add_prefix.length());
    }
    addLabel(text);

    auto find_it = std::find(_tip_data.begin(), _tip_data.end(), text);
    //鎵惧埌浜嗗氨鍙渶璁剧疆鐘舵€佷负閫変腑鍗冲彲
    if (find_it == _tip_data.end()) {
        _tip_data.push_back(text);
    }
   
    auto find_add = _add_labels.find(text);
    if (find_add != _add_labels.end()) {
        find_add.value()->SetCurState(ClickLbState::Selected);
        return;
    }
     
    //鏍囩灞曠ず鏍忎篃澧炲姞涓€涓爣绛? 骞惰缃豢鑹查€変腑
	auto* lb = new ClickedLabel(ui->lb_list);
	lb->SetState("normal", "hover", "pressed", "selected_normal",
		"selected_hover", "selected_pressed");
	lb->setObjectName("tipslb");
	lb->setText(text);
	connect(lb, &ClickedLabel::clicked, this, &ApplyFriend::SlotChangeFriendLabelByTip);
    qDebug() << "ui->lb_list->width() is " << ui->lb_list->width();
    qDebug() << "_tip_cur_point.x() is " << _tip_cur_point.x();
   
    QFontMetrics fontMetrics(lb->font());
    int textWidth = fontMetrics.horizontalAdvance(lb->text());
    int textHeight = fontMetrics.height();
    qDebug() << "textWidth is " << textWidth;

	if (_tip_cur_point.x() + textWidth+ tip_offset+3 > ui->lb_list->width()) {

		_tip_cur_point.setX(5);
		_tip_cur_point.setY(_tip_cur_point.y() + textHeight + 15);

	}

	auto next_point = _tip_cur_point;

	 AddTipLbs(lb, _tip_cur_point, next_point, textWidth,textHeight);
	_tip_cur_point = next_point;

    int diff_height = next_point.y() + textHeight + tip_offset - ui->lb_list->height();
    ui->lb_list->setFixedHeight(next_point.y() + textHeight + tip_offset);

    lb->SetCurState(ClickLbState::Selected);

    ui->scrollcontent->setFixedHeight(ui->scrollcontent->height()+ diff_height );
}

void ApplyFriend::SlotApplySure()
{
    qDebug()<<"Slot Apply Sure called" ;
    //鍙戦€佽姹傞€昏緫
    QJsonObject jsonObj;
    auto uid = UserMgr::GetInstance()->GetUid();
    jsonObj["uid"] = uid;
    auto name = ui->name_ed->text();
    if(name.isEmpty()){
        name = ui->name_ed->placeholderText();
    }

    jsonObj["applyname"] = name;

    auto bakname = ui->back_ed->text();
    if(bakname.isEmpty()){
        bakname = ui->back_ed->placeholderText();
    }

    jsonObj["bakname"] = bakname;
    jsonObj["touid"] = _si->_uid;

    QJsonDocument doc(jsonObj);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    //鍙戦€乼cp璇锋眰缁檆hat server
    emit TcpMgr::GetInstance()->sig_send_data(ReqId::ID_ADD_FRIEND_REQ, jsonData);
    this->hide();
    deleteLater();
}

void ApplyFriend::SlotApplyCancel()
{
    qDebug() << "Slot Apply Cancel";
    this->hide();
    deleteLater();
}

