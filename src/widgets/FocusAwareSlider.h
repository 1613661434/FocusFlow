#pragma once

#include <QSlider>

class FocusAwareSlider final : public QSlider
{
public:
    explicit FocusAwareSlider(Qt::Orientation orientation,
                              QWidget *parent = nullptr);

protected:
    void wheelEvent(QWheelEvent *event) override;
};
