#ifndef LIVE2DASSET_H
#define LIVE2DASSET_H

#include <QObject>
#include <QJsonValue>
#include <QString>
#include <QStringList>

class Live2DAsset : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString modelRoot READ modelRoot WRITE setModelRoot NOTIFY assetInputsChanged)
    Q_PROPERTY(QString modelJson READ modelJson WRITE setModelJson NOTIFY assetInputsChanged)
    Q_PROPERTY(QString motionDirectory READ motionDirectory WRITE setMotionDirectory NOTIFY assetInputsChanged)
    Q_PROPERTY(QString expressionDirectory READ expressionDirectory WRITE setExpressionDirectory NOTIFY assetInputsChanged)
    Q_PROPERTY(QString voiceDirectory READ voiceDirectory WRITE setVoiceDirectory NOTIFY assetInputsChanged)
    Q_PROPERTY(QString physicsFile READ physicsFile NOTIFY validationChanged)
    Q_PROPERTY(QString poseFile READ poseFile NOTIFY validationChanged)
    Q_PROPERTY(QString userDataFile READ userDataFile NOTIFY validationChanged)
    Q_PROPERTY(QString vtubeMappingFile READ vtubeMappingFile WRITE setVtubeMappingFile NOTIFY validationChanged)
    Q_PROPERTY(QString packageChecksum READ packageChecksum NOTIFY validationChanged)
    Q_PROPERTY(bool valid READ valid NOTIFY validationChanged)
    Q_PROPERTY(QString status READ status NOTIFY validationChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY validationChanged)
    Q_PROPERTY(QStringList errors READ errors NOTIFY validationChanged)
    Q_PROPERTY(QStringList warnings READ warnings NOTIFY validationChanged)
    Q_PROPERTY(int motionCount READ motionCount NOTIFY validationChanged)
    Q_PROPERTY(int expressionCount READ expressionCount NOTIFY validationChanged)
    Q_PROPERTY(int textureCount READ textureCount NOTIFY validationChanged)
    Q_PROPERTY(int voiceCount READ voiceCount NOTIFY validationChanged)
    Q_PROPERTY(int referencedFileCount READ referencedFileCount NOTIFY validationChanged)

public:
    explicit Live2DAsset(QObject *parent = nullptr);

    QString modelRoot() const { return _model_root; }
    QString modelJson() const { return _model_json; }
    QString motionDirectory() const { return _motion_directory; }
    QString expressionDirectory() const { return _expression_directory; }
    QString voiceDirectory() const { return _voice_directory; }
    QString vtubeMappingFile() const { return _resolved_vtube_mapping_file.isEmpty() ? _vtube_mapping_file : _resolved_vtube_mapping_file; }
    QString physicsFile() const { return _physics_file; }
    QString poseFile() const { return _pose_file; }
    QString userDataFile() const { return _user_data_file; }
    QString packageChecksum() const { return _package_checksum; }

    bool valid() const { return _valid; }
    QString status() const { return _status; }
    QString statusText() const { return _status_text; }
    QStringList errors() const { return _errors; }
    QStringList warnings() const { return _warnings; }
    int motionCount() const { return _motion_count; }
    int expressionCount() const { return _expression_count; }
    int textureCount() const { return _texture_count; }
    int voiceCount() const { return _voice_count; }
    int referencedFileCount() const { return _referenced_file_count; }

    Q_INVOKABLE void validate();
    Q_INVOKABLE void clear();

public slots:
    void setModelRoot(const QString &value);
    void setModelJson(const QString &value);
    void setMotionDirectory(const QString &value);
    void setExpressionDirectory(const QString &value);
    void setVoiceDirectory(const QString &value);
    void setVtubeMappingFile(const QString &value);

signals:
    void assetInputsChanged();
    void validationChanged();

private:
    static bool assignIfChanged(QString &target, const QString &value);
    static QString cleanedInput(const QString &value);
    static QString resolveInputPath(const QString &value, const QString &baseDirectory = QString());
    static QString resolveModelReference(const QString &modelDirectory, const QString &reference);
    static int countFilesWithSuffixes(const QString &directoryPath, const QStringList &suffixes);
    static QStringList jsonStringArray(const QJsonValue &value);

    void publishResult(const QString &status,
                       const QString &statusText,
                       const QStringList &errors,
                       const QStringList &warnings,
                       int motionCount,
                       int expressionCount,
                       int textureCount,
                       int voiceCount,
                       const QString &physicsFile,
                       const QString &poseFile,
                       const QString &userDataFile,
                       const QString &vtubeMappingFile,
                       const QString &packageChecksum,
                       int referencedFileCount);

    QString _model_root;
    QString _model_json;
    QString _motion_directory;
    QString _expression_directory;
    QString _voice_directory;
    QString _vtube_mapping_file;
    QString _resolved_vtube_mapping_file;
    QString _physics_file;
    QString _pose_file;
    QString _user_data_file;
    QString _package_checksum;

    bool _valid = false;
    QString _status = QStringLiteral("empty");
    QString _status_text = QStringLiteral("等待选择 model3.json");
    QStringList _errors;
    QStringList _warnings;
    int _motion_count = 0;
    int _expression_count = 0;
    int _texture_count = 0;
    int _voice_count = 0;
    int _referenced_file_count = 0;
};

#endif // LIVE2DASSET_H
