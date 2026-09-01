#pragma once

#include <QMainWindow>

class QLabel;
class QListWidget;
class QStackedWidget;
class QSystemTrayIcon;
class QEvent;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void showPage(int index);
    void showFromTray();
    void showNotification(const QString &title, const QString &message);

protected:
    void changeEvent(QEvent *event) override;

private:
    QWidget *createPlaceholderPage(const QString &title, const QString &description);
    void buildInterface();
    void applyTheme();
    void setupTray();

    QListWidget *navigation_ = nullptr;
    QStackedWidget *pages_ = nullptr;
    QLabel *pageTitle_ = nullptr;
    QSystemTrayIcon *trayIcon_ = nullptr;
};
