#include "widgets/FocusAwareComboBox.h"

#include <QWheelEvent>

FocusAwareComboBox::FocusAwareComboBox(QWidget *parent)
    : QComboBox(parent)
{
    setFocusPolicy(Qt::StrongFocus);
}

void FocusAwareComboBox::wheelEvent(QWheelEvent *event)
{
    if (!hasFocus()) {
        event->ignore();
        return;
    }
    QComboBox::wheelEvent(event);
}
