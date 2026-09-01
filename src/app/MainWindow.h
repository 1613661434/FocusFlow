#pragma once

#include <QMainWindow>

class QLabel;
class QListWidget;
class QStackedWidget;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void showPage(int index);

private:
    QWidget *createPlaceholderPage(const QString &title, const QString &description);
    void buildInterface();
    void applyTheme();

    QListWidget *navigation_ = nullptr;
    QStackedWidget *pages_ = nullptr;
    QLabel *pageTitle_ = nullptr;
};
