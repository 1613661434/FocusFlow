#pragma once

#include <QWidget>

class QLabel;
class QListWidget;

class DashboardPage final : public QWidget
{
    Q_OBJECT

public:
    explicit DashboardPage(QWidget *parent = nullptr);

public slots:
    void refresh();

private:
    void buildInterface();
    QWidget *createMetricCard(const QString &title, QLabel **valueLabel);
    static QString formatDuration(int seconds);

    QLabel *pendingValue_ = nullptr;
    QLabel *todayValue_ = nullptr;
    QLabel *overdueValue_ = nullptr;
    QLabel *completedValue_ = nullptr;
    QLabel *focusValue_ = nullptr;
    QListWidget *recommendations_ = nullptr;
};
