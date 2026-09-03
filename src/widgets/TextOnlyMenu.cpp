#include "widgets/TextOnlyMenu.h"

#include <QAction>
#include <QMenu>
#include <QPainterPath>
#include <QRegion>
#include <QTimer>

#ifdef Q_OS_WIN
#include <QtCore/qt_windows.h>
#include <dwmapi.h>
#endif

namespace {

void polishPopupSurface(QMenu *menu)
{
    if (menu == nullptr || menu->width() <= 1 || menu->height() <= 1) {
        return;
    }

    QPainterPath roundedSurface;
    roundedSurface.addRoundedRect(
        QRectF(menu->rect()).adjusted(0.0, 0.0, -1.0, -1.0),
        7.0,
        7.0);
    menu->setMask(QRegion(roundedSurface.toFillPolygon().toPolygon()));

#ifdef Q_OS_WIN
    const HWND handle = reinterpret_cast<HWND>(menu->winId());
    const DWMNCRENDERINGPOLICY policy = DWMNCRP_DISABLED;
    const HRESULT result = DwmSetWindowAttribute(
        handle,
        DWMWA_NCRENDERING_POLICY,
        &policy,
        sizeof(policy));
    menu->setProperty("focusFlowNativeFrameDisabled", SUCCEEDED(result));
#else
    menu->setProperty("focusFlowNativeFrameDisabled", true);
#endif
}

} // namespace

namespace TextOnlyMenu {

void apply(QMenu *menu)
{
    if (menu == nullptr) {
        return;
    }

    for (QAction *action : menu->actions()) {
        action->setIconVisibleInMenu(false);
    }
    // Qt styles clip the painted background, while a native popup can still
    // retain an outer frame. Shape the real window and suppress that frame.
    menu->setWindowFlag(Qt::FramelessWindowHint, true);
    menu->setWindowFlag(Qt::NoDropShadowWindowHint, true);
    menu->setAttribute(Qt::WA_TranslucentBackground, true);
    menu->setToolTipsVisible(true);
    menu->setStyleSheet(QStringLiteral(R"(
        QMenu {
            color: #182230;
            background: #ffffff;
            border: 1px solid #dce2ec;
            border-radius: 7px;
            padding: 4px;
        }
        QMenu::item {
            color: #182230;
            background: transparent;
            border-radius: 4px;
            margin: 0;
            padding: 6px 14px;
        }
        QMenu::item:selected:enabled {
            color: #2f49c7;
            background: #eef2ff;
        }
        QMenu::item:disabled {
            color: #98a2b3;
            background: transparent;
        }
        QMenu::indicator {
            width: 0;
            height: 0;
        }
    )"));

    QObject::connect(menu, &QMenu::aboutToShow, menu, [menu] {
        polishPopupSurface(menu);
        QTimer::singleShot(0, menu, [menu] { polishPopupSurface(menu); });
    });
}

void popup(QMenu *menu, const QPoint &globalPosition)
{
    if (menu == nullptr) {
        return;
    }

    apply(menu);
    menu->popup(globalPosition);
    polishPopupSurface(menu);
}

} // namespace TextOnlyMenu
