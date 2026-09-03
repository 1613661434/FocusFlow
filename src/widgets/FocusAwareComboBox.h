#pragma once

#include <QComboBox>

class FocusAwareComboBox final : public QComboBox
{
    Q_OBJECT

public:
    explicit FocusAwareComboBox(QWidget *parent = nullptr);

protected:
    void wheelEvent(QWheelEvent *event) override;
};
