#pragma once

#include <QSpinBox>

class FocusAwareSpinBox final : public QSpinBox
{
public:
    explicit FocusAwareSpinBox(QWidget *parent = nullptr);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    void clampEditorCursor();
};
