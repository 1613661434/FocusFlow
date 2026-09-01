#include "services/SoundStorageService.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QSet>
#include <QUuid>

namespace {
QString normalizedPath(const QString &path)
{
    return QDir::cleanPath(QFileInfo(path).absoluteFilePath());
}

Qt::CaseSensitivity pathCaseSensitivity()
{
#ifdef Q_OS_WIN
    return Qt::CaseInsensitive;
#else
    return Qt::CaseSensitive;
#endif
}
}

QString SoundStorageService::directoryPath() const
{
    const QString dataPath =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return QDir(dataPath).filePath(QStringLiteral("sounds"));
}

bool SoundStorageService::isManagedPath(const QString &path) const
{
    if (path.isEmpty()) {
        return false;
    }
    const QString parent = normalizedPath(QFileInfo(path).absolutePath());
    return parent.compare(normalizedPath(directoryPath()),
                          pathCaseSensitivity()) == 0;
}

QString SoundStorageService::install(const QString &sourcePath,
                                     const QString &prefix,
                                     QString *errorMessage) const
{
    if (sourcePath.isEmpty()) {
        return {};
    }
    if (isManagedPath(sourcePath) && QFileInfo::exists(sourcePath)) {
        return normalizedPath(sourcePath);
    }

    const QFileInfo source(sourcePath);
    if (!source.exists() || !source.isFile() || !source.isReadable()) {
        assignError(errorMessage,
                    QStringLiteral("无法读取声音文件：%1").arg(sourcePath));
        return {};
    }

    QDir soundDirectory(directoryPath());
    if (!soundDirectory.mkpath(QStringLiteral("."))) {
        assignError(errorMessage,
                    QStringLiteral("无法创建声音数据目录：%1")
                        .arg(soundDirectory.absolutePath()));
        return {};
    }

    const QString safePrefix = prefix.isEmpty() ? QStringLiteral("sound") : prefix;
    const QString suffix = source.suffix().toLower();
    const QString identifier =
        QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QString fileName = suffix.isEmpty()
                                 ? QStringLiteral("%1-%2").arg(safePrefix, identifier)
                                 : QStringLiteral("%1-%2.%3")
                                       .arg(safePrefix, identifier, suffix);
    const QString destination = soundDirectory.filePath(fileName);
    if (!QFile::copy(source.absoluteFilePath(), destination)) {
        assignError(errorMessage,
                    QStringLiteral("无法将声音文件复制到应用数据目录。"));
        return {};
    }
    return normalizedPath(destination);
}

QStringList SoundStorageService::removeUnused(const QStringList &retainedPaths) const
{
    QSet<QString> retained;
    for (const QString &path : retainedPaths) {
        if (isManagedPath(path)) {
            QString normalized = normalizedPath(path);
#ifdef Q_OS_WIN
            normalized = normalized.toLower();
#endif
            retained.insert(normalized);
        }
    }

    QStringList failures;
    QDir directory(directoryPath());
    const QFileInfoList files = directory.entryInfoList(
        QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
    for (const QFileInfo &file : files) {
        QString normalized = normalizedPath(file.absoluteFilePath());
#ifdef Q_OS_WIN
        normalized = normalized.toLower();
#endif
        if (!retained.contains(normalized) && !QFile::remove(file.absoluteFilePath())) {
            failures.append(file.absoluteFilePath());
        }
    }
    return failures;
}

void SoundStorageService::assignError(QString *errorMessage,
                                      const QString &message)
{
    if (errorMessage) {
        *errorMessage = message;
    }
}
