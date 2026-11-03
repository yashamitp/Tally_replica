#pragma once

#include <QSqlDatabase>
#include <string>

namespace tally::database {

class SchemaMigrations {
public:
    static bool applyMigrations(QSqlDatabase& db, std::string& errorOut);
};

} // namespace tally::database
