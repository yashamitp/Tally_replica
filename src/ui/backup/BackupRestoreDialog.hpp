#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTabWidget>
#include <string>
#include "../../database/DatabaseManager.hpp"

namespace tally::ui {

class BackupRestoreDialog : public QDialog {
    Q_OBJECT

public:
    explicit BackupRestoreDialog(
        database::DatabaseManager& dbManager,
        QWidget* parent = nullptr
    );

    bool databaseRestored() const { return m_restored; }

private slots:
    void onBrowseBackupDest();
    void onBrowseRestoreSrc();
    void onExecuteBackup();
    void onExecuteRestore();

private:
    void setupUi();

    database::DatabaseManager& m_dbManager;
    bool m_restored = false;

    QTabWidget* m_tabWidget = nullptr;

    // Backup Tab
    QLineEdit* m_backupDestEdit = nullptr;
    QPushButton* m_backupBtn = nullptr;

    // Restore Tab
    QLineEdit* m_restoreSrcEdit = nullptr;
    QPushButton* m_restoreBtn = nullptr;
};

} // namespace tally::ui
