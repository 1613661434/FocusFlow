#pragma once

#include <QColor>
#include <QComboBox>
#include <QPalette>
#include <QVariant>

namespace ColoredComboBox {

inline QColor itemColor(const QComboBox *comboBox, int index)
{
    const QColor color = comboBox->itemData(index, Qt::ForegroundRole).value<QColor>();
    return color.isValid() ? color : QColor(QStringLiteral("#182230"));
}

inline void applyCurrentItemColor(QComboBox *comboBox)
{
    QPalette palette = comboBox->palette();
    const QColor color = itemColor(comboBox, comboBox->currentIndex());
    palette.setColor(QPalette::Text, color);
    palette.setColor(QPalette::ButtonText, color);
    comboBox->setPalette(palette);
}

inline void enableCurrentItemColor(QComboBox *comboBox)
{
    QObject::connect(comboBox,
                     &QComboBox::currentIndexChanged,
                     comboBox,
                     [comboBox] { applyCurrentItemColor(comboBox); });
    applyCurrentItemColor(comboBox);
}

inline void addColoredItem(QComboBox *comboBox,
                           const QString &text,
                           const QVariant &data,
                           const QColor &color)
{
    comboBox->addItem(text, data);
    if (color.isValid()) {
        comboBox->setItemData(comboBox->count() - 1, color, Qt::ForegroundRole);
    }
}

} // namespace ColoredComboBox
