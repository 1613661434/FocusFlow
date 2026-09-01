#include "widgets/FocusAwareSpinBox.h"

#include <QLineEdit>
#include <QWheelEvent>

FocusAwareSpinBox::FocusAwareSpinBox(QWidget *parent)
    : QSpinBox(parent)
{
    // StrongFocus accepts clicks and Tab navigation, but wheel hovering alone
    // must not move focus away from the surrounding scroll area.
    setFocusPolicy(Qt::StrongFocus);
}

void FocusAwareSpinBox::wheelEvent(QWheelEvent *event)
{
    const bool editorHasFocus = lineEdit() != nullptr && lineEdit()->hasFocus();
    if (!hasFocus() && !editorHasFocus) {
        event->ignore();
        return;
    }
    QSpinBox::wheelEvent(event);
}
