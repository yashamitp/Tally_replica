#include <iostream>
#include <cassert>
#include "database/DatabaseManager.hpp"
#include "services/CompanyService.hpp"
#include "services/GroupService.hpp"
#include "services/LedgerService.hpp"

using namespace tally;

void testMastersLifecycle() {
    std::cout << "Running testMastersLifecycle..." << std::endl;

    database::DatabaseManager dbManager("TestMastersDb");
    bool ok = dbManager.open(":memory:");
    assert(ok && "Database failed to open in memory");

    services::CompanyService compService(dbManager);
    services::GroupService groupService(dbManager);
    services::LedgerService ledgerService(dbManager);

    // 1. Create Company
    services::CreateCompanyParams cp;
    cp.name = "Apex Industrial Corp";
    cp.formalName = "Apex Industrial Corporation Ltd";
    cp.gstin = "27AAACA1234A1Z5";
    auto comp = compService.createCompany(cp);
    assert(!comp.id.empty());
    assert(comp.name == "Apex Industrial Corp");

    // 2. Alter Company
    comp.city = "Pune";
    comp.state = "Maharashtra";
    bool altered = compService.alterCompany(comp);
    assert(altered);
    auto compRefetched = compService.getCompany(comp.id);
    assert(compRefetched.has_value());
    assert(compRefetched->city.value_or("") == "Pune");

    // 3. Create Custom Accounting Group
    services::CreateGroupParams gp;
    gp.companyId = comp.id;
    gp.name = "Regional Distributors";
    gp.nature = domain::GroupNature::Asset;
    auto sundryDebtors = groupService.getGroupByName(comp.id, "Sundry Debtors");
    assert(sundryDebtors.has_value());
    gp.parentId = sundryDebtors->id;
    auto customGroup = groupService.createGroup(gp);
    assert(!customGroup.id.empty());
    assert(customGroup.name == "Regional Distributors");
    assert(customGroup.parentId.value_or("") == sundryDebtors->id);

    // Verify duplicate group rejected
    bool dupRejected = false;
    try {
        groupService.createGroup(gp);
    } catch (...) {
        dupRejected = true;
    }
    assert(dupRejected && "Duplicate group name must be rejected");

    // 4. Alter Group
    customGroup.name = "Western Regional Distributors";
    bool groupAltered = groupService.alterGroup(customGroup);
    assert(groupAltered);
    auto groupRefetched = groupService.getGroup(customGroup.id);
    assert(groupRefetched.has_value());
    assert(groupRefetched->name == "Western Regional Distributors");

    // 5. Create Ledger under custom group
    services::CreateLedgerParams lp;
    lp.companyId = comp.id;
    lp.groupId = customGroup.id;
    lp.name = "Omkar Traders";
    lp.openingBalance = domain::ExactDecimal::fromString("25000.0000");
    lp.openingBalanceType = domain::BalanceType::Dr;
    lp.gstin = "27ABCDE1234F1Z9";
    auto ledger = ledgerService.createLedger(lp);
    assert(!ledger.id.empty());
    assert(ledger.name == "Omkar Traders");
    assert(ledger.openingBalance == domain::ExactDecimal::fromString("25000.0000"));

    // 6. Alter Ledger
    ledger.name = "Omkar Traders Pvt Ltd";
    ledger.address = "101 Industrial Area, Pune";
    bool ledgerAltered = ledgerService.alterLedger(ledger);
    assert(ledgerAltered);
    auto ledgerRefetched = ledgerService.getLedger(ledger.id);
    assert(ledgerRefetched.has_value());
    assert(ledgerRefetched->name == "Omkar Traders Pvt Ltd");
    assert(ledgerRefetched->address.value_or("") == "101 Industrial Area, Pune");

    // 7. Verify Group Deletion Protection when ledgers exist
    bool deleteBlocked = false;
    try {
        groupService.deleteGroup(customGroup.id);
    } catch (...) {
        deleteBlocked = true;
    }
    assert(deleteBlocked && "Group deletion must fail when ledgers exist under it");

    std::cout << "testMastersLifecycle passed successfully!" << std::endl;
}

int main() {
    testMastersLifecycle();
    return 0;
}
