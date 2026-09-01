#include "widgets/FocusAwareSpinBox.h"

#include <QApplication>
#include <QLineEdit>
#include <QTest>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QWidget>

class FocusAwareSpinBoxTests final : public QObject
{
    Q_OBJECT

private slots:
    void ignoresWheelUntilFocused();
    void keepsCursorOutOfSuffix();
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

QTEST_MAIN(FocusAwareSpinBoxTests)

#include "tst_FocusAwareSpinBox.moc"
