#include <cassert>
#include <iostream>
#include <QCoreApplication>
#include "../../src/database/DatabaseManager.hpp"
#include "../../src/services/CompanyService.hpp"
#include "../../src/services/LedgerService.hpp"
#include "../../src/domain/accounting/AccountingEngine.hpp"
#include "../../src/services/VoucherService.hpp"

using namespace tally;

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    std::cout << "--- Running Accounting Engine Integration Tests ---" << std::endl;

    database::DatabaseManager dbManager("AccountingTestConnection");
    bool ok = dbManager.open(":memory:");
    assert(ok);

    services::CompanyService companyService(dbManager);
    services::LedgerService ledgerService(dbManager);
    services::VoucherService voucherService(dbManager);

    // 1. Create Company
    services::CreateCompanyParams compParams;
    compParams.name = "Apex Trading Pvt Ltd";
    compParams.financialYearStart = "2026-04-01";
    auto company = companyService.createCompany(compParams);
    assert(!company.id.empty());
    assert(company.name == "Apex Trading Pvt Ltd");

    // Check voucher types
    auto vTypes = companyService.listVoucherTypes(company.id);
    assert(vTypes.size() == 6);
    std::string receiptTypeId, salesTypeId, journalTypeId;
    for (const auto& vt : vTypes) {
        if (vt.category == domain::VoucherCategory::Receipt) receiptTypeId = vt.id;
        if (vt.category == domain::VoucherCategory::Sales) salesTypeId = vt.id;
        if (vt.category == domain::VoucherCategory::Journal) journalTypeId = vt.id;
    }

    // Check Groups
    auto groups = companyService.listGroups(company.id);
    assert(groups.size() == 20); // 12 primary + 8 subgroups
    std::string bankGroupId, capitalGroupId, salesGroupId;
    for (const auto& g : groups) {
        if (g.name == "Bank Accounts") bankGroupId = g.id;
        if (g.name == "Capital Account") capitalGroupId = g.id;
        if (g.name == "Sales Accounts") salesGroupId = g.id;
    }

    // Check default Cash ledger
    auto cashLedger = ledgerService.getLedgerByName(company.id, "Cash");
    assert(cashLedger.has_value());
    assert(cashLedger->name == "Cash");
    assert(cashLedger->currentBalance == domain::ExactDecimal::zero());

    // 2. Create Ledgers
    services::CreateLedgerParams hdfcParams;
    hdfcParams.companyId = company.id;
    hdfcParams.groupId = bankGroupId;
    hdfcParams.name = "HDFC Bank Current A/c";
    hdfcParams.openingBalance = domain::ExactDecimal::zero();
    hdfcParams.openingBalanceType = domain::BalanceType::Dr;
    auto hdfc = ledgerService.createLedger(hdfcParams);

    services::CreateLedgerParams capitalParams;
    capitalParams.companyId = company.id;
    capitalParams.groupId = capitalGroupId;
    capitalParams.name = "Share Capital";
    capitalParams.openingBalance = domain::ExactDecimal::zero();
    capitalParams.openingBalanceType = domain::BalanceType::Cr;
    auto capital = ledgerService.createLedger(capitalParams);

    services::CreateLedgerParams salesParams;
    salesParams.companyId = company.id;
    salesParams.groupId = salesGroupId;
    salesParams.name = "Domestic Sales";
    salesParams.openingBalance = domain::ExactDecimal::zero();
    salesParams.openingBalanceType = domain::BalanceType::Cr;
    auto sales = ledgerService.createLedger(salesParams);

    // 3. Post Capital Injection Voucher: Dr HDFC 500,000, Cr Capital 500,000
    domain::Voucher v1;
    v1.companyId = company.id;
    v1.voucherTypeId = receiptTypeId;
    v1.voucherNumber = "RCP-001";
    v1.date = "2026-04-01";
    v1.narration = "Initial Capital Infusion";

    domain::VoucherEntry e1, e2;
    e1.ledgerId = hdfc.id;
    e1.entryType = domain::BalanceType::Dr;
    e1.amount = domain::ExactDecimal::fromString("500000.0000");

    e2.ledgerId = capital.id;
    e2.entryType = domain::BalanceType::Cr;
    e2.amount = domain::ExactDecimal::fromString("500000.0000");

    v1.entries = {e1, e2};
    auto postedV1 = voucherService.postVoucher(v1);
    assert(postedV1.status == domain::VoucherStatus::Posted);

    // Check balances
    auto hdfcUpdated = ledgerService.getLedger(hdfc.id);
    auto capitalUpdated = ledgerService.getLedger(capital.id);
    assert(hdfcUpdated->currentBalance == domain::ExactDecimal::fromString("500000.0000"));
    assert(capitalUpdated->currentBalance == domain::ExactDecimal::fromString("500000.0000"));
    std::cout << "[PASS] Capital injection voucher posted & balances updated correctly" << std::endl;

    // 4. Duplicate Voucher Number Check
    bool dupRejected = false;
    try {
        voucherService.postVoucher(v1);
    } catch (const std::invalid_argument&) {
        dupRejected = true;
    }
    assert(dupRejected);
    std::cout << "[PASS] Duplicate voucher number successfully rejected" << std::endl;

    // 5. Post Sales Voucher: Dr Cash 25,000, Cr Sales 25,000
    domain::Voucher v2;
    v2.companyId = company.id;
    v2.voucherTypeId = salesTypeId;
    v2.voucherNumber = "SAL-001";
    v2.date = "2026-04-05";
    v2.narration = "Cash Sales";

    domain::VoucherEntry v2_e1, v2_e2;
    v2_e1.ledgerId = cashLedger->id;
    v2_e1.entryType = domain::BalanceType::Dr;
    v2_e1.amount = domain::ExactDecimal::fromString("25000.0000");

    v2_e2.ledgerId = sales.id;
    v2_e2.entryType = domain::BalanceType::Cr;
    v2_e2.amount = domain::ExactDecimal::fromString("25000.0000");

    v2.entries = {v2_e1, v2_e2};
    auto postedV2 = voucherService.postVoucher(v2);

    auto cashAfterSale = ledgerService.getLedger(cashLedger->id);
    auto salesAfterSale = ledgerService.getLedger(sales.id);
    assert(cashAfterSale->currentBalance == domain::ExactDecimal::fromString("25000.0000"));
    assert(salesAfterSale->currentBalance == domain::ExactDecimal::fromString("25000.0000"));
    std::cout << "[PASS] Sales voucher posted & Cash balance updated to 25,000" << std::endl;

    // 6. Test Reversal of Sales Voucher
    domain::ReverseVoucherParams revParams;
    revParams.companyId = company.id;
    revParams.voucherId = postedV2.id;
    revParams.reversalDate = "2026-04-06";
    revParams.reason = "Customer returned goods";

    auto revVoucher = voucherService.reverseVoucher(revParams);
    assert(revVoucher.entries.size() == 2);

    auto cashAfterRev = ledgerService.getLedger(cashLedger->id);
    auto salesAfterRev = ledgerService.getLedger(sales.id);
    assert(cashAfterRev->currentBalance == domain::ExactDecimal::zero());
    assert(salesAfterRev->currentBalance == domain::ExactDecimal::zero());
    std::cout << "[PASS] Reversal voucher created & balances restored to 0 with zero distortion" << std::endl;

    // 7. Test Ledger Statement for HDFC Bank
    auto stmt = ledgerService.getLedgerStatement(company.id, hdfc.id, "2026-04-01", "2026-04-30");
    assert(stmt.openingBalance == domain::ExactDecimal::zero());
    assert(stmt.totalDebits == domain::ExactDecimal::fromString("500000.0000"));
    assert(stmt.closingBalance == domain::ExactDecimal::fromString("500000.0000"));
    assert(stmt.closingBalanceType == domain::BalanceType::Dr);
    assert(stmt.transactions.size() == 1);
    assert(stmt.transactions[0].particulars == "Share Capital");
    std::cout << "[PASS] Ledger Statement generated with exact running balance and counter-ledger particulars" << std::endl;

    std::cout << "All Accounting Engine integration tests passed successfully!" << std::endl;
    return 0;
}
