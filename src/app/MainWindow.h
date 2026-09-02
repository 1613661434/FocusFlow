#pragma once

#include <QMainWindow>

class QLabel;
class QListWidget;
class QStackedWidget;
class QSystemTrayIcon;
class QCloseEvent;
class SettingsPage;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

public slots:
    void restoreAndActivate();

private slots:
    void showPage(int index);
    void showNotification(const QString &title, const QString &message);
    void updateTrayStatus(const QString &status);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    QWidget *createPlaceholderPage(const QString &title, const QString &description);
    void buildInterface();
    void applyTheme();
    void setupTray();
    void restoreNavigationSelection(int index);

    QListWidget *navigation_ = nullptr;
    QStackedWidget *pages_ = nullptr;
    QLabel *pageTitle_ = nullptr;
    QSystemTrayIcon *trayIcon_ = nullptr;
    SettingsPage *settingsPage_ = nullptr;
    QString trayStatusText_ = QStringLiteral(
        "FocusFlow\n当前没有正在运行的计时");
    bool quitRequested_ = false;
};
