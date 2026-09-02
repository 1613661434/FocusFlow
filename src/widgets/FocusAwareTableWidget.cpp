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
    // 表格一旦获得焦点，就始终消费滚轮事件。即使已经到达顶部或底部，
    // 也不把事件继续传给外层设置页面，避免一次滚动同时移动两层内容。
    event->accept();
}
