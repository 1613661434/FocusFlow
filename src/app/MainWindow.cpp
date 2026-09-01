#include "app/MainWindow.h"

#include "views/TaskPage.h"
#include "views/FocusPage.h"
#include "views/ProjectPage.h"
#include "views/SettingsPage.h"

#include <QAction>
#include <QApplication>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QStackedWidget>
#include <QStyle>
#include <QSystemTrayIcon>
#include <QTimer>
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
    setupTray();
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
    auto *taskPage = new TaskPage(pages_);
    pages_->addWidget(taskPage);
    auto *projectPage = new ProjectPage(pages_);
    pages_->addWidget(projectPage);
    auto *focusPage = new FocusPage(pages_);
    pages_->addWidget(focusPage);
    pages_->addWidget(createPlaceholderPage(kPageNames.at(4), kPageDescriptions.at(4)));
    auto *settingsPage = new SettingsPage(pages_);
    pages_->addWidget(settingsPage);
    contentLayout->addWidget(pages_, 1);

    rootLayout->addWidget(sidebar);
    rootLayout->addWidget(content, 1);
    setCentralWidget(central);

    connect(navigation_, &QListWidget::currentRowChanged,
            this, &MainWindow::showPage);
    connect(taskPage, &TaskPage::tasksChanged,
            focusPage, &FocusPage::refreshTasks);
    connect(projectPage, &ProjectPage::lookupsChanged,
            taskPage, &TaskPage::refresh);
    connect(settingsPage, &SettingsPage::settingsSaved,
            focusPage, &FocusPage::reloadSettings);
    connect(focusPage, &FocusPage::notificationRequested,
            this, &MainWindow::showNotification);
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
        QLabel#timerLabel {
            color: #172033;
            font-size: 64px;
            font-weight: 700;
        }
        QGroupBox {
            background: #ffffff;
            border: 1px solid #e5e9f1;
            border-radius: 12px;
            margin-top: 16px;
            padding: 18px;
            font-weight: 650;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 16px;
            padding: 0 6px;
        }
        QProgressBar {
            background: #e8ecf5;
            border: none;
            border-radius: 5px;
        }
        QProgressBar::chunk {
            background: #4f6ef7;
            border-radius: 5px;
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

void MainWindow::setupTray()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        return;
    }

    trayIcon_ = new QSystemTrayIcon(
        style()->standardIcon(QStyle::SP_ComputerIcon), this);
    trayIcon_->setToolTip(QStringLiteral("FocusFlow"));

    auto *menu = new QMenu(this);
    auto *showAction = menu->addAction(QStringLiteral("显示 FocusFlow"));
    auto *quitAction = menu->addAction(QStringLiteral("退出"));
    connect(showAction, &QAction::triggered, this, &MainWindow::showFromTray);
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);
    connect(trayIcon_, &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason reason) {
                if (reason == QSystemTrayIcon::DoubleClick
                    || reason == QSystemTrayIcon::Trigger) {
                    showFromTray();
                }
            });
    trayIcon_->setContextMenu(menu);
    trayIcon_->show();
}

void MainWindow::showFromTray()
{
    showNormal();
    raise();
    activateWindow();
}

void MainWindow::showNotification(const QString &title, const QString &message)
{
    if (trayIcon_ != nullptr && trayIcon_->supportsMessages()) {
        trayIcon_->showMessage(title,
                               message,
                               QSystemTrayIcon::Information,
                               8000);
    }
}

void MainWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::WindowStateChange
        && isMinimized()
        && trayIcon_ != nullptr) {
        QTimer::singleShot(0, this, &QWidget::hide);
        trayIcon_->showMessage(QStringLiteral("FocusFlow 仍在运行"),
                               QStringLiteral("计时和提醒将继续工作。"),
                               QSystemTrayIcon::Information,
                               3000);
    }
}
