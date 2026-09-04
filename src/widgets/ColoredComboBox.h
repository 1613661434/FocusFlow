#pragma once

#include <QColor>
#include <QComboBox>
#include <QPalette>
#include <QVariant>

namespace ColoredComboBox {

inline constexpr auto kBaseStyleProperty =
    "_focusFlowColoredComboBoxBaseStyle";

inline QColor itemColor(const QComboBox *comboBox, int index)
{
    const QColor color = comboBox->itemData(index, Qt::ForegroundRole).value<QColor>();
    return color.isValid() ? color : QColor(QStringLiteral("#182230"));
}

inline void applyCurrentItemColor(QComboBox *comboBox)
{
    // Keep popup placeholder items on the normal text colour. A local colour
    // rule is needed for the collapsed label because the application-wide
    // QWidget rule otherwise takes precedence over the widget palette.
    const QColor defaultColor(QStringLiteral("#182230"));
    for (int index = 0; index < comboBox->count(); ++index) {
        const QColor foreground =
            comboBox->itemData(index, Qt::ForegroundRole).value<QColor>();
        if (!foreground.isValid()) {
            comboBox->setItemData(index, defaultColor, Qt::ForegroundRole);
        }
    }

    QPalette palette = comboBox->palette();
    const QColor color = itemColor(comboBox, comboBox->currentIndex());
    palette.setColor(QPalette::Text, color);
    palette.setColor(QPalette::ButtonText, color);
    comboBox->setPalette(palette);

    if (!comboBox->property(kBaseStyleProperty).isValid()) {
        comboBox->setProperty(kBaseStyleProperty, comboBox->styleSheet());
    }
    const QString baseStyle =
        comboBox->property(kBaseStyleProperty).toString();
    comboBox->setStyleSheet(QStringLiteral(
        "%1\nQComboBox { color: %2; }\n"
        "QComboBox:disabled { color: #98a2b3; }\n"
        "QComboBox::down-arrow {"
        " image: url(:/icons/combo-down-arrow.svg);"
        " width: 10px; height: 6px;"
        "}\n"
        "QComboBox::down-arrow:disabled {"
        " image: url(:/icons/combo-down-arrow-disabled.svg);"
        "}")
                                .arg(baseStyle,
                                     color.name(QColor::HexRgb)));
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
