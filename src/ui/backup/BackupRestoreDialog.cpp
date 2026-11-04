#include "BackupRestoreDialog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFileDialog>
#include <QMessageBox>
#include "../../utilities/DateUtils.hpp"

namespace tally::ui {

BackupRestoreDialog::BackupRestoreDialog(
    database::DatabaseManager& dbManager,
    QWidget* parent
) : QDialog(parent),
    m_dbManager(dbManager) {
    setWindowTitle("Company Data Backup & Restore (Alt+Y)");
    setFixedWidth(520);
    setupUi();
}

void BackupRestoreDialog::setupUi() {
    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(12);

    m_tabWidget = new QTabWidget(this);

    // Tab 1: Backup
    QWidget* backupTab = new QWidget(this);
    QVBoxLayout* bLayout = new QVBoxLayout(backupTab);
    bLayout->setSpacing(12);

    QLabel* bTitle = new QLabel("Create a complete backup of the accounting database:", backupTab);
    bTitle->setStyleSheet("font-weight: bold; color: #1B4977;");
    bLayout->addWidget(bTitle);

    QHBoxLayout* destLayout = new QHBoxLayout();
    std::string defaultName = "tally_backup_" + utilities::DateUtils::currentIsoDate() + ".sqlite3";
    m_backupDestEdit = new QLineEdit(QString::fromStdString(defaultName), backupTab);
    destLayout->addWidget(m_backupDestEdit);

    QPushButton* bBrowseBtn = new QPushButton("Browse...", backupTab);
    connect(bBrowseBtn, &QPushButton::clicked, this, &BackupRestoreDialog::onBrowseBackupDest);
    destLayout->addWidget(bBrowseBtn);
    bLayout->addLayout(destLayout);

    QLabel* bNote = new QLabel("Note: Backups are transactionally consistent copies using SQLite WAL vacuum.", backupTab);
    bNote->setStyleSheet("color: #555; font-size: 8.5pt; font-style: italic;");
    bLayout->addWidget(bNote);

    bLayout->addStretch();

    QHBoxLayout* bActionLayout = new QHBoxLayout();
    bActionLayout->addStretch();
    m_backupBtn = new QPushButton("Backup Now", backupTab);
    m_backupBtn->setStyleSheet("background-color: #F6C343; font-weight: bold; padding: 6px 16px; border: 1px solid #1B4977;");
    connect(m_backupBtn, &QPushButton::clicked, this, &BackupRestoreDialog::onExecuteBackup);
    bActionLayout->addWidget(m_backupBtn);
    bLayout->addLayout(bActionLayout);

    m_tabWidget->addTab(backupTab, "Backup Data");

    // Tab 2: Restore
    QWidget* restoreTab = new QWidget(this);
    QVBoxLayout* rLayout = new QVBoxLayout(restoreTab);
    rLayout->setSpacing(12);

    QLabel* rTitle = new QLabel("Restore database from a previous backup file:", restoreTab);
    rTitle->setStyleSheet("font-weight: bold; color: #1B4977;");
    rLayout->addWidget(rTitle);

    QHBoxLayout* srcLayout = new QHBoxLayout();
    m_restoreSrcEdit = new QLineEdit(restoreTab);
    m_restoreSrcEdit->setPlaceholderText("Select backup file (.sqlite3)...");
    srcLayout->addWidget(m_restoreSrcEdit);

    QPushButton* rBrowseBtn = new QPushButton("Browse...", restoreTab);
    connect(rBrowseBtn, &QPushButton::clicked, this, &BackupRestoreDialog::onBrowseRestoreSrc);
    srcLayout->addWidget(rBrowseBtn);
    rLayout->addLayout(srcLayout);

    QLabel* rWarning = new QLabel("Warning: Restoring will replace existing records. Ensure integrity check passes.", restoreTab);
    rWarning->setStyleSheet("color: #C0392B; font-weight: bold; font-size: 8.5pt;");
    rLayout->addWidget(rWarning);

    rLayout->addStretch();

    QHBoxLayout* rActionLayout = new QHBoxLayout();
    rActionLayout->addStretch();
    m_restoreBtn = new QPushButton("Restore Now", restoreTab);
    m_restoreBtn->setStyleSheet("background-color: #C0392B; color: white; font-weight: bold; padding: 6px 16px;");
    connect(m_restoreBtn, &QPushButton::clicked, this, &BackupRestoreDialog::onExecuteRestore);
    rActionLayout->addWidget(m_restoreBtn);
    rLayout->addLayout(rActionLayout);

    m_tabWidget->addTab(restoreTab, "Restore Data");

    rootLayout->addWidget(m_tabWidget);

    QHBoxLayout* closeLayout = new QHBoxLayout();
    closeLayout->addStretch();
    QPushButton* closeBtn = new QPushButton("Close (Esc)", this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    closeLayout->addWidget(closeBtn);
    rootLayout->addLayout(closeLayout);
}

void BackupRestoreDialog::onBrowseBackupDest() {
    QString path = QFileDialog::getSaveFileName(this, "Save Database Backup", m_backupDestEdit->text(), "SQLite Database Files (*.sqlite3 *.db);;All Files (*)");
    if (!path.isEmpty()) {
        m_backupDestEdit->setText(path);
    }
}

void BackupRestoreDialog::onBrowseRestoreSrc() {
    QString path = QFileDialog::getOpenFileName(this, "Select Database Backup to Restore", "", "SQLite Database Files (*.sqlite3 *.db);;All Files (*)");
    if (!path.isEmpty()) {
        m_restoreSrcEdit->setText(path);
    }
}

void BackupRestoreDialog::onExecuteBackup() {
    QString dest = m_backupDestEdit->text().trimmed();
    if (dest.isEmpty()) {
        QMessageBox::warning(this, "Validation", "Destination backup file path is required.");
        return;
    }

    if (m_dbManager.backupTo(dest.toStdString())) {
        QMessageBox::information(this, "Success", "Database backup completed successfully to:\n" + dest);
        accept();
    } else {
        QMessageBox::critical(this, "Error", "Backup failed: " + QString::fromStdString(m_dbManager.lastError()));
    }
}

void BackupRestoreDialog::onExecuteRestore() {
    QString src = m_restoreSrcEdit->text().trimmed();
    if (src.isEmpty()) {
        QMessageBox::warning(this, "Validation", "Source backup file path is required.");
        return;
    }

    auto reply = QMessageBox::warning(
        this, "Confirm Restore",
        "Are you sure you want to restore? This will replace current database state with the backup file.",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No
    );
    if (reply != QMessageBox::Yes) return;

    if (m_dbManager.restoreFrom(src.toStdString())) {
        m_restored = true;
        QMessageBox::information(this, "Success", "Database successfully restored from backup!");
        accept();
    } else {
        QMessageBox::critical(this, "Error", "Restore failed: " + QString::fromStdString(m_dbManager.lastError()));
    }
}

} // namespace tally::ui
