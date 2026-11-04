#include <iostream>
#include <cassert>
#include <QFile>
#include "database/DatabaseManager.hpp"
#include "services/CompanyService.hpp"
#include "services/LedgerService.hpp"

using namespace tally;

void testBackupAndRestore() {
    std::cout << "Running testBackupAndRestore..." << std::endl;

    const std::string origDb = "test_orig.sqlite3";
    const std::string backupDb = "test_backup.sqlite3";

    QFile::remove(QString::fromStdString(origDb));
    QFile::remove(QString::fromStdString(backupDb));

    {
        database::DatabaseManager dbManager("TestBackupConn");
        bool ok = dbManager.open(origDb);
        assert(ok && "Failed to open test database");

        services::CompanyService compService(dbManager);
        services::LedgerService ledgerService(dbManager);

        services::CreateCompanyParams cp;
        cp.name = "Original Company";
        auto comp = compService.createCompany(cp);
        assert(!comp.id.empty());

        // Perform backup to backupDb
        bool backedUp = dbManager.backupTo(backupDb);
        assert(backedUp && "backupTo failed");
        assert(QFile::exists(QString::fromStdString(backupDb)) && "Backup file does not exist");

        // Now modify original database
        comp.name = "Altered Original Company";
        compService.alterCompany(comp);
        auto refetched = compService.getCompany(comp.id);
        assert(refetched->name == "Altered Original Company");

        // Now restore from backupDb
        bool restored = dbManager.restoreFrom(backupDb);
        assert(restored && "restoreFrom failed");

        // Verify company name is reverted to "Original Company"
        auto restoredComp = compService.getCompany(comp.id);
        assert(restoredComp.has_value());
        assert(restoredComp->name == "Original Company" && "Restored state did not match original backup");

        dbManager.close();
    }

    // Clean up
    QFile::remove(QString::fromStdString(origDb));
    QFile::remove(QString::fromStdString(backupDb));

    std::cout << "testBackupAndRestore passed successfully!" << std::endl;
}

int main() {
    testBackupAndRestore();
    return 0;
}
