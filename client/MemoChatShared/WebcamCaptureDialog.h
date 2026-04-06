#pragma once

#include <QDialog>
#include <QPixmap>

class WebcamCaptureDialog : public QDialog {
    Q_OBJECT
public:
    explicit WebcamCaptureDialog(QWidget *parent = nullptr);
    ~WebcamCaptureDialog();

    static QPixmap capture(QWidget *parent = nullptr);

private:
    QPixmap _captured;
};
