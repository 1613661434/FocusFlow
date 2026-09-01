#pragma once

#include "models/Project.h"
#include "models/Task.h"
#include "repositories/ProjectRepository.h"

#include <QWidget>

class QTableWidget;

class ProjectPage final : public QWidget
{
    Q_OBJECT

public:
    explicit ProjectPage(QWidget *parent = nullptr);

signals:
    void lookupsChanged();

public slots:
    void refresh();

private slots:
    void addProject();
    void editProject();
    void toggleProjectArchive();
    void deleteProject();
    void addCategory();
    void editCategory();
    void deleteCategory();

private:
    void buildInterface();
    bool editProjectValues(Project &project);
    bool editCategoryValues(LookupItem &category);
    int selectedProjectIndex() const;
    int selectedCategoryIndex() const;
    void showError(const QString &action, const QString &details);

    ProjectRepository repository_;
    QVector<Project> projects_;
    QVector<LookupItem> categories_;
    QTableWidget *projectTable_ = nullptr;
    QTableWidget *categoryTable_ = nullptr;
};
