#pragma once

#include <QString>
#include <QStringList>

class SoundStorageService final
{
public:
    QString directoryPath() const;
    bool isManagedPath(const QString &path) const;
    QString install(const QString &sourcePath,
                    const QString &prefix,
                    QString *errorMessage = nullptr) const;
    QStringList removeUnused(const QStringList &retainedPaths) const;

private:
    static void assignError(QString *errorMessage, const QString &message);
};
