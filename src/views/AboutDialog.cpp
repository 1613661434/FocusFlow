#include "views/AboutDialog.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QSizePolicy>
#include <QUrl>
#include <QVBoxLayout>

namespace {
const QUrl kAuthorBlog(QStringLiteral("https://1613661434.github.io/"));
}

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
{
    setObjectName(QStringLiteral("aboutDialog"));
    setWindowTitle(QStringLiteral("关于 FocusFlow"));
    setWindowIcon(QIcon(QStringLiteral(":/icons/focusflow.svg")));
    setModal(true);
    setMinimumSize(560, 470);
    resize(600, 500);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(28, 28, 28, 24);
    root->setSpacing(18);

    auto *card = new QFrame(this);
    card->setObjectName(QStringLiteral("aboutCard"));
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(28, 26, 28, 26);
    cardLayout->setSpacing(18);

    auto *brandRow = new QHBoxLayout;
    brandRow->setSpacing(16);
    auto *iconLabel = new QLabel(card);
    iconLabel->setObjectName(QStringLiteral("aboutAppIcon"));
    iconLabel->setAccessibleName(QStringLiteral("FocusFlow 应用图标"));
    iconLabel->setPixmap(
        QIcon(QStringLiteral(":/icons/focusflow.svg")).pixmap(68, 68));
    iconLabel->setFixedSize(68, 68);

    auto *brandText = new QVBoxLayout;
    brandText->setSpacing(4);
    auto *title = new QLabel(QStringLiteral("FocusFlow"), card);
    title->setObjectName(QStringLiteral("aboutAppTitle"));
    auto *subtitle = new QLabel(
        QStringLiteral("个人任务规划与专注管理系统"), card);
    subtitle->setObjectName(QStringLiteral("aboutSubtitle"));
    auto *version = new QLabel(
        QStringLiteral("版本 %1").arg(QCoreApplication::applicationVersion()),
        card);
    version->setObjectName(QStringLiteral("aboutVersion"));
    brandText->addWidget(title);
    brandText->addWidget(subtitle);
    brandText->addWidget(version);
    brandRow->addWidget(iconLabel);
    brandRow->addLayout(brandText, 1);
    cardLayout->addLayout(brandRow);

    auto *divider = new QFrame(card);
    divider->setObjectName(QStringLiteral("aboutDivider"));
    divider->setFrameShape(QFrame::HLine);
    divider->setFixedHeight(1);
    cardLayout->addWidget(divider);

    auto *details = new QGridLayout;
    details->setHorizontalSpacing(22);
    details->setVerticalSpacing(12);
    auto addDetail = [details, card](int row,
                                     const QString &labelText,
                                     QLabel *value) {
        auto *label = new QLabel(labelText, card);
        label->setObjectName(QStringLiteral("aboutFieldLabel"));
        details->addWidget(label, row, 0, Qt::AlignTop);
        details->addWidget(value, row, 1);
    };

    auto *author = new QLabel(QStringLiteral("ol木子李lo"), card);
    author->setObjectName(QStringLiteral("aboutAuthorName"));
    auto *shortName = new QLabel(QStringLiteral("OL"), card);
    shortName->setObjectName(QStringLiteral("aboutAuthorShortName"));
    auto *blog = new QLabel(
        QStringLiteral("<a href=\"%1\">%1</a>").arg(kAuthorBlog.toString()),
        card);
    blog->setObjectName(QStringLiteral("aboutAuthorBlog"));
    blog->setProperty("authorBlogUrl", kAuthorBlog.toString());
    blog->setOpenExternalLinks(true);
    blog->setTextInteractionFlags(Qt::LinksAccessibleByMouse
                                  | Qt::LinksAccessibleByKeyboard);
    blog->setFocusPolicy(Qt::StrongFocus);
    addDetail(0, QStringLiteral("作者"), author);
    addDetail(1, QStringLiteral("简称"), shortName);
    addDetail(2, QStringLiteral("作者博客"), blog);
    details->setColumnStretch(1, 1);
    cardLayout->addLayout(details);

    auto *copyright = new QLabel(
        QStringLiteral("版权所有 © 2026 ol木子李lo（OL）\n"
                       "Copyright © 2026 OL. All rights reserved."),
        card);
    copyright->setObjectName(QStringLiteral("aboutCopyright"));
    copyright->setAlignment(Qt::AlignCenter);
    copyright->setWordWrap(true);
    cardLayout->addWidget(copyright);

    auto *frameworkNotice = new QLabel(
        QStringLiteral("FocusFlow 由 OL 设计与开发。本程序使用 Qt 开源框架构建，"
                       "相关第三方组件版权归其各自权利人所有。"),
        card);
    frameworkNotice->setObjectName(QStringLiteral("aboutNotice"));
    frameworkNotice->setAlignment(Qt::AlignCenter);
    frameworkNotice->setWordWrap(true);
    cardLayout->addWidget(frameworkNotice);

    root->addWidget(card, 1);

    auto *buttons = new QHBoxLayout;
    buttons->setSpacing(10);
    buttons->addStretch();
    auto *blogButton = new QPushButton(QStringLiteral("访问作者博客"), this);
    blogButton->setObjectName(QStringLiteral("aboutBlogButton"));
    blogButton->setProperty("authorBlogUrl", kAuthorBlog.toString());
    blogButton->setAccessibleName(QStringLiteral("在浏览器中访问作者博客"));
    blogButton->setMinimumHeight(40);
    auto *closeButton = new QPushButton(QStringLiteral("关闭"), this);
    closeButton->setObjectName(QStringLiteral("primaryButton"));
    closeButton->setMinimumSize(96, 40);
    closeButton->setDefault(true);
    buttons->addWidget(blogButton);
    buttons->addWidget(closeButton);
    root->addLayout(buttons);

    connect(blogButton, &QPushButton::clicked, this, [] {
        QDesktopServices::openUrl(kAuthorBlog);
    });
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    setStyleSheet(QStringLiteral(R"(
        QDialog#aboutDialog {
            background: #f6f8fc;
            color: #172033;
        }
        QFrame#aboutCard {
            background: #ffffff;
            border: 1px solid #e1e7f0;
            border-radius: 14px;
        }
        QLabel#aboutAppIcon {
            background: transparent;
            border: none;
        }
        QLabel#aboutAppTitle {
            color: #172033;
            font-size: 25px;
            font-weight: 700;
        }
        QLabel#aboutSubtitle {
            color: #344054;
            font-size: 15px;
        }
        QLabel#aboutVersion, QLabel#aboutNotice {
            color: #667085;
        }
        QFrame#aboutDivider {
            background: #e7eaf0;
            border: none;
        }
        QLabel#aboutFieldLabel {
            color: #667085;
            font-weight: 600;
        }
        QLabel#aboutAuthorName, QLabel#aboutAuthorShortName {
            color: #172033;
            font-weight: 600;
        }
        QLabel#aboutAuthorBlog {
            color: #405dde;
        }
        QLabel#aboutCopyright {
            color: #344054;
            font-weight: 600;
            line-height: 1.5;
        }
    )"));
}
