#include "views/AboutDialog.h"

#include <QCoreApplication>
#include <QLabel>
#include <QPushButton>
#include <QTest>

class AboutDialogTests final : public QObject
{
    Q_OBJECT

private slots:
    void exposesCopyrightAndAuthorInformation();
};

void AboutDialogTests::exposesCopyrightAndAuthorInformation()
{
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.19"));
    AboutDialog dialog;

    QCOMPARE(dialog.windowTitle(), QStringLiteral("关于 FocusFlow"));
    auto *author = dialog.findChild<QLabel *>(
        QStringLiteral("aboutAuthorName"));
    auto *shortName = dialog.findChild<QLabel *>(
        QStringLiteral("aboutAuthorShortName"));
    auto *blog = dialog.findChild<QLabel *>(
        QStringLiteral("aboutAuthorBlog"));
    auto *version = dialog.findChild<QLabel *>(
        QStringLiteral("aboutVersion"));
    auto *repository = dialog.findChild<QLabel *>(
        QStringLiteral("aboutProjectRepository"));
    auto *copyright = dialog.findChild<QLabel *>(
        QStringLiteral("aboutCopyright"));
    auto *blogButton = dialog.findChild<QPushButton *>(
        QStringLiteral("aboutBlogButton"));
    auto *repositoryButton = dialog.findChild<QPushButton *>(
        QStringLiteral("aboutRepositoryButton"));
    QVERIFY(author != nullptr);
    QVERIFY(shortName != nullptr);
    QVERIFY(blog != nullptr);
    QVERIFY(version != nullptr);
    QVERIFY(repository != nullptr);
    QVERIFY(copyright != nullptr);
    QVERIFY(blogButton != nullptr);
    QVERIFY(repositoryButton != nullptr);
    QCOMPARE(author->text(), QStringLiteral("ol木子李lo"));
    QCOMPARE(shortName->text(), QStringLiteral("OL"));
    QCOMPARE(blog->property("authorBlogUrl").toString(),
             QStringLiteral("https://1613661434.github.io/"));
    QCOMPARE(blogButton->property("authorBlogUrl").toString(),
             QStringLiteral("https://1613661434.github.io/"));
    QCOMPARE(repository->property("projectRepositoryUrl").toString(),
             QStringLiteral("https://github.com/1613661434/FocusFlow"));
    QCOMPARE(repositoryButton->property("projectRepositoryUrl").toString(),
             QStringLiteral("https://github.com/1613661434/FocusFlow"));
    QVERIFY(version->text().contains(QStringLiteral("0.1.19")));
    QVERIFY(copyright->text().contains(QStringLiteral("© 2026")));
    QVERIFY(copyright->text().contains(QStringLiteral("ol木子李lo")));
}

QTEST_MAIN(AboutDialogTests)

#include "tst_AboutDialog.moc"
