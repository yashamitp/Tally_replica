#include <cassert>
#include <iostream>
#include <QCoreApplication>
#include <QSqlQuery>
#include <QUuid>
#include "../../src/database/DatabaseManager.hpp"
#include "../../src/services/CompanyService.hpp"
#include "../../src/services/LedgerService.hpp"
#include "../../src/services/VoucherService.hpp"
#include "../../src/services/ReportService.hpp"
#include "../../src/services/DayBookService.hpp"

using namespace tally;

static std::string createSubGroup(
    database::DatabaseManager& dbManager,
    const std::string& companyId,
    const std::string& parentId,
    const std::string& name,
    domain::GroupNature nature,
    bool affectsGrossProfit
) {
    std::string gid = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    QSqlQuery q(dbManager.database());
    q.prepare(R"(
        INSERT INTO accounting_groups (
            id, company_id, parent_id, name, nature, affects_gross_profit, is_primary, is_system, created_at, updated_at
        ) VALUES (
            :id, :company_id, :parent_id, :name, :nature, :affects_gross_profit, 0, 0, '2026-04-01T00:00:00Z', '2026-04-01T00:00:00Z'
        )
    )");
    q.bindValue(":id", QString::fromStdString(gid));
    q.bindValue(":company_id", QString::fromStdString(companyId));
    q.bindValue(":parent_id", QString::fromStdString(parentId));
    q.bindValue(":name", QString::fromStdString(name));
    q.bindValue(":nature", QString::fromStdString(domain::groupNatureToString(nature)));
    q.bindValue(":affects_gross_profit", affectsGrossProfit ? 1 : 0);
    if (!q.exec()) {
        throw std::runtime_error("Failed to insert subgroup: " + name);
    }
    return gid;
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    std::cout << "=== Running Comprehensive Accounting & Nested Trial Balance Tests ===" << std::endl;

    database::DatabaseManager dbManager("NestedTBTestConnection");
    bool ok = dbManager.open(":memory:");
    assert(ok && "Failed to open in-memory database");

    services::CompanyService companyService(dbManager);
    services::LedgerService ledgerService(dbManager);
    services::VoucherService voucherService(dbManager);
    services::ReportService reportService(dbManager);
    services::DayBookService dayBookService(dbManager);

    // 1. Create Disposable Company
    services::CreateCompanyParams compParams;
    compParams.name = "Apex Nested Audit Ltd";
    compParams.financialYearStart = "2026-04-01";
    compParams.booksBeginningFrom = "2026-04-01";
    auto company = companyService.createCompany(compParams);
    assert(!company.id.empty());
    std::cout << "[PASS] Company created: " << company.name << std::endl;

    // Resolve system primary groups
    auto groups = companyService.listGroups(company.id);
    std::string curAssetsId, bankAccountsId, capitalGroupId, salesGroupId, dirExpId, indirExpId;
    for (const auto& g : groups) {
        if (g.name == "Current Assets") curAssetsId = g.id;
        else if (g.name == "Bank Accounts") bankAccountsId = g.id;
        else if (g.name == "Capital Account") capitalGroupId = g.id;
        else if (g.name == "Sales Accounts") salesGroupId = g.id;
        else if (g.name == "Direct Expenses") dirExpId = g.id;
        else if (g.name == "Indirect Expenses") indirExpId = g.id;
    }
    assert(!curAssetsId.empty() && !dirExpId.empty() && !indirExpId.empty());

    // 2. Build 4-Level Nested Group Hierarchy
    // Level 1: Current Assets (Primary)
    // Level 2: Sundry Debtors (Child of Current Assets)
    std::string debtorsL2 = createSubGroup(dbManager, company.id, curAssetsId, "Sundry Debtors L2", domain::GroupNature::Asset, false);
    // Level 3: North Region Debtors (Child of Sundry Debtors L2)
    std::string debtorsL3 = createSubGroup(dbManager, company.id, debtorsL2, "North Region Debtors L3", domain::GroupNature::Asset, false);
    // Level 4: Retail North Debtors (Child of North Region Debtors L3)
    std::string debtorsL4 = createSubGroup(dbManager, company.id, debtorsL3, "Retail North Debtors L4", domain::GroupNature::Asset, false);

    // Nested Direct Expense Hierarchy: Direct Expenses -> Manufacturing Expenses -> Factory Power
    std::string mfgExpId = createSubGroup(dbManager, company.id, dirExpId, "Manufacturing Expenses", domain::GroupNature::Expense, true);
    std::string factoryPowerGroupId = createSubGroup(dbManager, company.id, mfgExpId, "Factory Power Group", domain::GroupNature::Expense, true);

    // Nested Indirect Expense Hierarchy: Indirect Expenses -> Administrative Expenses -> Office Expenses
    std::string adminExpId = createSubGroup(dbManager, company.id, indirExpId, "Administrative Expenses", domain::GroupNature::Expense, false);
    std::string officeExpGroupId = createSubGroup(dbManager, company.id, adminExpId, "Office Expenses Group", domain::GroupNature::Expense, false);

    // 3. Create Ledgers across hierarchy levels with Opening Balances
    // Debtor Gamma at Level 2 (Sundry Debtors): ₹20,000 Dr opening balance
    services::CreateLedgerParams gammaParams;
    gammaParams.companyId = company.id;
    gammaParams.groupId = debtorsL2;
    gammaParams.name = "Debtor Gamma L2";
    gammaParams.openingBalance = domain::ExactDecimal::fromString("20000.00");
    gammaParams.openingBalanceType = domain::BalanceType::Dr;
    auto lGamma = ledgerService.createLedger(gammaParams);

    // Debtor Beta at Level 3 (North Region): ₹30,000 Dr opening balance
    services::CreateLedgerParams betaParams;
    betaParams.companyId = company.id;
    betaParams.groupId = debtorsL3;
    betaParams.name = "Debtor Beta L3";
    betaParams.openingBalance = domain::ExactDecimal::fromString("30000.00");
    betaParams.openingBalanceType = domain::BalanceType::Dr;
    auto lBeta = ledgerService.createLedger(betaParams);

    // Debtor Alpha at Level 4 (Retail North): ₹50,000 Dr opening balance
    services::CreateLedgerParams alphaParams;
    alphaParams.companyId = company.id;
    alphaParams.groupId = debtorsL4;
    alphaParams.name = "Debtor Alpha L4";
    alphaParams.openingBalance = domain::ExactDecimal::fromString("50000.00");
    alphaParams.openingBalanceType = domain::BalanceType::Dr;
    auto lAlpha = ledgerService.createLedger(alphaParams);

    // Capital Ledger: ₹1,00,000 Cr opening balance to reconcile opening TB
    services::CreateLedgerParams capParams;
    capParams.companyId = company.id;
    capParams.groupId = capitalGroupId;
    capParams.name = "Proprietor Capital";
    capParams.openingBalance = domain::ExactDecimal::fromString("100000.00");
    capParams.openingBalanceType = domain::BalanceType::Cr;
    auto lCapital = ledgerService.createLedger(capParams);

    // Additional Operational Ledgers
    services::CreateLedgerParams hdfcParams;
    hdfcParams.companyId = company.id;
    hdfcParams.groupId = bankAccountsId;
    hdfcParams.name = "HDFC Bank A/c";
    auto lHdfc = ledgerService.createLedger(hdfcParams);

    services::CreateLedgerParams salesParams;
    salesParams.companyId = company.id;
    salesParams.groupId = salesGroupId;
    salesParams.name = "Domestic Sales";
    auto lSales = ledgerService.createLedger(salesParams);

    services::CreateLedgerParams powerParams;
    powerParams.companyId = company.id;
    powerParams.groupId = factoryPowerGroupId;
    powerParams.name = "Factory Electricity A/c";
    auto lPower = ledgerService.createLedger(powerParams);

    services::CreateLedgerParams statParams;
    statParams.companyId = company.id;
    statParams.groupId = officeExpGroupId;
    statParams.name = "Printing & Stationery A/c";
    auto lStat = ledgerService.createLedger(statParams);

    auto cashLedger = ledgerService.getLedgerByName(company.id, "Cash");
    assert(cashLedger.has_value());

    // 4. Verify Initial Trial Balance (Nested Groups Double-Counting Check)
    auto tbInit = reportService.generateTrialBalance(company.id, "2026-04-01", "2026-04-30", true);
    std::cout << "Initial TB Total Closing Debit: " << tbInit.totalClosingDebit.toString()
              << ", Total Closing Credit: " << tbInit.totalClosingCredit.toString() << std::endl;

    // Grand total closing debit MUST be exactly 100,000 (Gamma 20k + Beta 30k + Alpha 50k)
    // Parent groups MUST NOT be double counted into the grand total!
    assert(tbInit.totalClosingDebit == domain::ExactDecimal::fromString("100000.00"));
    assert(tbInit.totalClosingCredit == domain::ExactDecimal::fromString("100000.00"));
    assert(tbInit.isReconciled == true);
    assert(tbInit.discrepancy == domain::ExactDecimal::zero());

    // Verify hierarchical aggregation in TB tree
    std::function<void(const services::TrialBalanceGroupNode&)> inspectNode =
        [&](const services::TrialBalanceGroupNode& node) {
        if (node.groupId == debtorsL4) {
            // Level 4: Retail North = Alpha (50k)
            assert(node.closingDebit == domain::ExactDecimal::fromString("50000.00"));
        } else if (node.groupId == debtorsL3) {
            // Level 3: North Region = Beta (30k) + Retail North (50k) = 80k
            assert(node.closingDebit == domain::ExactDecimal::fromString("80000.00"));
        } else if (node.groupId == debtorsL2) {
            // Level 2: Sundry Debtors = Gamma (20k) + North Region (80k) = 100k
            assert(node.closingDebit == domain::ExactDecimal::fromString("100000.00"));
        }
        for (const auto& sub : node.subGroups) inspectNode(sub);
    };
    for (const auto& root : tbInit.groups) inspectNode(root);
    std::cout << "[PASS] Nested Group Trial Balance rollup verified with ZERO double-counting!" << std::endl;

    // Resolve voucher types
    auto vTypes = companyService.listVoucherTypes(company.id);
    std::string vtypeReceipt, vtypeContra, vtypePayment, vtypeSales, vtypeJournal;
    for (const auto& vt : vTypes) {
        if (vt.category == domain::VoucherCategory::Receipt) vtypeReceipt = vt.id;
        else if (vt.category == domain::VoucherCategory::Contra) vtypeContra = vt.id;
        else if (vt.category == domain::VoucherCategory::Payment) vtypePayment = vt.id;
        else if (vt.category == domain::VoucherCategory::Sales) vtypeSales = vt.id;
        else if (vt.category == domain::VoucherCategory::Journal) vtypeJournal = vt.id;
    }

    // 5. Test Accounting Operations 1 to 8

    // Operation 1: Capital Introduced (Receipt)
    // Dr HDFC Bank ₹5,00,000, Cr Proprietor Capital ₹5,00,000
    domain::Voucher v1;
    v1.companyId = company.id;
    v1.voucherTypeId = vtypeReceipt;
    v1.voucherNumber = "RCP-001";
    v1.date = "2026-04-01";
    v1.narration = "Fresh capital infusion by partner";
    v1.entries = {
        {"", "", lHdfc.id, domain::BalanceType::Dr, domain::ExactDecimal::fromString("500000.00"), std::nullopt, 1, ""},
        {"", "", lCapital.id, domain::BalanceType::Cr, domain::ExactDecimal::fromString("500000.00"), std::nullopt, 2, ""}
    };
    voucherService.postVoucher(v1);
    assert(ledgerService.getLedger(lHdfc.id)->currentBalance == domain::ExactDecimal::fromString("500000.00"));
    std::cout << "[PASS] Op 1: Capital introduced posted successfully." << std::endl;

    // Operation 2: Bank Deposit / Cash Withdrawal (Contra)
    // Dr Cash ₹50,000, Cr HDFC Bank ₹50,000
    domain::Voucher v2;
    v2.companyId = company.id;
    v2.voucherTypeId = vtypeContra;
    v2.voucherNumber = "CNT-001";
    v2.date = "2026-04-02";
    v2.narration = "Cash withdrawn for office petty expenses";
    v2.entries = {
        {"", "", cashLedger->id, domain::BalanceType::Dr, domain::ExactDecimal::fromString("50000.00"), std::nullopt, 1, ""},
        {"", "", lHdfc.id, domain::BalanceType::Cr, domain::ExactDecimal::fromString("50000.00"), std::nullopt, 2, ""}
    };
    voucherService.postVoucher(v2);
    assert(ledgerService.getLedger(cashLedger->id)->currentBalance == domain::ExactDecimal::fromString("50000.00"));
    assert(ledgerService.getLedger(lHdfc.id)->currentBalance == domain::ExactDecimal::fromString("450000.00"));
    std::cout << "[PASS] Op 2: Contra voucher posted successfully." << std::endl;

    // Operation 3: Payment Voucher
    // Dr Factory Electricity ₹15,000, Dr Printing & Stationery ₹5,000, Cr HDFC Bank ₹20,000
    domain::Voucher v3;
    v3.companyId = company.id;
    v3.voucherTypeId = vtypePayment;
    v3.voucherNumber = "PMT-001";
    v3.date = "2026-04-05";
    v3.narration = "Payment of factory electricity bill and stationery expenses";
    v3.entries = {
        {"", "", lPower.id, domain::BalanceType::Dr, domain::ExactDecimal::fromString("15000.00"), std::nullopt, 1, ""},
        {"", "", lStat.id, domain::BalanceType::Dr, domain::ExactDecimal::fromString("5000.00"), std::nullopt, 2, ""},
        {"", "", lHdfc.id, domain::BalanceType::Cr, domain::ExactDecimal::fromString("20000.00"), std::nullopt, 3, ""}
    };
    voucherService.postVoucher(v3);
    assert(ledgerService.getLedger(lPower.id)->currentBalance == domain::ExactDecimal::fromString("15000.00"));
    assert(ledgerService.getLedger(lStat.id)->currentBalance == domain::ExactDecimal::fromString("5000.00"));
    assert(ledgerService.getLedger(lHdfc.id)->currentBalance == domain::ExactDecimal::fromString("430000.00"));
    std::cout << "[PASS] Op 3: Multi-entry Payment voucher posted successfully." << std::endl;

    // Operation 4: Sales Voucher
    // Dr Debtor Alpha L4 ₹75,000, Cr Domestic Sales ₹75,000
    domain::Voucher v4;
    v4.companyId = company.id;
    v4.voucherTypeId = vtypeSales;
    v4.voucherNumber = "SAL-001";
    v4.date = "2026-04-10";
    v4.narration = "Goods sold on credit to Debtor Alpha";
    v4.entries = {
        {"", "", lAlpha.id, domain::BalanceType::Dr, domain::ExactDecimal::fromString("75000.00"), std::nullopt, 1, ""},
        {"", "", lSales.id, domain::BalanceType::Cr, domain::ExactDecimal::fromString("75000.00"), std::nullopt, 2, ""}
    };
    voucherService.postVoucher(v4);
    // Debtor Alpha: 50,000 opening + 75,000 sale = 1,25,000 Dr
    assert(ledgerService.getLedger(lAlpha.id)->currentBalance == domain::ExactDecimal::fromString("125000.00"));
    assert(ledgerService.getLedger(lSales.id)->currentBalance == domain::ExactDecimal::fromString("75000.00"));
    std::cout << "[PASS] Op 4: Sales voucher posted successfully." << std::endl;

    // Operation 5: Receipt Voucher
    // Dr HDFC Bank ₹40,000, Cr Debtor Alpha L4 ₹40,000
    domain::Voucher v5;
    v5.companyId = company.id;
    v5.voucherTypeId = vtypeReceipt;
    v5.voucherNumber = "RCP-002";
    v5.date = "2026-04-15";
    v5.narration = "Part payment received from Debtor Alpha";
    v5.entries = {
        {"", "", lHdfc.id, domain::BalanceType::Dr, domain::ExactDecimal::fromString("40000.00"), std::nullopt, 1, ""},
        {"", "", lAlpha.id, domain::BalanceType::Cr, domain::ExactDecimal::fromString("40000.00"), std::nullopt, 2, ""}
    };
    voucherService.postVoucher(v5);
    // Debtor Alpha: 125,000 - 40,000 = 85,000 Dr
    assert(ledgerService.getLedger(lAlpha.id)->currentBalance == domain::ExactDecimal::fromString("85000.00"));
    assert(ledgerService.getLedger(lHdfc.id)->currentBalance == domain::ExactDecimal::fromString("470000.00"));
    std::cout << "[PASS] Op 5: Receipt voucher posted successfully." << std::endl;

    // Operation 6: Journal Voucher
    // Dr Printing & Stationery ₹2,000, Cr Cash ₹2,000
    domain::Voucher v6;
    v6.companyId = company.id;
    v6.voucherTypeId = vtypeJournal;
    v6.voucherNumber = "JRN-001";
    v6.date = "2026-04-20";
    v6.narration = "Office stationery purchased from petty cash";
    v6.entries = {
        {"", "", lStat.id, domain::BalanceType::Dr, domain::ExactDecimal::fromString("2000.00"), std::nullopt, 1, ""},
        {"", "", cashLedger->id, domain::BalanceType::Cr, domain::ExactDecimal::fromString("2000.00"), std::nullopt, 2, ""}
    };
    voucherService.postVoucher(v6);
    // Printing & Stationery: 5,000 + 2,000 = 7,000 Dr
    assert(ledgerService.getLedger(lStat.id)->currentBalance == domain::ExactDecimal::fromString("7000.00"));
    assert(ledgerService.getLedger(cashLedger->id)->currentBalance == domain::ExactDecimal::fromString("48000.00"));
    std::cout << "[PASS] Op 6: Journal voucher posted successfully." << std::endl;

    // Operation 7: Voucher Reversal
    domain::Voucher vRevTest;
    vRevTest.companyId = company.id;
    vRevTest.voucherTypeId = vtypeJournal;
    vRevTest.voucherNumber = "JRN-REV-TEST";
    vRevTest.date = "2026-04-22";
    vRevTest.narration = "Temporary entry for reversal testing";
    vRevTest.entries = {
        {"", "", lStat.id, domain::BalanceType::Dr, domain::ExactDecimal::fromString("3000.00"), std::nullopt, 1, ""},
        {"", "", cashLedger->id, domain::BalanceType::Cr, domain::ExactDecimal::fromString("3000.00"), std::nullopt, 2, ""}
    };
    auto postedRevTest = voucherService.postVoucher(vRevTest);
    assert(ledgerService.getLedger(lStat.id)->currentBalance == domain::ExactDecimal::fromString("10000.00"));

    domain::ReverseVoucherParams revParams;
    revParams.companyId = company.id;
    revParams.voucherId = postedRevTest.id;
    revParams.reversalDate = "2026-04-23";
    revParams.reason = "Erroneous posting reversed";
    auto revResult = voucherService.reverseVoucher(revParams);
    assert(revResult.entries.size() == 2);
    // Balance restored back to 7,000
    assert(ledgerService.getLedger(lStat.id)->currentBalance == domain::ExactDecimal::fromString("7000.00"));
    std::cout << "[PASS] Op 7: Voucher reversal executed and balance cleanly restored." << std::endl;

    // Operation 8: Voucher Cancellation
    domain::Voucher vCancelTest;
    vCancelTest.companyId = company.id;
    vCancelTest.voucherTypeId = vtypePayment;
    vCancelTest.voucherNumber = "PMT-CANCEL-TEST";
    vCancelTest.date = "2026-04-24";
    vCancelTest.narration = "Test payment to be cancelled";
    vCancelTest.entries = {
        {"", "", lPower.id, domain::BalanceType::Dr, domain::ExactDecimal::fromString("5000.00"), std::nullopt, 1, ""},
        {"", "", cashLedger->id, domain::BalanceType::Cr, domain::ExactDecimal::fromString("5000.00"), std::nullopt, 2, ""}
    };
    auto postedCancelTest = voucherService.postVoucher(vCancelTest);
    assert(ledgerService.getLedger(lPower.id)->currentBalance == domain::ExactDecimal::fromString("20000.00"));

    bool cancelOk = voucherService.cancelVoucher(company.id, postedCancelTest.id, "Duplicate entry");
    assert(cancelOk == true);
    // Factory Power balance must roll back to 15,000
    assert(ledgerService.getLedger(lPower.id)->currentBalance == domain::ExactDecimal::fromString("15000.00"));
    assert(ledgerService.getLedger(cashLedger->id)->currentBalance == domain::ExactDecimal::fromString("48000.00"));

    // Verify Day Book treatment of Cancelled Voucher
    services::DayBookFilter dbf;
    dbf.companyId = company.id;
    dbf.fromDate = "2026-04-01";
    dbf.toDate = "2026-04-30";
    auto dayBookResult = dayBookService.getDayBook(dbf);

    bool foundCancelled = false;
    for (const auto& dbi : dayBookResult.items) {
        if (dbi.id == postedCancelTest.id) {
            foundCancelled = true;
            assert(dbi.status == domain::VoucherStatus::Cancelled);
            assert(dbi.particulars.rfind("[CANCELLED]", 0) == 0);
        }
    }
    assert(foundCancelled == true);
    std::cout << "[PASS] Op 8: Voucher cancellation verified and Day Book [CANCELLED] tag confirmed." << std::endl;

    // 6. Test Unbalanced and Invalid Vouchers
    try {
        domain::Voucher vBad;
        vBad.companyId = company.id;
        vBad.voucherTypeId = vtypeJournal;
        vBad.voucherNumber = "BAD-001";
        vBad.date = "2026-04-25";
        vBad.entries = {
            {"", "", lPower.id, domain::BalanceType::Dr, domain::ExactDecimal::fromString("1000.00"), std::nullopt, 1, ""},
            {"", "", cashLedger->id, domain::BalanceType::Cr, domain::ExactDecimal::fromString("999.00"), std::nullopt, 2, ""}
        };
        voucherService.postVoucher(vBad);
        assert(false && "Unbalanced voucher must be rejected!");
    } catch (const std::exception& e) {
        std::cout << "[PASS] Unbalanced voucher correctly rejected: " << e.what() << std::endl;
    }

    try {
        domain::Voucher vEmpty;
        vEmpty.companyId = company.id;
        vEmpty.voucherTypeId = vtypeJournal;
        vEmpty.voucherNumber = "BAD-002";
        vEmpty.date = "2026-04-25";
        voucherService.postVoucher(vEmpty);
        assert(false && "Empty voucher entries must be rejected!");
    } catch (const std::exception& e) {
        std::cout << "[PASS] Empty voucher entries correctly rejected: " << e.what() << std::endl;
    }

    // 7. Verify Profit & Loss Report Correctness with Nested Groups
    // Direct Income (Sales): ₹75,000
    // Direct Expense (Factory Electricity in nested group): ₹15,000
    // Gross Profit: ₹75,000 - ₹15,000 = ₹60,000
    // Indirect Expense (Stationery in nested group): ₹7,000
    // Net Profit: ₹60,000 - ₹7,000 = ₹53,000
    auto pnl = reportService.generateProfitAndLoss(company.id, "2026-04-01", "2026-04-30");
    std::cout << "P&L Direct Income Total: " << pnl.tradingAccount.directIncomeTotal.toString()
              << ", Direct Expense Total: " << pnl.tradingAccount.directExpensesTotal.toString()
              << ", Gross Profit: " << pnl.tradingAccount.grossProfit.toString() << std::endl;

    assert(pnl.tradingAccount.directIncomeTotal == domain::ExactDecimal::fromString("75000.00"));
    assert(pnl.tradingAccount.directExpensesTotal == domain::ExactDecimal::fromString("15000.00"));
    assert(pnl.tradingAccount.isGrossProfit == true);
    assert(pnl.tradingAccount.grossProfit == domain::ExactDecimal::fromString("60000.00"));

    std::cout << "P&L Indirect Expense Total: " << pnl.operatingAccount.indirectExpensesTotal.toString()
              << ", Net Profit: " << pnl.operatingAccount.netProfit.toString() << std::endl;
    assert(pnl.operatingAccount.indirectExpensesTotal == domain::ExactDecimal::fromString("7000.00"));
    assert(pnl.operatingAccount.isNetProfit == true);
    assert(pnl.operatingAccount.netProfit == domain::ExactDecimal::fromString("53000.00"));
    std::cout << "[PASS] Profit & Loss report verified with nested groups & exact gross/net profit." << std::endl;

    // 8. Verify Post-Operation Trial Balance Reconciliation & Hierarchy
    auto tbFinal = reportService.generateTrialBalance(company.id, "2026-04-01", "2026-04-30", true);
    std::cout << "Final TB Total Closing Debit: " << tbFinal.totalClosingDebit.toString()
              << ", Total Closing Credit: " << tbFinal.totalClosingCredit.toString() << std::endl;

    // Invariant: Total Closing Debit MUST EQUAL Total Closing Credit
    assert(tbFinal.totalClosingDebit == tbFinal.totalClosingCredit);
    assert(tbFinal.isReconciled == true);
    assert(tbFinal.discrepancy == domain::ExactDecimal::zero());

    // Debtor Alpha L4: 85,000 Dr
    // Debtor Beta L3: 30,000 Dr
    // Debtor Gamma L2: 20,000 Dr
    // Level 3 (North Region) rollup = 85k + 30k = 115k
    // Level 2 (Sundry Debtors) rollup = 115k + 20k = 135k
    for (const auto& root : tbFinal.groups) {
        inspectNode = [&](const services::TrialBalanceGroupNode& node) {
            if (node.groupId == debtorsL4) {
                assert(node.closingDebit == domain::ExactDecimal::fromString("85000.00"));
            } else if (node.groupId == debtorsL3) {
                assert(node.closingDebit == domain::ExactDecimal::fromString("115000.00"));
            } else if (node.groupId == debtorsL2) {
                assert(node.closingDebit == domain::ExactDecimal::fromString("135000.00"));
            }
            for (const auto& sub : node.subGroups) inspectNode(sub);
        };
        inspectNode(root);
    }
    std::cout << "[PASS] Post-operation nested Trial Balance hierarchical rollup verified." << std::endl;

    std::cout << "\n>>> ALL SCREENSHOT-BASED FUNCTIONAL & ACCOUNTING AUDIT TESTS PASSED! <<<" << std::endl;
    return 0;
}
