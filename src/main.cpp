#include "app/MainWindow.h"
#include "data/DatabaseManager.h"

#include <QApplication>
#include <QCoreApplication>
#include <QIcon>
#include <QLocale>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("FocusFlow"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.3"));
    QCoreApplication::setOrganizationName(QStringLiteral("FocusFlow"));
    QLocale::setDefault(QLocale(QLocale::Chinese, QLocale::China));
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
