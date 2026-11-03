#include <cassert>
#include <iostream>
#include <QCoreApplication>
#include "../../src/database/DatabaseManager.hpp"
#include "../../src/services/CompanyService.hpp"
#include "../../src/services/LedgerService.hpp"
#include "../../src/services/VoucherService.hpp"
#include "../../src/services/ReportService.hpp"
#include "../../src/integrations/json/JsonExchange.hpp"

using namespace tally;
using namespace tally::integrations::json;

void testJsonExportAndImport() {
    database::DatabaseManager dbManager("JsonTestConnection");
    bool ok = dbManager.open(":memory:");
    assert(ok);

    services::CompanyService companyService(dbManager);
    services::LedgerService ledgerService(dbManager);
    services::VoucherService voucherService(dbManager);
    services::ReportService reportService(dbManager);

    // 1. Setup Source Company
    services::CreateCompanyParams cp;
    cp.name = "Omega Global Trade Ltd";
    cp.financialYearStart = "2026-04-01";
    auto compA = companyService.createCompany(cp);

    // Find Groups
    auto groupsA = companyService.listGroups(compA.id);
    std::string bankGrp, capGrp, salesGrp;
    for (const auto& g : groupsA) {
        if (g.name == "Bank Accounts") bankGrp = g.id;
        if (g.name == "Capital Account") capGrp = g.id;
        if (g.name == "Sales Accounts") salesGrp = g.id;
    }

    // Create Ledgers
    services::CreateLedgerParams pBank;
    pBank.companyId = compA.id; pBank.groupId = bankGrp; pBank.name = "Axis Bank";
    pBank.openingBalance = domain::ExactDecimal::fromString("100000.0000");
    pBank.openingBalanceType = domain::BalanceType::Dr;
    auto lBankA = ledgerService.createLedger(pBank);

    services::CreateLedgerParams pCap;
    pCap.companyId = compA.id; pCap.groupId = capGrp; pCap.name = "Share Capital";
    pCap.openingBalance = domain::ExactDecimal::fromString("100000.0000");
    pCap.openingBalanceType = domain::BalanceType::Cr;
    auto lCapA = ledgerService.createLedger(pCap);

    services::CreateLedgerParams pSales;
    pSales.companyId = compA.id; pSales.groupId = salesGrp; pSales.name = "Domestic Sales";
    auto lSalesA = ledgerService.createLedger(pSales);

    // Find Receipt Voucher Type
    auto vTypesA = companyService.listVoucherTypes(compA.id);
    std::string receiptTypeId;
    for (const auto& vt : vTypesA) {
        if (vt.category == domain::VoucherCategory::Receipt) receiptTypeId = vt.id;
    }

    // Post Voucher: Receipt Axis Bank DR 50,000, Domestic Sales CR 50,000
    domain::Voucher vp;
    vp.companyId = compA.id;
    vp.voucherTypeId = receiptTypeId;
    vp.voucherNumber = "RCPT-001";
    vp.date = "2026-04-15";
    vp.narration = "Advance from client";
    
    domain::VoucherEntry e1;
    e1.ledgerId = lBankA.id;
    e1.entryType = domain::BalanceType::Dr;
    e1.amount = domain::ExactDecimal::fromString("50000.0000");
    e1.narration = "Received via RTGS";
    e1.lineIndex = 0;
    vp.entries.push_back(e1);

    domain::VoucherEntry e2;
    e2.ledgerId = lSalesA.id;
    e2.entryType = domain::BalanceType::Cr;
    e2.amount = domain::ExactDecimal::fromString("50000.0000");
    e2.narration = "Domestic sales advance";
    e2.lineIndex = 1;
    vp.entries.push_back(e2);

    voucherService.postVoucher(vp);

    // Verify Source Balance Sheet
    auto bsA = reportService.generateBalanceSheet(compA.id, "2026-04-30");
    assert(bsA.isReconciled);
    assert(bsA.totalAssets.toDbString() == "150000.0000");
    assert(bsA.totalLiabilitiesAndEquity.toDbString() == "150000.0000");

    // 2. Export Masters and Vouchers to JSON
    JsonExchange exchange(dbManager);
    QJsonObject mastersJson = exchange.exportMastersJson(compA.id);
    assert(mastersJson["format"].toString() == "TALLY_COMPATIBLE_JSON_MASTERS");
    assert(mastersJson["groups"].toArray().size() >= 20);
    assert(mastersJson["ledgers"].toArray().size() >= 4);

    QJsonObject vouchersJson = exchange.exportVouchersJson(compA.id);
    assert(vouchersJson["format"].toString() == "TALLY_COMPATIBLE_JSON_VOUCHERS");
    assert(vouchersJson["vouchers"].toArray().size() == 1);

    // 3. Create Target Company and Import
    services::CreateCompanyParams cp2;
    cp2.name = "Omega Global Trade Ltd (Restored)";
    cp2.financialYearStart = "2026-04-01";
    auto compB = companyService.createCompany(cp2);

    auto mImport = exchange.importMastersJson(compB.id, mastersJson);
    assert(mImport.success);

    auto vImport = exchange.importVouchersJson(compB.id, vouchersJson);
    assert(vImport.success);
    assert(vImport.createdVouchers == 1);

    // 4. Verify Target Balance Sheet matches Source exactly!
    auto bsB = reportService.generateBalanceSheet(compB.id, "2026-04-30");
    assert(bsB.isReconciled);
    assert(bsB.totalAssets.toDbString() == "150000.0000");
    assert(bsB.totalLiabilitiesAndEquity.toDbString() == "150000.0000");
    assert(bsB.equity.currentPeriodProfitOrLoss.toDbString() == "50000.0000");

    std::cout << "[PASS] testJsonExportAndImport: 100% mathematical parity across companies!" << std::endl;
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    std::cout << "--- Running JSON Data Exchange Integration Tests ---" << std::endl;
    testJsonExportAndImport();
    std::cout << "All JSON Data Exchange tests passed successfully!" << std::endl;
    return 0;
}
