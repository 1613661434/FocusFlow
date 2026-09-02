#include "widgets/FocusAwareTableWidget.h"

#include <QWheelEvent>

FocusAwareTableWidget::FocusAwareTableWidget(QWidget *parent)
    : QTableWidget(parent)
{
    // 悬停只让外层页面滚动；点击或 Tab 选中表格后才滚动表格内容。
    setFocusPolicy(Qt::StrongFocus);
}

void FocusAwareTableWidget::wheelEvent(QWheelEvent *event)
{
    if (!hasFocus()) {
        event->ignore();
        return;
    }
    QTableWidget::wheelEvent(event);
}
