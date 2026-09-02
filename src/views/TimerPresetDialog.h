#pragma once

#include "models/TimerPreset.h"

#include <QDialog>

class QCheckBox;
class QLineEdit;
class QSpinBox;

class TimerPresetDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit TimerPresetDialog(QWidget *parent = nullptr);

    void setPreset(const TimerPreset &preset);
    TimerPreset preset() const;

protected:
    void accept() override;

private:
    TimerPreset originalPreset_;
    QLineEdit *nameEdit_ = nullptr;
    QSpinBox *focusMinutes_ = nullptr;
    QSpinBox *shortBreakMinutes_ = nullptr;
    QSpinBox *longBreakMinutes_ = nullptr;
    QSpinBox *cycles_ = nullptr;
    QCheckBox *autoStartBreak_ = nullptr;
    QCheckBox *autoStartFocus_ = nullptr;
};
