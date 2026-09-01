#include "app/MainWindow.h"
#include "data/DatabaseManager.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QIcon>
#include <QLibraryInfo>
#include <QLocale>
#include <QMessageBox>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("FocusFlow"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.15"));
    // AppDataLocation 只使用应用名，避免生成 FocusFlow/FocusFlow 双层目录。
    QCoreApplication::setOrganizationName(QString());
    QLocale::setDefault(QLocale(QLocale::Chinese, QLocale::China));

    const QString packagedTranslations = QDir(
        QCoreApplication::applicationDirPath()).filePath(
        QStringLiteral("translations"));
    const QString installedTranslations =
        QLibraryInfo::path(QLibraryInfo::TranslationsPath);
    auto loadTranslation = [&](QTranslator &translator,
                               const QString &catalog) {
        return translator.load(catalog, packagedTranslations)
               || translator.load(catalog, installedTranslations);
    };
    QTranslator qtTranslator;
    QTranslator qtBaseTranslator;
    if (loadTranslation(qtTranslator, QStringLiteral("qt_zh_CN"))) {
        application.installTranslator(&qtTranslator);
    }
    if (loadTranslation(qtBaseTranslator, QStringLiteral("qtbase_zh_CN"))) {
        application.installTranslator(&qtBaseTranslator);
    }

    application.setWindowIcon(QIcon(QStringLiteral(":/icons/focusflow.svg")));

    auto &databaseManager = DatabaseManager::instance();
    if (!databaseManager.initialize()) {
        QMessageBox::critical(nullptr,
                              QStringLiteral("无法启动 FocusFlow"),
                              QStringLiteral("数据库初始化失败：\n%1")
                                  .arg(databaseManager.lastError()));
        return 1;
    }

    MainWindow window;
    window.show();

    return application.exec();
}
