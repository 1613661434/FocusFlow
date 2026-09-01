#pragma once

#include <QString>

class DataManagementService final
{
public:
    bool backupDatabase(const QString &destinationPath,
                        QString *errorMessage = nullptr) const;
    bool prepareRestore(const QString &sourcePath,
                        QString *recoveryBackupPath = nullptr,
                        QString *errorMessage = nullptr) const;
    bool exportTasksCsv(const QString &destinationPath,
                        QString *errorMessage = nullptr) const;
    bool exportFocusSessionsCsv(const QString &destinationPath,
                                QString *errorMessage = nullptr) const;
    bool clearFocusStatistics(QString *errorMessage = nullptr) const;

private:
    bool validateBackup(const QString &sourcePath,
                        QString *errorMessage) const;
    static QString csvCell(const QString &value);
    static QString statusLabel(const QString &status);
    static QString sessionTypeLabel(const QString &sessionType);
    static void assignError(const QString &message, QString *destination);
};
