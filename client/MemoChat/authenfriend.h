#ifndef AUTHENFRIEND_H
#define AUTHENFRIEND_H

#include <QDialog>
#include "clickedlabel.h"
#include "friendlabel.h"
#include "userdata.h"

namespace Ui {
class AuthenFriend;
}

class AuthenFriend : public QDialog
{
    Q_OBJECT

public:
    explicit AuthenFriend(QWidget *parent = nullptr);
    ~AuthenFriend();

    void InitTipLbs();
    void AddTipLbs(ClickedLabel*, QPoint cur_point, QPoint &next_point, int text_width, int text_height);
    bool eventFilter(QObject *obj, QEvent *event);
    void SetApplyInfo(std::shared_ptr<ApplyInfo> apply_info);
private:
    void resetLabels();


    QMap<QString, ClickedLabel*> _add_labels;
    std::vector<QString> _add_label_keys;
    QPoint _label_point;

    QMap<QString, FriendLabel*> _friend_labels;
    std::vector<QString> _friend_label_keys;
    void addLabel(QString name);
    std::vector<QString> _tip_data;
    QPoint _tip_cur_point;
public slots:

    void ShowMoreLabel();

    void SlotLabelEnter();

    void SlotRemoveFriendLabel(QString);

    void SlotChangeFriendLabelByTip(QString, ClickLbState);

    void SlotLabelTextChange(const QString& text);

    void SlotLabelEditFinished();

    void SlotAddFirendLabelByClickTip(QString text);

    void SlotApplySure();

    void SlotApplyCancel();
   
private:
 	std::shared_ptr<ApplyInfo> _apply_info;
    Ui::AuthenFriend *ui;
};

#endif // AUTHENFRIEND_H
