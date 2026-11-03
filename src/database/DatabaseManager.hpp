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

    std::string lastError() const;

private:
    std::string m_connectionName;
    std::string m_dbPath;
    QSqlDatabase m_db;
};

} // namespace tally::database
