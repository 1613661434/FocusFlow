#pragma once

#include <QAction>
#include <QMenu>

namespace TextOnlyMenu {

inline void apply(QMenu *menu)
{
    if (menu == nullptr) {
        return;
    }

    for (QAction *action : menu->actions()) {
        action->setIconVisibleInMenu(false);
    }
    // Let Qt paint the complete rounded popup surface.  On Windows, keeping
    // the native popup frame can leave a dark one-pixel edge at the bottom
    // and right even when the drop shadow hint is disabled.
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
}

} // namespace TextOnlyMenu
