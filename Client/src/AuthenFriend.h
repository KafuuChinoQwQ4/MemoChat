#pragma once
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QScrollArea>
#include <QMap>
#include <vector>
#include "userdata.h"
#include "global.h"

class CustomizeEdit;
class ClickedLabel;
class FriendLabel;
class ClickedOnceLabel;
class ClickedBtn;

class AuthenFriend : public QDialog
{
    Q_OBJECT

public:
    explicit AuthenFriend(QWidget *parent = nullptr);
    ~AuthenFriend();

    void InitTipLbs();
    void AddTipLbs(ClickedLabel* lb, QPoint cur_point, QPoint& next_point, int text_width, int text_height);
    bool eventFilter(QObject *obj, QEvent *event) override;
    void SetApplyInfo(std::shared_ptr<ApplyInfo> apply_info);

private:
    void initUI();
    void resetLabels();
    void addLabel(QString name);

    // UI Components
    CustomizeEdit* _lb_ed;
    CustomizeEdit* _name_ed;
    CustomizeEdit* _back_ed;
    QWidget* _input_tip_wid;
    ClickedOnceLabel* _tip_lb;
    ClickedOnceLabel* _more_lb;
    QWidget* _more_lb_wid;
    QWidget* _lb_list;
    QScrollArea* _scrollArea;
    QWidget* _scrollcontent;
    QWidget* _gridWidget;
    ClickedBtn* _sure_btn;
    ClickedBtn* _cancel_btn;

    std::shared_ptr<ApplyInfo> _apply_info;
    QMap<QString, ClickedLabel*> _add_labels;
    std::vector<QString> _add_label_keys;
    QPoint _label_point;
    QMap<QString, FriendLabel*> _friend_labels;
    std::vector<QString> _friend_label_keys;
    std::vector<QString> _tip_data;
    QPoint _tip_cur_point;

    const int tip_offset = 5;
    const int MIN_APPLY_LABEL_ED_LEN = 50;
    QString add_prefix = "添加标签: ";

private slots:
    void ShowMoreLabel();
    void SlotLabelEnter();
    void SlotRemoveFriendLabel(QString name);
    void SlotChangeFriendLabelByTip(QString lbtext, ClickLbState state);
    void SlotLabelTextChange(const QString& text);
    void SlotLabelEditFinished();
    void SlotAddFirendLabelByClickTip(QString text);
    void SlotApplySure();
    void SlotApplyCancel();
};