#include "WebcamCaptureDialog.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QApplication>

#if HAVE_QT_MULTIMEDIA
#include <QCamera>
#include <QCameraImageCapture>
#include <QCameraInfo>
#include <QVideoWidget>
#endif

class WebcamCaptureDialogPrivate : public QDialog {
    Q_OBJECT
public:
    WebcamCaptureDialogPrivate(QPixmap &outPixmap, QWidget *parent = nullptr)
        : QDialog(parent), _outPixmap(outPixmap)
    {
        setAttribute(Qt::WA_DeleteOnClose);
        setWindowTitle(QStringLiteral("拍照头像"));
        resize(640, 520);

        QVBoxLayout *mainLayout = new QVBoxLayout(this);

        _previewLabel = new QLabel(this);
        _previewLabel->setMinimumSize(480, 360);
        _previewLabel->setAlignment(Qt::AlignCenter);
        _previewLabel->setStyleSheet(QStringLiteral("background-color: #1a1a1a; color: #888;"));
        _previewLabel->setText(QStringLiteral("正在初始化摄像头..."));
        mainLayout->addWidget(_previewLabel, 1);

        QHBoxLayout *btnLayout = new QHBoxLayout();
        btnLayout->addStretch();
        _btnCapture = new QPushButton(QStringLiteral("拍照"), this);
        _btnCancel = new QPushButton(QStringLiteral("取消"), this);
        _btnCapture->setMinimumWidth(80);
        _btnCancel->setMinimumWidth(80);
        btnLayout->addWidget(_btnCapture);
        btnLayout->addWidget(_btnCancel);
        mainLayout->addLayout(btnLayout);

        connect(_btnCapture, &QPushButton::clicked, this, &WebcamCaptureDialogPrivate::onCapture);
        connect(_btnCancel, &QPushButton::clicked, this, &WebcamCaptureDialogPrivate::onCancel);

#if HAVE_QT_MULTIMEDIA
        initCamera();
#else
        _previewLabel->setText(QStringLiteral("摄像头功能未编译\n请在 CMakeLists.txt 中启用 Qt Multimedia"));
        _btnCapture->setEnabled(false);
#endif
    }

private slots:
    void onCapture()
    {
#if HAVE_QT_MULTIMEDIA
        if (_imageCapture && _imageCapture->isReadyForCapture()) {
            _imageCapture->capture();
        }
#else
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("摄像头功能未启用"));
#endif
    }

    void onCancel()
    {
        _outPixmap = QPixmap();
        reject();
    }

#if HAVE_QT_MULTIMEDIA
    void initCamera()
    {
        const QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
        if (cameras.isEmpty()) {
            _previewLabel->setText(QStringLiteral("未检测到可用摄像头"));
            _btnCapture->setEnabled(false);
            return;
        }

        QCameraInfo cameraInfo = cameras.first();
        for (const QCameraInfo &ci : cameras) {
            // Prefer built-in / back camera
            if (ci.deviceName().contains(QStringLiteral("back"),
                                          Qt::CaseInsensitive) ||
                ci.deviceName().contains(QStringLiteral("内置"),
                                          Qt::CaseInsensitive)) {
                cameraInfo = ci;
                break;
            }
        }

        _camera = new QCamera(cameraInfo, this);
        _imageCapture = new QCameraImageCapture(_camera, this);

        connect(_imageCapture, &QCameraImageCapture::imageCaptured,
                this, &WebcamCaptureDialogPrivate::onImageCaptured);
        connect(_imageCapture, &QCameraImageCapture::error,
                this, [this](int, QCameraImageCapture::Error, const QString &err) {
            _previewLabel->setText(QStringLiteral("摄像头错误: %1").arg(err));
        });

        QCameraViewfinder *viewfinder = new QCameraViewfinder(this);
        viewfinder->setMinimumSize(480, 360);
        QVBoxLayout *vl = qobject_cast<QVBoxLayout*>(layout());
        if (vl) {
            delete vl->takeAt(0)->widget();
            vl->insertWidget(0, viewfinder);
        }
        _camera->setViewfinder(viewfinder);
        _camera->start();
    }

    void onImageCaptured(int id, const QImage &image)
    {
        Q_UNUSED(id);
        _capturedImage = image;
        if (!image.isNull()) {
            QPixmap pm = QPixmap::fromImage(image);
            _outPixmap = pm;
            accept();
        }
    }
#endif

private:
    QLabel *_previewLabel = nullptr;
    QPushButton *_btnCapture = nullptr;
    QPushButton *_btnCancel = nullptr;
    QPixmap &_outPixmap;
#if HAVE_QT_MULTIMEDIA
    QCamera *_camera = nullptr;
    QCameraImageCapture *_imageCapture = nullptr;
    QImage _capturedImage;
#endif
};

WebcamCaptureDialog::WebcamCaptureDialog(QWidget *parent)
    : QDialog(parent) {
}

WebcamCaptureDialog::~WebcamCaptureDialog() = default;

#include "WebcamCaptureDialog.moc"

QPixmap WebcamCaptureDialog::capture(QWidget *parent)
{
    QPixmap out;
    WebcamCaptureDialogPrivate dlg(out, parent);
    dlg.exec();
    return out;
}
