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

std::string DatabaseManager::lastError() const {
    return m_db.lastError().text().toStdString();
}

} // namespace tally::database
