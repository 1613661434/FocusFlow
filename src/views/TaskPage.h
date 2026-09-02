#pragma once

#include "models/Task.h"
#include "repositories/TaskRepository.h"

#include <QWidget>

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;

class TaskPage final : public QWidget
{
    Q_OBJECT

public:
    explicit TaskPage(QWidget *parent = nullptr);

signals:
    void tasksChanged();

public slots:
    void refresh();
    void setActiveFocusTask(int taskId);

private slots:
    void addTask();
    void editSelectedTask();
    void toggleSelectedTask();
    void deleteSelectedTask();
    void refreshForFilter();

private:
    void buildInterface();
    int selectedTaskId() const;
    int selectedTaskIndex() const;
    void showRepositoryError(const QString &action, const QString &details);
    static QString statusText(const QString &status);
    static QString importanceText(int importance);

    TaskRepository repository_;
    QVector<Task> tasks_;
    QLineEdit *searchEdit_ = nullptr;
    QComboBox *filterCombo_ = nullptr;
    QTableWidget *table_ = nullptr;
    QLabel *summaryLabel_ = nullptr;
    QPushButton *completeButton_ = nullptr;
    int activeFocusTaskId_ = -1;
};
