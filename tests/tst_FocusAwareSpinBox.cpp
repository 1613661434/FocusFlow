#include "widgets/FocusAwareComboBox.h"
#include "widgets/FocusAwareSpinBox.h"
#include "widgets/FocusAwareSlider.h"
#include "widgets/FocusAwareTableWidget.h"
#include "widgets/ColoredComboBox.h"

#include <QApplication>
#include <QColor>
#include <QComboBox>
#include <QImage>
#include <QLineEdit>
#include <QScrollBar>
#include <QTableWidgetItem>
#include <QTest>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QWidget>

class FocusAwareSpinBoxTests final : public QObject
{
    Q_OBJECT

private slots:
    void ignoresWheelUntilFocused();
    void sliderIgnoresWheelUntilFocused();
    void comboBoxIgnoresWheelUntilFocused();
    void tableIgnoresWheelUntilClicked();
    void keepsCursorOutOfSuffix();
    void coloredComboBoxPaintsCurrentItemColor();
};

namespace {
QWheelEvent upwardWheelEvent()
{
    return QWheelEvent(QPointF(10, 10),
                       QPointF(10, 10),
                       QPoint(),
                       QPoint(0, 120),
                       Qt::NoButton,
                       Qt::NoModifier,
                       Qt::NoScrollPhase,
                       false);
}

QWheelEvent downwardWheelEvent()
{
    return QWheelEvent(QPointF(10, 10),
                       QPointF(10, 10),
                       QPoint(),
                       QPoint(0, -120),
                       Qt::NoButton,
                       Qt::NoModifier,
                       Qt::NoScrollPhase,
                       false);
}
}

void FocusAwareSpinBoxTests::ignoresWheelUntilFocused()
{
    QWidget window;
    auto *layout = new QVBoxLayout(&window);
    auto *otherInput = new QLineEdit(&window);
    auto *spinBox = new FocusAwareSpinBox(&window);
    spinBox->setRange(0, 100);
    spinBox->setValue(10);
    layout->addWidget(otherInput);
    layout->addWidget(spinBox);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    otherInput->setFocus();
    QTRY_VERIFY(otherInput->hasFocus());
    auto ignoredWheel = upwardWheelEvent();
    QApplication::sendEvent(spinBox, &ignoredWheel);
    QCOMPARE(spinBox->value(), 10);
    QVERIFY(!ignoredWheel.isAccepted());

    spinBox->setFocus(Qt::MouseFocusReason);
    QTRY_VERIFY(spinBox->hasFocus());
    auto acceptedWheel = upwardWheelEvent();
    QApplication::sendEvent(spinBox, &acceptedWheel);
    QCOMPARE(spinBox->value(), 11);
    QVERIFY(acceptedWheel.isAccepted());
}

void FocusAwareSpinBoxTests::keepsCursorOutOfSuffix()
{
    FocusAwareSpinBox spinBox;
    spinBox.setRange(0, 100);
    spinBox.setValue(25);
    spinBox.setSuffix(QStringLiteral(" 分钟"));
    spinBox.resize(180, 40);
    spinBox.show();
    QVERIFY(QTest::qWaitForWindowExposed(&spinBox));

    auto *editor = spinBox.findChild<QLineEdit *>();
    QVERIFY(editor != nullptr);
    editor->setFocus();
    QTest::mouseClick(editor,
                      Qt::LeftButton,
                      Qt::NoModifier,
                      QPoint(editor->width() - 2, editor->height() / 2));
    QCoreApplication::processEvents();

    const int numericEnd = editor->text().size() - spinBox.suffix().size();
    QVERIFY(editor->cursorPosition() <= numericEnd);

    editor->selectAll();
    QTest::keyClick(editor, Qt::Key_End);
    QCoreApplication::processEvents();
    QCOMPARE(editor->cursorPosition(), numericEnd);
}

void FocusAwareSpinBoxTests::sliderIgnoresWheelUntilFocused()
{
    QWidget window;
    auto *layout = new QVBoxLayout(&window);
    auto *otherInput = new QLineEdit(&window);
    auto *slider = new FocusAwareSlider(Qt::Horizontal, &window);
    slider->setRange(0, 100);
    slider->setValue(10);
    layout->addWidget(otherInput);
    layout->addWidget(slider);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    otherInput->setFocus();
    QTRY_VERIFY(otherInput->hasFocus());
    auto ignoredWheel = upwardWheelEvent();
    QApplication::sendEvent(slider, &ignoredWheel);
    QCOMPARE(slider->value(), 10);
    QVERIFY(!ignoredWheel.isAccepted());

    slider->setFocus(Qt::MouseFocusReason);
    QTRY_VERIFY(slider->hasFocus());
    auto acceptedWheel = upwardWheelEvent();
    QApplication::sendEvent(slider, &acceptedWheel);
    QVERIFY(slider->value() > 10);
    QVERIFY(acceptedWheel.isAccepted());
}

void FocusAwareSpinBoxTests::comboBoxIgnoresWheelUntilFocused()
{
    QWidget window;
    auto *layout = new QVBoxLayout(&window);
    auto *otherInput = new QLineEdit(&window);
    auto *comboBox = new FocusAwareComboBox(&window);
    comboBox->addItems({QStringLiteral("第一项"),
                        QStringLiteral("第二项"),
                        QStringLiteral("第三项")});
    layout->addWidget(otherInput);
    layout->addWidget(comboBox);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    otherInput->setFocus();
    QTRY_VERIFY(otherInput->hasFocus());
    auto ignoredWheel = downwardWheelEvent();
    QApplication::sendEvent(comboBox, &ignoredWheel);
    QCOMPARE(comboBox->currentIndex(), 0);
    QVERIFY(!ignoredWheel.isAccepted());

    QTest::mouseClick(comboBox, Qt::LeftButton);
    QTRY_VERIFY(comboBox->hasFocus());
    comboBox->hidePopup();
    auto acceptedWheel = downwardWheelEvent();
    QApplication::sendEvent(comboBox, &acceptedWheel);
    QCOMPARE(comboBox->currentIndex(), 1);
    QVERIFY(acceptedWheel.isAccepted());
}

void FocusAwareSpinBoxTests::tableIgnoresWheelUntilClicked()
{
    QWidget window;
    auto *layout = new QVBoxLayout(&window);
    auto *otherInput = new QLineEdit(&window);
    auto *table = new FocusAwareTableWidget(&window);
    table->setColumnCount(1);
    table->setRowCount(10);
    table->setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
    table->setFixedHeight(150);
    for (int row = 0; row < table->rowCount(); ++row) {
        table->setItem(row, 0,
                       new QTableWidgetItem(QString::number(row + 1)));
    }
    layout->addWidget(otherInput);
    layout->addWidget(table);
    window.resize(320, 260);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QVERIFY(table->verticalScrollBar()->maximum() > 0);

    otherInput->setFocus();
    QTRY_VERIFY(otherInput->hasFocus());
    auto ignoredWheel = downwardWheelEvent();
    QApplication::sendEvent(table->viewport(), &ignoredWheel);
    QCOMPARE(table->verticalScrollBar()->value(), 0);
    QVERIFY(!ignoredWheel.isAccepted());

    QTest::mouseClick(table->viewport(), Qt::LeftButton,
                      Qt::NoModifier, QPoint(20, 20));
    QTRY_VERIFY(table->hasFocus());
    auto acceptedWheel = downwardWheelEvent();
    QApplication::sendEvent(table->viewport(), &acceptedWheel);
    QVERIFY(table->verticalScrollBar()->value() > 0);
    QVERIFY(acceptedWheel.isAccepted());

    table->verticalScrollBar()->setValue(
        table->verticalScrollBar()->maximum());
    const int bottomValue = table->verticalScrollBar()->value();
    auto boundaryWheel = downwardWheelEvent();
    QApplication::sendEvent(table->viewport(), &boundaryWheel);
    QCOMPARE(table->verticalScrollBar()->value(), bottomValue);
    QVERIFY(boundaryWheel.isAccepted());
}

void FocusAwareSpinBoxTests::coloredComboBoxPaintsCurrentItemColor()
{
    QWidget window;
    auto *layout = new QVBoxLayout(&window);
    auto *comboBox = new QComboBox(&window);
    comboBox->setObjectName(QStringLiteral("coloredComboBox"));
    comboBox->addItem(QStringLiteral("全部项目"), -2);
    ColoredComboBox::addColoredItem(comboBox,
                                    QStringLiteral("彩色项目"),
                                    1,
                                    QColor(QStringLiteral("#e02020")));
    ColoredComboBox::enableCurrentItemColor(comboBox);
    layout->addWidget(comboBox);
    // FocusFlow applies its parent theme after pages are constructed. The
    // selected entity colour must retain precedence in that same order.
    window.setStyleSheet(QStringLiteral(
        "QWidget { color: #182230; font-size: 20px; }"
        "QComboBox { background: #ffffff; padding: 7px 9px; }"));
    window.resize(280, 80);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    comboBox->setCurrentIndex(1);
    QCoreApplication::processEvents();
    QImage image(comboBox->size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    comboBox->render(&image);

    int redTextPixels = 0;
    int redArrowPixels = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor pixel = image.pixelColor(x, y);
            if (pixel.red() > 150 && pixel.green() < 110
                && pixel.blue() < 110 && pixel.alpha() > 0) {
                if (x < image.width() - 40) {
                    ++redTextPixels;
                } else {
                    ++redArrowPixels;
                }
            }
        }
    }
    QVERIFY(redTextPixels > 5);
    QCOMPARE(redArrowPixels, 0);
    QCOMPARE(comboBox->itemData(0, Qt::ForegroundRole).value<QColor>(),
             QColor(QStringLiteral("#182230")));

    comboBox->setCurrentIndex(0);
    QCOMPARE(comboBox->palette().color(QPalette::Text),
             QColor(QStringLiteral("#182230")));
}

QTEST_MAIN(FocusAwareSpinBoxTests)

#include "tst_FocusAwareSpinBox.moc"
