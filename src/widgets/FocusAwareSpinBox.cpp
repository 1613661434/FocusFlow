#include "widgets/FocusAwareSpinBox.h"

#include <QEvent>
#include <QLineEdit>
#include <QTimer>
#include <QWheelEvent>

FocusAwareSpinBox::FocusAwareSpinBox(QWidget *parent)
    : QSpinBox(parent)
{
    // StrongFocus accepts clicks and Tab navigation, but wheel hovering alone
    // must not move focus away from the surrounding scroll area.
    setFocusPolicy(Qt::StrongFocus);
    if (lineEdit() != nullptr) {
        lineEdit()->installEventFilter(this);
    }
}

bool FocusAwareSpinBox::eventFilter(QObject *watched, QEvent *event)
{
    const bool isEditorEvent = watched == lineEdit();
    const QEvent::Type type = event->type();
    const bool mayMoveCursor =
        type == QEvent::MouseButtonPress
        || type == QEvent::MouseButtonRelease
        || type == QEvent::MouseButtonDblClick
        || type == QEvent::MouseMove
        || type == QEvent::KeyPress
        || type == QEvent::KeyRelease
        || type == QEvent::FocusIn
        || type == QEvent::InputMethod;
    if (isEditorEvent && mayMoveCursor) {
        QTimer::singleShot(0, this, [this] { clampEditorCursor(); });
    }
    return QSpinBox::eventFilter(watched, event);
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

void FocusAwareSpinBox::clampEditorCursor()
{
    QLineEdit *editor = lineEdit();
    if (editor == nullptr) {
        return;
    }

    const int firstEditablePosition = prefix().size();
    const int lastEditablePosition =
        qMax(firstEditablePosition, editor->text().size() - suffix().size());
    if (editor->hasSelectedText()) {
        const int selectionStart = editor->selectionStart();
        const int selectionEnd = selectionStart + editor->selectedText().size();
        const int clampedStart = qBound(firstEditablePosition,
                                        selectionStart,
                                        lastEditablePosition);
        const int clampedEnd = qBound(firstEditablePosition,
                                      selectionEnd,
                                      lastEditablePosition);
        if (clampedEnd > clampedStart) {
            editor->setSelection(clampedStart, clampedEnd - clampedStart);
            return;
        }
    }
    editor->deselect();
    editor->setCursorPosition(qBound(firstEditablePosition,
                                     editor->cursorPosition(),
                                     lastEditablePosition));
}
