#include <QFile>
#include <QFileInfo>
#include "DatabaseManager.hpp"
#include "migrations/SchemaMigrations.hpp"
#include "../utilities/Logger.hpp"

namespace tally::database {

DatabaseManager::DatabaseManager(const std::string& connectionName)
    : m_connectionName(connectionName) {
}

DatabaseManager::~DatabaseManager() {
    close();
}

bool DatabaseManager::open(const std::string& dbPath) {
    m_dbPath = dbPath;

    if (QSqlDatabase::contains(QString::fromStdString(m_connectionName))) {
        m_db = QSqlDatabase::database(QString::fromStdString(m_connectionName));
    } else {
        m_db = QSqlDatabase::addDatabase("QSQLITE", QString::fromStdString(m_connectionName));
    }

    m_db.setDatabaseName(QString::fromStdString(m_dbPath));

    if (!m_db.open()) {
        tally::utilities::Logger::error("Failed to open SQLite database: " + m_db.lastError().text().toStdString());
        return false;
    }

    // Configure SQLite pragmas for safety & performance
    QSqlQuery pragma(m_db);
    pragma.exec("PRAGMA foreign_keys = ON;");
    if (m_dbPath != ":memory:") {
        pragma.exec("PRAGMA journal_mode = WAL;");
        pragma.exec("PRAGMA synchronous = NORMAL;");
    }

    // Apply migrations
    std::string migrationErr;
    if (!SchemaMigrations::applyMigrations(m_db, migrationErr)) {
        tally::utilities::Logger::error("Failed to apply migrations: " + migrationErr);
        return false;
    }

    tally::utilities::Logger::info("DatabaseManager opened successfully at: " + m_dbPath);
    return true;
}

void DatabaseManager::close() {
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool DatabaseManager::isOpen() const {
    return m_db.isOpen();
}

bool DatabaseManager::beginTransaction() {
    return m_db.transaction();
}

bool DatabaseManager::commitTransaction() {
    return m_db.commit();
}

bool DatabaseManager::rollbackTransaction() {
    return m_db.rollback();
}

bool DatabaseManager::backupTo(const std::string& destinationPath) {
    if (!isOpen()) {
        utilities::Logger::error("Cannot backup: database is not open");
        return false;
    }

    QString dest = QString::fromStdString(destinationPath);
    if (QFile::exists(dest)) {
        QFile::remove(dest);
    }

    QSqlQuery q(m_db);
    QString sql = QString("VACUUM INTO '%1'").arg(dest);
    if (!q.exec(sql)) {
        utilities::Logger::error("Backup failed: " + q.lastError().text().toStdString());
        return false;
    }

    utilities::Logger::info("Database successfully backed up to: " + destinationPath);
    return true;
}

bool DatabaseManager::restoreFrom(const std::string& sourcePath) {
    QString src = QString::fromStdString(sourcePath);
    if (!QFile::exists(src)) {
        utilities::Logger::error("Restore failed: source file does not exist: " + sourcePath);
        return false;
    }

    // Validate that the backup file is a valid SQLite database
    {
        QString testConnName = QString::fromStdString(m_connectionName) + "_RestoreCheck";
        QSqlDatabase testDb = QSqlDatabase::addDatabase("QSQLITE", testConnName);
        testDb.setDatabaseName(src);
        if (!testDb.open()) {
            utilities::Logger::error("Restore failed: unable to open source file as SQLite database: " + testDb.lastError().text().toStdString());
            QSqlDatabase::removeDatabase(testConnName);
            return false;
        }

        QSqlQuery checkQuery(testDb);
        if (!checkQuery.exec("PRAGMA quick_check") || !checkQuery.next() || checkQuery.value(0).toString() != "ok") {
            utilities::Logger::error("Restore failed: source database failed integrity check");
            testDb.close();
            QSqlDatabase::removeDatabase(testConnName);
            return false;
        }
        testDb.close();
        QSqlDatabase::removeDatabase(testConnName);
    }

    std::string currentDbPath = m_dbPath;
    close();

    if (currentDbPath != ":memory:") {
        QString currentTarget = QString::fromStdString(currentDbPath);
        if (QFile::exists(currentTarget)) {
            QFile::remove(currentTarget);
        }
        if (!QFile::copy(src, currentTarget)) {
            utilities::Logger::error("Restore failed: unable to copy backup file over current database");
            open(currentDbPath);
            return false;
        }
        return open(currentDbPath);
    } else {
        // If in-memory, reopen with the source path directly
        return open(sourcePath);
    }
}

std::string DatabaseManager::lastError() const {
    return m_db.lastError().text().toStdString();
}

} // namespace tally::database
