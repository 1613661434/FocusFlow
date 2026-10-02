#pragma once

class QMenu;
class QPoint;

namespace TextOnlyMenu {

void apply(QMenu *menu, bool leftClickOnly = false);
void popup(QMenu *menu, const QPoint &globalPosition);

} // namespace TextOnlyMenu
