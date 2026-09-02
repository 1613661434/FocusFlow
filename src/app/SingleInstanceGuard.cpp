#include "app/SingleInstanceGuard.h"

#include <QCryptographicHash>
#include <QDir>
#include <QElapsedTimer>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLockFile>
#include <QStandardPaths>
#include <QThread>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

namespace {
constexpr int kConnectionRetryDurationMs = 2000;
constexpr int kConnectionAttemptTimeoutMs = 150;
constexpr int kConnectionRetryIntervalMs = 50;

QString normalizedInstanceName(const QString &instanceKey)
{
    const QByteArray digest = QCryptographicHash::hash(
                                  instanceKey.toUtf8(),
                                  QCryptographicHash::Sha256)
                                  .toHex()
                                  .left(24);
    return QStringLiteral("FocusFlow-%1")
        .arg(QString::fromLatin1(digest));
}
}

SingleInstanceGuard::SingleInstanceGuard(const QString &instanceKey,
                                         QObject *parent)
    : QObject(parent),
      serverName_(normalizedInstanceName(instanceKey)),
      lockFile_(std::make_unique<QLockFile>(
          QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
              .filePath(serverName_ + QStringLiteral(".lock"))))
{
}

SingleInstanceGuard::~SingleInstanceGuard()
{
    if (server_ != nullptr) {
        server_->close();
    }
    if (primary_) {
        QLocalServer::removeServer(serverName_);
    }
}

SingleInstanceGuard::StartResult SingleInstanceGuard::start(
    QString *errorMessage)
{
    if (primary_) {
        return StartResult::Primary;
    }

    if (lockFile_->tryLock(0)) {
        return beginListening(errorMessage)
            ? StartResult::Primary
            : StartResult::Error;
    }

#ifdef Q_OS_WIN
    qint64 primaryProcessId = 0;
    QString primaryHostName;
    QString primaryApplicationName;
    if (lockFile_->getLockInfo(&primaryProcessId,
                               &primaryHostName,
                               &primaryApplicationName)) {
        AllowSetForegroundWindow(static_cast<DWORD>(primaryProcessId));
    }
#endif

    if (notifyPrimaryInstance()) {
        return StartResult::Secondary;
    }

    if (lockFile_->removeStaleLockFile() && lockFile_->tryLock(0)) {
        return beginListening(errorMessage)
            ? StartResult::Primary
            : StartResult::Error;
    }

    if (errorMessage != nullptr) {
        *errorMessage = QStringLiteral(
            "检测到 FocusFlow 已在运行，但无法连接到原窗口。请稍后重试，"
            "或从任务管理器结束异常进程后再启动。");
    }
    return StartResult::Error;
}

bool SingleInstanceGuard::beginListening(QString *errorMessage)
{
    QLocalServer::removeServer(serverName_);
    server_ = std::make_unique<QLocalServer>();
    connect(server_.get(), &QLocalServer::newConnection,
            this, &SingleInstanceGuard::handlePendingConnections);

    if (!server_->listen(serverName_)) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("无法建立程序实例通信：%1")
                                .arg(server_->errorString());
        }
        server_.reset();
        lockFile_->unlock();
        return false;
    }

    primary_ = true;
    return true;
}

bool SingleInstanceGuard::notifyPrimaryInstance()
{
    QElapsedTimer retryTimer;
    retryTimer.start();

    do {
        QLocalSocket socket;
        socket.connectToServer(serverName_, QIODevice::WriteOnly);
        if (socket.waitForConnected(kConnectionAttemptTimeoutMs)) {
            socket.write("activate\n");
            socket.waitForBytesWritten(kConnectionAttemptTimeoutMs);
            // 等待主进程确认，避免客户端过早关闭导致 Windows 命名管道
            // 尚未被主进程接收。
            socket.waitForReadyRead(kConnectionAttemptTimeoutMs * 4);
            socket.disconnectFromServer();
            return true;
        }
        QThread::msleep(kConnectionRetryIntervalMs);
    } while (retryTimer.elapsed() < kConnectionRetryDurationMs);

    return false;
}

void SingleInstanceGuard::handlePendingConnections()
{
    bool receivedActivationRequest = false;
    while (server_ != nullptr && server_->hasPendingConnections()) {
        QLocalSocket *socket = server_->nextPendingConnection();
        if (socket == nullptr) {
            continue;
        }
        receivedActivationRequest = true;
        socket->write("accepted\n");
        socket->flush();
        socket->disconnectFromServer();
        socket->deleteLater();
    }

    if (receivedActivationRequest) {
        emit activationRequested();
    }
}
