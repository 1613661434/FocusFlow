#include <QApplication>
#include <QColorDialog>
#include <QDialogButtonBox>
#include <QLibraryInfo>
#include <QPushButton>
#include <QTest>
#include <QTranslator>

class QtLocalizationTests final : public QObject
{
    Q_OBJECT

private slots:
    void standardDialogButtonsUseSimplifiedChinese();
};

void QtLocalizationTests::standardDialogButtonsUseSimplifiedChinese()
{
    QTranslator translator;
    QVERIFY(translator.load(
        QStringLiteral("qtbase_zh_CN"),
        QLibraryInfo::path(QLibraryInfo::TranslationsPath)));
    QVERIFY(qApp->installTranslator(&translator));

    QDialogButtonBox saveButtons(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    QVERIFY(saveButtons.button(QDialogButtonBox::Save)
                ->text()
                .contains(QStringLiteral("保存")));
    QVERIFY(saveButtons.button(QDialogButtonBox::Cancel)
                ->text()
                .contains(QStringLiteral("取消")));

    QColorDialog colorDialog;
    colorDialog.setOption(QColorDialog::DontUseNativeDialog, true);
    auto *colorButtons = colorDialog.findChild<QDialogButtonBox *>();
    QVERIFY(colorButtons != nullptr);
    QVERIFY(colorButtons->button(QDialogButtonBox::Ok)
                ->text()
                .contains(QStringLiteral("确定")));
    QVERIFY(colorButtons->button(QDialogButtonBox::Cancel)
                ->text()
                .contains(QStringLiteral("取消")));

    qApp->removeTranslator(&translator);
}

QTEST_MAIN(QtLocalizationTests)

#include "tst_QtLocalization.moc"
