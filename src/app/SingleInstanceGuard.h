#pragma once

#include <QObject>
#include <QString>

#include <memory>

class QLocalServer;
class QLockFile;

class SingleInstanceGuard final : public QObject
{
    Q_OBJECT

public:
    enum class StartResult {
        Primary,
        Secondary,
        Error,
    };

    explicit SingleInstanceGuard(const QString &instanceKey,
                                 QObject *parent = nullptr);
    ~SingleInstanceGuard() override;

    StartResult start(QString *errorMessage = nullptr);

signals:
    void activationRequested();

private:
    bool beginListening(QString *errorMessage);
    bool notifyPrimaryInstance();
    void handlePendingConnections();

    QString serverName_;
    std::unique_ptr<QLockFile> lockFile_;
    std::unique_ptr<QLocalServer> server_;
    bool primary_ = false;
};
