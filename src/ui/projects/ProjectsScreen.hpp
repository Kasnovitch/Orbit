#pragma once

#include <QWidget>

#include "src/data/auth/AuthService.hpp"

class QFrame;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QPushButton;
class QVBoxLayout;

// day 3: the projects page - list, add, edit, archive/restore, delete.
// mutations are permission-checked in ProjectService (owner/admin/write
// grant); the row buttons simply hide when you can't touch the project
class ProjectsScreen : public QWidget
{
    Q_OBJECT
public:
    explicit ProjectsScreen(const AuthUser& user, QWidget* parent = nullptr);

    void reload();

signals:
    // fired after any change so the project selector can refresh
    void projectsChanged();

private:
    void addProject();
    void editProject(qint64 projectId, const QString& name, const QString& description);
    void toggleArchive(qint64 projectId, bool archived);
    void removeProject(qint64 projectId, const QString& name);
    void showError(const QString& message);

    AuthUser m_user;
    QLineEdit* m_nameInput = nullptr;
    QLineEdit* m_descriptionInput = nullptr;
    QVBoxLayout* m_listLayout = nullptr;
    QLabel* m_statusLabel = nullptr;
};
