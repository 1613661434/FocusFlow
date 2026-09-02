#pragma once

#include <QTableWidget>

class FocusAwareTableWidget final : public QTableWidget
{
public:
    explicit FocusAwareTableWidget(QWidget *parent = nullptr);

protected:
    void wheelEvent(QWheelEvent *event) override;
};
