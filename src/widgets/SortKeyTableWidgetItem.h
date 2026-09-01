#pragma once

#include <QMetaType>
#include <QTableWidgetItem>
#include <QVariant>

class SortKeyTableWidgetItem : public QTableWidgetItem
{
public:
    static constexpr int SortRole = Qt::UserRole + 1;

    explicit SortKeyTableWidgetItem(const QString &text = {},
                                    const QVariant &sortKey = {})
        : QTableWidgetItem(text)
    {
        setData(SortRole, sortKey.isValid() ? sortKey : text.toCaseFolded());
    }

    bool operator<(const QTableWidgetItem &other) const override
    {
        const QVariant left = data(SortRole);
        const QVariant right = other.data(SortRole);
        if (isNumeric(left) && isNumeric(right)) {
            return left.toDouble() < right.toDouble();
        }
        return QString::localeAwareCompare(left.toString(), right.toString()) < 0;
    }

private:
    static bool isNumeric(const QVariant &value)
    {
        switch (value.typeId()) {
        case QMetaType::Int:
        case QMetaType::UInt:
        case QMetaType::LongLong:
        case QMetaType::ULongLong:
        case QMetaType::Double:
        case QMetaType::Float:
            return true;
        default:
            return false;
        }
    }
};
