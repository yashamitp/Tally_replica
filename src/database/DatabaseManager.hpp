#pragma once

#include <string>
#include <memory>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

namespace tally::database {

class DatabaseManager {
public:
    explicit DatabaseManager(const std::string& connectionName = "TallyReplicaConnection");
    ~DatabaseManager();

    bool open(const std::string& dbPath = ":memory:");
    void close();
    bool isOpen() const;

    QSqlDatabase& database() { return m_db; }
    std::string connectionName() const { return m_connectionName; }

    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();

    bool backupTo(const std::string& destinationPath);
    bool restoreFrom(const std::string& sourcePath);

    std::string dbPath() const { return m_dbPath; }
    std::string lastError() const;

private:
    std::string m_connectionName;
    std::string m_dbPath;
    QSqlDatabase m_db;
};

} // namespace tally::database
