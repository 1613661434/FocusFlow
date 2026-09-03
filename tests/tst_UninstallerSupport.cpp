#include "uninstaller/UninstallerSupport.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

class UninstallerSupportTests final : public QObject
{
    Q_OBJECT

private slots:
    void rejectsBroadOrUnmarkedProgramDirectories();
    void removesOnlyMarkedProgramTree();
    void removesOnlyTheExpectedDataTree();
};

void UninstallerSupportTests::rejectsBroadOrUnmarkedProgramDirectories()
{
    QVERIFY(!UninstallerSupport::isSafeProgramDirectory(L"C:\\"));

    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    QVERIFY(!UninstallerSupport::isSafeProgramDirectory(
        QDir::toNativeSeparators(temporaryDirectory.path()).toStdWString()));
}

void UninstallerSupportTests::removesOnlyMarkedProgramTree()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    temporaryDirectory.setAutoRemove(false);
    QDir directory(temporaryDirectory.path());
    QVERIFY(directory.mkpath(QStringLiteral("plugins/platforms")));

    QFile executable(directory.filePath(QStringLiteral("FocusFlow.exe")));
    QVERIFY(executable.open(QIODevice::WriteOnly));
    executable.write("test");
    executable.close();
    QFile plugin(directory.filePath(
        QStringLiteral("plugins/platforms/qwindows.dll")));
    QVERIFY(plugin.open(QIODevice::WriteOnly));
    plugin.write("test");
    plugin.close();

    const std::wstring nativePath =
        QDir::toNativeSeparators(temporaryDirectory.path()).toStdWString();
    QVERIFY(UninstallerSupport::isSafeProgramDirectory(nativePath));
    QVERIFY(UninstallerSupport::removeProgramDirectory(nativePath));
    QVERIFY(!QDir(temporaryDirectory.path()).exists());
}

void UninstallerSupportTests::removesOnlyTheExpectedDataTree()
{
    QTemporaryDir root;
    QVERIFY(root.isValid());
    QDir directory(root.path());
    QVERIFY(directory.mkpath(QStringLiteral("FocusFlow/sounds")));
    const QString dataPath = directory.filePath(QStringLiteral("FocusFlow"));
    QFile database(QDir(dataPath).filePath(QStringLiteral("focusflow.db")));
    QVERIFY(database.open(QIODevice::WriteOnly));
    database.write("test");
    database.close();

    const std::wstring nativePath =
        QDir::toNativeSeparators(dataPath).toStdWString();
    QVERIFY(!UninstallerSupport::removeDataDirectory(
        nativePath, nativePath + L"-other"));
    QVERIFY(QDir(dataPath).exists());
    QVERIFY(UninstallerSupport::removeDataDirectory(nativePath, nativePath));
    QVERIFY(!QDir(dataPath).exists());
}

QTEST_GUILESS_MAIN(UninstallerSupportTests)

#include "tst_UninstallerSupport.moc"
