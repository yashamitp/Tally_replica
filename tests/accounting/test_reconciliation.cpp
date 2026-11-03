#include <cassert>
#include <iostream>
#include <QCoreApplication>
#include "../../src/database/DatabaseManager.hpp"
#include "../../src/services/CompanyService.hpp"
#include "../../src/services/LedgerService.hpp"
#include "../../src/services/VoucherService.hpp"
#include "../../src/services/ReportService.hpp"

using namespace tally;

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    std::cout << "--- Running Cross-Report Triple Reconciliation Tests ---" << std::endl;

    database::DatabaseManager dbManager("ReconciliationTestConnection");
    bool ok = dbManager.open(":memory:");
    assert(ok);

    services::CompanyService companyService(dbManager);
    services::LedgerService ledgerService(dbManager);
    services::VoucherService voucherService(dbManager);
    services::ReportService reportService(dbManager);

    // Setup Company: "Kuber Industries Ltd"
    services::CreateCompanyParams compParams;
    compParams.name = "Kuber Industries Ltd";
    compParams.financialYearStart = "2026-04-01";
    auto company = companyService.createCompany(compParams);

    // Voucher Types
    auto vTypes = companyService.listVoucherTypes(company.id);
    std::string salesTypeId, purchaseTypeId, paymentTypeId, receiptTypeId;
    for (const auto& vt : vTypes) {
        if (vt.category == domain::VoucherCategory::Sales) salesTypeId = vt.id;
        if (vt.category == domain::VoucherCategory::Purchase) purchaseTypeId = vt.id;
        if (vt.category == domain::VoucherCategory::Payment) paymentTypeId = vt.id;
        if (vt.category == domain::VoucherCategory::Receipt) receiptTypeId = vt.id;
    }

    // Groups
    auto groups = companyService.listGroups(company.id);
    std::string bankGrp, capitalGrp, debtorGrp, creditorGrp, salesGrp, purchaseGrp, expGrp;
    for (const auto& g : groups) {
        if (g.name == "Bank Accounts") bankGrp = g.id;
        if (g.name == "Capital Account") capitalGrp = g.id;
        if (g.name == "Sundry Debtors") debtorGrp = g.id;
        if (g.name == "Sundry Creditors") creditorGrp = g.id;
        if (g.name == "Sales Accounts") salesGrp = g.id;
        if (g.name == "Purchase Accounts") purchaseGrp = g.id;
        if (g.name == "Indirect Expenses") expGrp = g.id;
    }

    // 1. Initial Opening Balances:
    // Bank: 500,000 DR
    // Capital: 500,000 CR
    services::CreateLedgerParams p1;
    p1.companyId = company.id; p1.groupId = bankGrp; p1.name = "HDFC Current Account";
    p1.openingBalance = domain::ExactDecimal::fromString("500000.0000");
    p1.openingBalanceType = domain::BalanceType::Dr;
    auto bank = ledgerService.createLedger(p1);

    services::CreateLedgerParams p2;
    p2.companyId = company.id; p2.groupId = capitalGrp; p2.name = "Founder Equity";
    p2.openingBalance = domain::ExactDecimal::fromString("500000.0000");
    p2.openingBalanceType = domain::BalanceType::Cr;
    auto capital = ledgerService.createLedger(p2);

    // Customer & Supplier Ledgers
    services::CreateLedgerParams p3;
    p3.companyId = company.id; p3.groupId = debtorGrp; p3.name = "Acme Retailers";
    auto customer = ledgerService.createLedger(p3);

    services::CreateLedgerParams p4;
    p4.companyId = company.id; p4.groupId = creditorGrp; p4.name = "Global Wholesalers";
    auto supplier = ledgerService.createLedger(p4);

    services::CreateLedgerParams p5;
    p5.companyId = company.id; p5.groupId = salesGrp; p5.name = "Product Sales A/c";
    auto sales = ledgerService.createLedger(p5);

    services::CreateLedgerParams p6;
    p6.companyId = company.id; p6.groupId = purchaseGrp; p6.name = "Goods Purchase A/c";
    auto purchase = ledgerService.createLedger(p6);

    services::CreateLedgerParams p7;
    p7.companyId = company.id; p7.groupId = expGrp; p7.name = "Office Rent";
    auto rent = ledgerService.createLedger(p7);

    // 2. Post Purchase: 150,000 (Dr Purchase, Cr Global Wholesalers)
    domain::Voucher v1;
    v1.companyId = company.id; v1.voucherTypeId = purchaseTypeId; v1.voucherNumber = "PUR-01";
    v1.date = "2026-04-10";
    domain::VoucherEntry v1_e1{ "", "", purchase.id, domain::BalanceType::Dr, domain::ExactDecimal::fromString("150000.0000"), "", 1, "" };
    domain::VoucherEntry v1_e2{ "", "", supplier.id, domain::BalanceType::Cr, domain::ExactDecimal::fromString("150000.0000"), "", 2, "" };
    v1.entries = {v1_e1, v1_e2};
    voucherService.postVoucher(v1);

    // 3. Post Sales: 300,000 (Dr Acme Retailers, Cr Product Sales)
    domain::Voucher v2;
    v2.companyId = company.id; v2.voucherTypeId = salesTypeId; v2.voucherNumber = "SAL-01";
    v2.date = "2026-04-15";
    domain::VoucherEntry v2_e1{ "", "", customer.id, domain::BalanceType::Dr, domain::ExactDecimal::fromString("300000.0000"), "", 1, "" };
    domain::VoucherEntry v2_e2{ "", "", sales.id, domain::BalanceType::Cr, domain::ExactDecimal::fromString("300000.0000"), "", 2, "" };
    v2.entries = {v2_e1, v2_e2};
    voucherService.postVoucher(v2);

    // 4. Post Payment for Rent: 20,000 (Dr Office Rent, Cr HDFC)
    domain::Voucher v3;
    v3.companyId = company.id; v3.voucherTypeId = paymentTypeId; v3.voucherNumber = "PAY-01";
    v3.date = "2026-04-20";
    domain::VoucherEntry v3_e1{ "", "", rent.id, domain::BalanceType::Dr, domain::ExactDecimal::fromString("20000.0000"), "", 1, "" };
    domain::VoucherEntry v3_e2{ "", "", bank.id, domain::BalanceType::Cr, domain::ExactDecimal::fromString("20000.0000"), "", 2, "" };
    v3.entries = {v3_e1, v3_e2};
    voucherService.postVoucher(v3);

    // 5. Post Customer Receipt: 200,000 (Dr HDFC, Cr Acme Retailers)
    domain::Voucher v4;
    v4.companyId = company.id; v4.voucherTypeId = receiptTypeId; v4.voucherNumber = "RCP-02";
    v4.date = "2026-04-25";
    domain::VoucherEntry v4_e1{ "", "", bank.id, domain::BalanceType::Dr, domain::ExactDecimal::fromString("200000.0000"), "", 1, "" };
    domain::VoucherEntry v4_e2{ "", "", customer.id, domain::BalanceType::Cr, domain::ExactDecimal::fromString("200000.0000"), "", 2, "" };
    v4.entries = {v4_e1, v4_e2};
    voucherService.postVoucher(v4);

    // 6. Post Supplier Payment: 100,000 (Dr Global Wholesalers, Cr HDFC)
    domain::Voucher v5;
    v5.companyId = company.id; v5.voucherTypeId = paymentTypeId; v5.voucherNumber = "PAY-02";
    v5.date = "2026-04-28";
    domain::VoucherEntry v5_e1{ "", "", supplier.id, domain::BalanceType::Dr, domain::ExactDecimal::fromString("100000.0000"), "", 1, "" };
    domain::VoucherEntry v5_e2{ "", "", bank.id, domain::BalanceType::Cr, domain::ExactDecimal::fromString("100000.0000"), "", 2, "" };
    v5.entries = {v5_e1, v5_e2};
    voucherService.postVoucher(v5);

    // --- VERIFICATION 1: Trial Balance Reconciles ---
    auto tb = reportService.generateTrialBalance(company.id, "2026-04-01", "2026-04-30", true);
    assert(tb.isReconciled);
    assert(tb.discrepancy.isZero());
    assert(tb.totalClosingDebit == tb.totalClosingCredit);
    std::cout << "[PASS] Trial Balance Reconciles: Debits (" << tb.totalClosingDebit << ") == Credits (" << tb.totalClosingCredit << ")" << std::endl;

    // --- VERIFICATION 2: Profit & Loss Statement ---
    // Direct Income (Sales) = 300,000
    // Direct Expense (Purchase) = 150,000
    // Gross Profit = 150,000
    // Indirect Expense (Rent) = 20,000
    // Net Profit = 130,000
    auto pnl = reportService.generateProfitAndLoss(company.id, "2026-04-01", "2026-04-30");
    assert(pnl.tradingAccount.isGrossProfit);
    assert(pnl.tradingAccount.grossProfit == domain::ExactDecimal::fromString("150000.0000"));
    assert(pnl.operatingAccount.isNetProfit);
    assert(pnl.operatingAccount.netProfit == domain::ExactDecimal::fromString("130000.0000"));
    std::cout << "[PASS] Profit & Loss Reconciles: Gross Profit = " << pnl.tradingAccount.grossProfit << ", Net Profit = " << pnl.operatingAccount.netProfit << std::endl;

    // --- VERIFICATION 3: Balance Sheet Statement ---
    // Assets:
    //   Bank: 500,000 (Open) - 20,000 (Rent) + 200,000 (Cust) - 100,000 (Supp) = 580,000 DR
    //   Sundry Debtors (Acme): 300,000 - 200,000 = 100,000 DR
    //   Total Assets = 680,000
    // Liabilities:
    //   Sundry Creditors (Global): 150,000 - 100,000 = 50,000 CR
    // Equity:
    //   Founder Equity = 500,000 CR
    //   Current Period Net Profit = 130,000
    //   Total Equity = 630,000
    // Total Liabilities & Equity = 50,000 + 630,000 = 680,000
    auto bs = reportService.generateBalanceSheet(company.id, "2026-04-30");
    std::cout << "DEBUG: totalAssets=" << bs.totalAssets << " liab=" << bs.liabilities.total
              << " cap=" << bs.equity.totalCapital << " pnl=" << bs.equity.currentPeriodProfitOrLoss
              << " totLiabEq=" << bs.totalLiabilitiesAndEquity << " disc=" << bs.discrepancy << std::endl;
    assert(bs.isReconciled);
    assert(bs.discrepancy.isZero());
    assert(bs.totalAssets == domain::ExactDecimal::fromString("680000.0000"));
    assert(bs.totalLiabilitiesAndEquity == domain::ExactDecimal::fromString("680000.0000"));
    std::cout << "[PASS] Balance Sheet Reconciles: Assets (" << bs.totalAssets << ") == Liabilities + Equity (" << bs.totalLiabilitiesAndEquity << ")" << std::endl;

    std::cout << "All cross-report reconciliation tests passed with mathematical precision!" << std::endl;
    return 0;
}
