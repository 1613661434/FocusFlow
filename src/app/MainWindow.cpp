#include "app/MainWindow.h"

#include "views/TaskPage.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>

namespace {
constexpr int kSidebarWidth = 220;

const QStringList kPageNames{
    QStringLiteral("今日概览"),
    QStringLiteral("任务"),
    QStringLiteral("项目"),
    QStringLiteral("专注计时"),
    QStringLiteral("数据统计"),
    QStringLiteral("设置"),
};

const QStringList kPageDescriptions{
    QStringLiteral("查看今日任务、截止时间和专注概况。"),
    QStringLiteral("记录、规划并跟踪你的工作、学习与生活任务。"),
    QStringLiteral("用项目和分类组织长期目标。"),
    QStringLiteral("将任务与可配置的专注和休息周期关联。"),
    QStringLiteral("分析任务完成率和专注时间。"),
    QStringLiteral("配置计时、提醒声音、主题和数据备份。"),
};
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("FocusFlow"));
    resize(1180, 760);
    setMinimumSize(920, 620);

    buildInterface();
    applyTheme();
}

void MainWindow::buildInterface()
{
    auto *central = new QWidget(this);
    auto *rootLayout = new QHBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto *sidebar = new QFrame(central);
    sidebar->setObjectName(QStringLiteral("sidebar"));
    sidebar->setFixedWidth(kSidebarWidth);
    auto *sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(18, 24, 18, 24);
    sidebarLayout->setSpacing(20);

    auto *brand = new QLabel(QStringLiteral("FocusFlow"), sidebar);
    brand->setObjectName(QStringLiteral("brand"));
    sidebarLayout->addWidget(brand);

    navigation_ = new QListWidget(sidebar);
    navigation_->setObjectName(QStringLiteral("navigation"));
    navigation_->addItems(kPageNames);
    navigation_->setCurrentRow(0);
    navigation_->setSpacing(4);
    sidebarLayout->addWidget(navigation_, 1);

    auto *version = new QLabel(QStringLiteral("v0.1.0"), sidebar);
    version->setObjectName(QStringLiteral("mutedLabel"));
    sidebarLayout->addWidget(version);

    auto *content = new QWidget(central);
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(36, 28, 36, 36);
    contentLayout->setSpacing(20);

    pageTitle_ = new QLabel(kPageNames.first(), content);
    pageTitle_->setObjectName(QStringLiteral("pageTitle"));
    contentLayout->addWidget(pageTitle_);

    pages_ = new QStackedWidget(content);
    pages_->addWidget(createPlaceholderPage(kPageNames.at(0), kPageDescriptions.at(0)));
    pages_->addWidget(new TaskPage(pages_));
    for (qsizetype index = 2; index < kPageNames.size(); ++index) {
        pages_->addWidget(createPlaceholderPage(kPageNames.at(index),
                                                kPageDescriptions.at(index)));
    }
    contentLayout->addWidget(pages_, 1);

    rootLayout->addWidget(sidebar);
    rootLayout->addWidget(content, 1);
    setCentralWidget(central);

    connect(navigation_, &QListWidget::currentRowChanged,
            this, &MainWindow::showPage);
}

QWidget *MainWindow::createPlaceholderPage(const QString &title,
                                           const QString &description)
{
    auto *page = new QFrame;
    page->setObjectName(QStringLiteral("card"));
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(32, 32, 32, 32);
    layout->setSpacing(12);

    auto *heading = new QLabel(title, page);
    heading->setObjectName(QStringLiteral("cardTitle"));
    auto *body = new QLabel(description, page);
    body->setObjectName(QStringLiteral("mutedLabel"));
    body->setWordWrap(true);

    layout->addWidget(heading);
    layout->addWidget(body);
    layout->addStretch();
    return page;
}

void MainWindow::showPage(int index)
{
    if (index < 0 || index >= pages_->count()) {
        return;
    }

    pages_->setCurrentIndex(index);
    pageTitle_->setText(kPageNames.at(index));
}

void MainWindow::applyTheme()
{
    setStyleSheet(QStringLiteral(R"(
        QMainWindow, QWidget {
            background: #f5f7fb;
            color: #182230;
            font-family: "Microsoft YaHei UI";
            font-size: 14px;
        }
        QFrame#sidebar {
            background: #172033;
        }
        QLabel#brand {
            color: #ffffff;
            font-size: 24px;
            font-weight: 700;
            padding: 4px 8px 14px 8px;
        }
        QListWidget#navigation {
            background: transparent;
            color: #aeb9cc;
            border: none;
            outline: none;
        }
        QListWidget#navigation::item {
            border-radius: 8px;
            padding: 12px 14px;
        }
        QListWidget#navigation::item:hover {
            background: #222e46;
            color: #ffffff;
        }
        QListWidget#navigation::item:selected {
            background: #4f6ef7;
            color: #ffffff;
        }
        QLabel#pageTitle {
            font-size: 26px;
            font-weight: 700;
        }
        QFrame#card {
            background: #ffffff;
            border: 1px solid #e5e9f1;
            border-radius: 14px;
        }
        QLabel#cardTitle {
            font-size: 20px;
            font-weight: 650;
        }
        QLabel#mutedLabel {
            color: #778196;
        }
        QLineEdit, QComboBox, QDateTimeEdit, QSpinBox, QTextEdit {
            background: #ffffff;
            border: 1px solid #dce2ec;
            border-radius: 7px;
            padding: 7px 9px;
            selection-background-color: #4f6ef7;
        }
        QLineEdit:focus, QComboBox:focus, QDateTimeEdit:focus,
        QSpinBox:focus, QTextEdit:focus {
            border-color: #4f6ef7;
        }
        QPushButton {
            background: #ffffff;
            border: 1px solid #dce2ec;
            border-radius: 7px;
            padding: 8px 14px;
        }
        QPushButton:hover {
            background: #f0f3f9;
        }
        QPushButton#primaryButton {
            background: #4f6ef7;
            border-color: #4f6ef7;
            color: #ffffff;
            font-weight: 600;
        }
        QPushButton#primaryButton:hover {
            background: #405dde;
        }
        QPushButton#dangerButton {
            color: #d84a4a;
        }
        QTableWidget {
            background: #ffffff;
            alternate-background-color: #f8f9fc;
            border: 1px solid #e5e9f1;
            border-radius: 10px;
            selection-background-color: #e9edff;
            selection-color: #182230;
        }
        QHeaderView::section {
            background: #f6f8fc;
            border: none;
            border-bottom: 1px solid #e5e9f1;
            padding: 10px;
            font-weight: 600;
        }
        QFrame#sidebar QLabel#mutedLabel {
            color: #71809b;
            padding-left: 8px;
        }
    )"));
}
