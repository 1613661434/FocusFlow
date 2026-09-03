#include "views/TimerPresetDialog.h"

#include "widgets/FocusAwareSpinBox.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

TimerPresetDialog::TimerPresetDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("新建专注方案"));
    setMinimumWidth(480);

    auto *root = new QVBoxLayout(this);
    root->setSpacing(14);
    form_ = new QFormLayout;
    form_->setHorizontalSpacing(18);
    form_->setVerticalSpacing(12);
    form_->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    nameEdit_ = new QLineEdit(this);
    nameEdit_->setObjectName(QStringLiteral("presetNameEdit"));
    nameEdit_->setMaxLength(40);
    nameEdit_->setPlaceholderText(QStringLiteral("例如：深度工作"));

    auto createMinutes = [this](const QString &objectName, int maximum) {
        auto *spin = new FocusAwareSpinBox(this);
        spin->setObjectName(objectName);
        spin->setRange(1, maximum);
        spin->setSuffix(QStringLiteral(" 分钟"));
        return spin;
    };
    focusMinutes_ = createMinutes(QStringLiteral("presetFocusMinutes"), 180);
    shortBreakMinutes_ = createMinutes(QStringLiteral("presetShortBreakMinutes"), 60);
    longBreakMinutes_ = createMinutes(QStringLiteral("presetLongBreakMinutes"), 120);
    cycles_ = new FocusAwareSpinBox(this);
    cycles_->setObjectName(QStringLiteral("presetCycles"));
    cycles_->setRange(2, 8);
    cycles_->setSuffix(QStringLiteral(" 次专注"));
    breakMode_ = new QComboBox(this);
    breakMode_->setObjectName(QStringLiteral("presetBreakMode"));
    breakMode_->addItem(QStringLiteral("正常休息"), true);
    breakMode_->addItem(QStringLiteral("不安排休息"), false);
    autoStartBreak_ = new QCheckBox(
        QStringLiteral("专注完成后自动开始休息"), this);
    autoStartBreak_->setObjectName(
        QStringLiteral("presetAutoStartBreak"));
    autoStartFocus_ = new QCheckBox(
        QStringLiteral("休息完成后自动开始专注"), this);
    autoStartFocus_->setObjectName(
        QStringLiteral("presetAutoStartFocus"));
    autoStartNextFocus_ = new QCheckBox(
        QStringLiteral("专注结束后自动开始下一轮"), this);
    autoStartNextFocus_->setObjectName(
        QStringLiteral("presetAutoStartNextFocus"));

    form_->addRow(QStringLiteral("方案名称："), nameEdit_);
    form_->addRow(QStringLiteral("专注时长："), focusMinutes_);
    form_->addRow(QStringLiteral("休息方式："), breakMode_);
    form_->addRow(QStringLiteral("短休息："), shortBreakMinutes_);
    form_->addRow(QStringLiteral("长休息："), longBreakMinutes_);
    form_->addRow(QStringLiteral("长休息间隔："), cycles_);
    form_->addRow(QString(), autoStartBreak_);
    form_->addRow(QString(), autoStartFocus_);
    form_->addRow(QString(), autoStartNextFocus_);

    auto *hint = new QLabel(
        QStringLiteral("任务可以绑定该方案；计时开始前也可临时换用其他方案。"),
        this);
    hint->setObjectName(QStringLiteral("mutedLabel"));
    hint->setWordWrap(true);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Save)->setText(QStringLiteral("保存"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    connect(buttons, &QDialogButtonBox::accepted,
            this, &TimerPresetDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected,
            this, &TimerPresetDialog::reject);
    connect(breakMode_, &QComboBox::currentIndexChanged,
            this, &TimerPresetDialog::updateBreakControls);

    root->addLayout(form_);
    root->addWidget(hint);
    root->addWidget(buttons);

    TimerPreset defaults;
    setPreset(defaults);
}

void TimerPresetDialog::setPreset(const TimerPreset &preset)
{
    originalPreset_ = preset;
    setWindowTitle(preset.id > 0 ? QStringLiteral("编辑专注方案")
                                  : QStringLiteral("新建专注方案"));
    nameEdit_->setText(preset.name);
    focusMinutes_->setValue(preset.focusMinutes);
    shortBreakMinutes_->setValue(preset.shortBreakMinutes);
    longBreakMinutes_->setValue(preset.longBreakMinutes);
    cycles_->setValue(preset.cyclesBeforeLongBreak);
    breakMode_->setCurrentIndex(preset.breaksEnabled ? 0 : 1);
    autoStartBreak_->setChecked(preset.autoStartBreak);
    autoStartFocus_->setChecked(preset.autoStartFocus);
    autoStartNextFocus_->setChecked(preset.autoStartNextFocus);
    updateBreakControls();
}

TimerPreset TimerPresetDialog::preset() const
{
    TimerPreset result = originalPreset_;
    result.name = nameEdit_->text().trimmed();
    result.focusMinutes = focusMinutes_->value();
    result.shortBreakMinutes = shortBreakMinutes_->value();
    result.longBreakMinutes = longBreakMinutes_->value();
    result.cyclesBeforeLongBreak = cycles_->value();
    result.breaksEnabled = breakMode_->currentData().toBool();
    result.autoStartBreak = result.breaksEnabled
                                && autoStartBreak_->isChecked();
    result.autoStartFocus = result.breaksEnabled
                                && autoStartFocus_->isChecked();
    result.autoStartNextFocus = !result.breaksEnabled
                                && autoStartNextFocus_->isChecked();
    return result;
}

void TimerPresetDialog::updateBreakControls()
{
    const bool breaksEnabled = breakMode_->currentData().toBool();
    const auto setRestFieldEnabled = [this, breaksEnabled](QWidget *field) {
        field->setEnabled(breaksEnabled);
        if (QWidget *label = form_->labelForField(field)) {
            label->setEnabled(breaksEnabled);
        }
    };
    setRestFieldEnabled(shortBreakMinutes_);
    setRestFieldEnabled(longBreakMinutes_);
    setRestFieldEnabled(cycles_);
    autoStartBreak_->setEnabled(breaksEnabled);
    autoStartFocus_->setEnabled(breaksEnabled);
    autoStartNextFocus_->setEnabled(!breaksEnabled);

    const QString disabledHint = QStringLiteral(
        "当前方案不安排休息，此项不会生效");
    shortBreakMinutes_->setToolTip(breaksEnabled ? QString() : disabledHint);
    longBreakMinutes_->setToolTip(breaksEnabled ? QString() : disabledHint);
    cycles_->setToolTip(breaksEnabled ? QString() : disabledHint);
    autoStartBreak_->setToolTip(breaksEnabled ? QString() : disabledHint);
    autoStartFocus_->setToolTip(breaksEnabled ? QString() : disabledHint);
    autoStartNextFocus_->setToolTip(
        breaksEnabled
            ? QStringLiteral("仅用于不安排休息的连续专注方案")
            : QStringLiteral("专注完成后直接开始下一轮专注"));
}

void TimerPresetDialog::accept()
{
    if (nameEdit_->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("无法保存"),
                             QStringLiteral("请输入方案名称。"));
        nameEdit_->setFocus();
        return;
    }
    QDialog::accept();
}
