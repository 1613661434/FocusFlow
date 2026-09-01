#pragma once

#include <QSpinBox>

class FocusAwareSpinBox final : public QSpinBox
{
public:
    explicit FocusAwareSpinBox(QWidget *parent = nullptr);

protected:
    void wheelEvent(QWheelEvent *event) override;
};
