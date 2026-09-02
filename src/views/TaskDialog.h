#pragma once

#include "models/Task.h"
#include "models/TimerPreset.h"

#include <QDialog>

class QCheckBox;
class QComboBox;
class QDateTimeEdit;
class QLineEdit;
class QSpinBox;
class QTextEdit;

class TaskDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit TaskDialog(const QVector<LookupItem> &projects,
                        const QVector<LookupItem> &categories,
                        const QVector<TimerPreset> &presets,
                        QWidget *parent = nullptr);

    void setTask(const Task &task);
    Task task() const;

protected:
    void accept() override;

private:
    void buildInterface(const QVector<LookupItem> &projects,
                        const QVector<LookupItem> &categories,
                        const QVector<TimerPreset> &presets);
    static void selectId(QComboBox *comboBox, int id);

    Task originalTask_;
    QLineEdit *titleEdit_ = nullptr;
    QTextEdit *descriptionEdit_ = nullptr;
    QComboBox *projectCombo_ = nullptr;
    QComboBox *categoryCombo_ = nullptr;
    QComboBox *importanceCombo_ = nullptr;
    QComboBox *timerPresetCombo_ = nullptr;
    QCheckBox *dueEnabled_ = nullptr;
    QDateTimeEdit *dueEdit_ = nullptr;
    QSpinBox *dueHour_ = nullptr;
    QSpinBox *dueMinute_ = nullptr;
    QSpinBox *estimatedMinutes_ = nullptr;
};
