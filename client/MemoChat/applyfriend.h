#ifndef APPLYFRIEND_H
#define APPLYFRIEND_H

#include <QDialog>
#include "clickedlabel.h"
#include "friendlabel.h"
#include "userdata.h"
namespace Ui {
class ApplyFriend;
}

class ApplyFriend : public QDialog
{
    Q_OBJECT

public:
    explicit ApplyFriend(QWidget *parent = nullptr);
    ~ApplyFriend();
    void InitTipLbs();
    void AddTipLbs(ClickedLabel*, QPoint cur_point, QPoint &next_point, int text_width, int text_height);
    bool eventFilter(QObject *obj, QEvent *event);
    void SetSearchInfo(std::shared_ptr<SearchInfo> si);
private:
    void resetLabels();
    Ui::ApplyFriend *ui;
    // Labels that have already been created.
    QMap<QString, ClickedLabel*> _add_labels;
    std::vector<QString> _add_label_keys;
    QPoint _label_point;
    // Labels used in the input area for adding new friends.
    QMap<QString, FriendLabel*> _friend_labels;
    std::vector<QString> _friend_label_keys;
    void addLabel(QString name);
    std::vector<QString> _tip_data;
    QPoint _tip_cur_point;
    std::shared_ptr<SearchInfo> _si;
public slots:
    // Show more label tags.
    void ShowMoreLabel();
    // Press Enter in the label input to add it into the display area.
    void SlotLabelEnter();
    // Click close to remove a friend tag from the display area.
    void SlotRemoveFriendLabel(QString);
    // Increase or decrease friend tags by clicking tips.
    void SlotChangeFriendLabelByTip(QString, ClickLbState);
    // Show different tips when input text changes.
    void SlotLabelTextChange(const QString& text);
    // Input editing finished.
    void SlotLabelEditFinished();
   // Show tip popup while typing labels; click tip to add friend tag.
    void SlotAddFirendLabelByClickTip(QString text);
    // Handle confirm callback.
    void SlotApplySure();
    // Handle cancel callback.
    void SlotApplyCancel();
};

#endif // APPLYFRIEND_H
