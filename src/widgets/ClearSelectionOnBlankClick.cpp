#include "widgets/ClearSelectionOnBlankClick.h"

#include <QAbstractItemView>
#include <QEvent>
#include <QMouseEvent>
#include <QWidget>

namespace {
class BlankClickFilter final : public QObject
{
public:
    explicit BlankClickFilter(QAbstractItemView *view)
        : QObject(view), view_(view)
    {
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (watched == view_->viewport()
            && event->type() == QEvent::MouseButtonPress) {
            auto *mouseEvent = static_cast<QMouseEvent *>(event);
            if (mouseEvent->button() == Qt::LeftButton
                && !view_->indexAt(mouseEvent->position().toPoint()).isValid()) {
                view_->clearSelection();
                view_->setCurrentIndex(QModelIndex());
            }
        }
        return QObject::eventFilter(watched, event);
    }

private:
    QAbstractItemView *view_ = nullptr;
};

class SurfaceClickFilter final : public QObject
{
public:
    SurfaceClickFilter(QWidget *surface, QAbstractItemView *view)
        : QObject(view), surface_(surface), view_(view)
    {
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (watched == surface_ && event->type() == QEvent::MouseButtonPress) {
            auto *mouseEvent = static_cast<QMouseEvent *>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                view_->clearSelection();
                view_->setCurrentIndex(QModelIndex());
            }
        }
        return QObject::eventFilter(watched, event);
    }

private:
    QWidget *surface_ = nullptr;
    QAbstractItemView *view_ = nullptr;
};
}

void enableClearSelectionOnBlankClick(QAbstractItemView *view)
{
    if (view == nullptr) {
        return;
    }
    view->viewport()->installEventFilter(new BlankClickFilter(view));
}

void enableClearSelectionOnClick(QWidget *surface, QAbstractItemView *view)
{
    if (surface == nullptr || view == nullptr) {
        return;
    }
    surface->installEventFilter(new SurfaceClickFilter(surface, view));
}
