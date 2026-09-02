#include "widgets/FocusAwareSlider.h"

#include <QWheelEvent>

FocusAwareSlider::FocusAwareSlider(Qt::Orientation orientation,
                                   QWidget *parent)
    : QSlider(orientation, parent)
{
    // 只有点击或 Tab 选中滑块后，滚轮才用于调节数值。
    setFocusPolicy(Qt::StrongFocus);
}

void FocusAwareSlider::wheelEvent(QWheelEvent *event)
{
    if (!hasFocus()) {
        event->ignore();
        return;
    }
    QSlider::wheelEvent(event);
}
