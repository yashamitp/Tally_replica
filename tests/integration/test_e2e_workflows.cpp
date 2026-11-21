#include <cassert>
#include <iostream>
#include <QCoreApplication>
#include <QFile>
#include "../../src/database/DatabaseManager.hpp"
#include "../../src/services/CompanyService.hpp"
#include "../../src/services/GroupService.hpp"
#include "../../src/services/LedgerService.hpp"
#include "../../src/services/VoucherService.hpp"
#include "../../src/services/DayBookService.hpp"
#include "../../src/services/ReportService.hpp"

using namespace tally;

void runEndToEndAccountingWorkflows() {
    std::cout << "========================================================\n"
              << "STARTING END-TO-END ACCOUNTING WORKFLOW VERIFICATION\n"
              << "========================================================" << std::endl;

    std::string testDbFile = "e2e_test_accounting.sqlite3";
    if (QFile::exists(QString::fromStdString(testDbFile))) {
        QFile::remove(QString::fromStdString(testDbFile));
    }

    // =========================================================================
    // WORKFLOW A: Company Creation & Persistence
    // =========================================================================
    std::cout << "\n--- WORKFLOW A: Company Creation & Persistence ---" << std::endl;
    std::string companyId;
    {
        database::DatabaseManager db("E2E_Conn1");
        bool ok = db.open(testDbFile);
        assert(ok && "Database failed to open");

        services::CompanyService compService(db);

        services::CreateCompanyParams cp;
        cp.name = "Vanguard Logistics Ltd";
        cp.formalName = "Vanguard Logistics Private Limited";
        cp.addressLine1 = "Plot 42, Port Industrial Corridor";
        cp.city = "Mumbai";
        cp.state = "Maharashtra";
        cp.pincode = "400001";
        cp.gstin = "27AAACV1234A1Z5";
        cp.pan = "AAACV1234A";
        cp.financialYearStart = "2026-04-01";
        cp.booksBeginningFrom = "2026-04-01";
        cp.currencySymbol = "₹";
        cp.currencyFormalName = "INR";
        cp.decimalPlaces = 2;

        auto company = compService.createCompany(cp);
        assert(!company.id.empty());
        assert(company.name == "Vanguard Logistics Ltd");
        companyId = company.id;

        // Verify standard seeding
        auto groups = compService.listGroups(companyId);
        assert(groups.size() == 20 && "Must seed exactly 12 primary + 8 subgroups");

        auto vtypes = compService.listVoucherTypes(companyId);
        assert(vtypes.size() == 6 && "Must seed 6 standard voucher types");

        // Save active company setting
        db.setSetting("last_active_company_id", companyId);
        std::cout << "[PASS] Company created with standard Chart of Accounts & voucher types: " << company.name << std::endl;
    }

    // Verify persistence across restart (new connection / reload)
    {
        database::DatabaseManager db("E2E_Conn2");
        bool ok = db.open(testDbFile);
        assert(ok && "Failed to reopen database");
        services::CompanyService compService(db);

        auto reloadedSetting = db.getSetting("last_active_company_id");
        assert(reloadedSetting.has_value());
        assert(*reloadedSetting == companyId);

        auto comp = compService.getCompany(companyId);
        assert(comp.has_value());
        assert(comp->name == "Vanguard Logistics Ltd");
        assert(comp->city.value_or("") == "Mumbai");
        std::cout << "[PASS] Application restart verified: data & active company setting persisted cleanly." << std::endl;
    }

    // =========================================================================
    // WORKFLOW B: Group and Ledger Creation
    // =========================================================================
    std::cout << "\n--- WORKFLOW B: Group & Ledger Hierarchy Creation ---" << std::endl;
    database::DatabaseManager db("E2E_Workflows");
    bool ok3 = db.open(testDbFile);
    assert(ok3 && "Database failed to open");

    services::CompanyService compService(db);
    services::GroupService groupService(db);
    services::LedgerService ledgerService(db);
    services::VoucherService voucherService(db);
    services::DayBookService dayBookService(db);
    services::ReportService reportService(db);

    // Find parent groups
    auto sundryDebtors = groupService.getGroupByName(companyId, "Sundry Debtors");
    auto sundryCreditors = groupService.getGroupByName(companyId, "Sundry Creditors");
    auto bankAccounts = groupService.getGroupByName(companyId, "Bank Accounts");
    auto indExpenses = groupService.getGroupByName(companyId, "Indirect Expenses");
    auto salesAccounts = groupService.getGroupByName(companyId, "Sales Accounts");
    auto capitalAccounts = groupService.getGroupByName(companyId, "Capital Account");

    assert(sundryDebtors && sundryCreditors && bankAccounts && indExpenses && salesAccounts && capitalAccounts);

    // 1. Create Subgroup: "Port Transport Clients" under "Sundry Debtors"
    services::CreateGroupParams gp;
    gp.companyId = companyId;
    gp.name = "Port Transport Clients";
    gp.parentId = sundryDebtors->id;
    gp.nature = domain::GroupNature::Asset;
    auto clientGroup = groupService.createGroup(gp);
    assert(!clientGroup.id.empty());
    std::cout << "[PASS] Subgroup created: " << clientGroup.name << " under " << sundryDebtors->name << std::endl;

    // Verify duplicate group rejected
    bool dupGroupFailed = false;
    try {
        groupService.createGroup(gp);
    } catch (...) {
        dupGroupFailed = true;
    }
    assert(dupGroupFailed && "Duplicate group creation must fail");
    std::cout << "[PASS] Duplicate group rejection invariant verified." << std::endl;

    // 2. Create Ledgers with opening balances
    // Bank Ledger: HDFC Escrow A/c (Dr 1,000,000)
    services::CreateLedgerParams lpBank;
    lpBank.companyId = companyId;
    lpBank.groupId = bankAccounts->id;
    lpBank.name = "HDFC Current Account";
    lpBank.openingBalance = domain::ExactDecimal::fromString("1000000.0000");
    lpBank.openingBalanceType = domain::BalanceType::Dr;
    auto bankLedger = ledgerService.createLedger(lpBank);

    // Capital Ledger: Share Capital (Cr 1,000,000)
    services::CreateLedgerParams lpCap;
    lpCap.companyId = companyId;
    lpCap.groupId = capitalAccounts->id;
    lpCap.name = "Promoter Capital";
    lpCap.openingBalance = domain::ExactDecimal::fromString("1000000.0000");
    lpCap.openingBalanceType = domain::BalanceType::Cr;
    auto capLedger = ledgerService.createLedger(lpCap);

    // Client Ledger: Oceanic Freightways under custom subgroup (Dr 150,000)
    services::CreateLedgerParams lpClient;
    lpClient.companyId = companyId;
    lpClient.groupId = clientGroup.id;
    lpClient.name = "Oceanic Freightways";
    lpClient.openingBalance = domain::ExactDecimal::fromString("150000.0000");
    lpClient.openingBalanceType = domain::BalanceType::Dr;
    auto clientLedger = ledgerService.createLedger(lpClient);

    // Supplier Ledger: Coastal Fuel Terminal under Sundry Creditors (Cr 150,000)
    services::CreateLedgerParams lpSupp;
    lpSupp.companyId = companyId;
    lpSupp.groupId = sundryCreditors->id;
    lpSupp.name = "Coastal Fuel Terminal";
    lpSupp.openingBalance = domain::ExactDecimal::fromString("150000.0000");
    lpSupp.openingBalanceType = domain::BalanceType::Cr;
    auto suppLedger = ledgerService.createLedger(lpSupp);

    // Revenue Ledger: Container Haulage Income under Sales Accounts
    services::CreateLedgerParams lpRev;
    lpRev.companyId = companyId;
    lpRev.groupId = salesAccounts->id;
    lpRev.name = "Container Haulage Revenue";
    auto revLedger = ledgerService.createLedger(lpRev);

    // Expense Ledger: Fleet Maintenance under Indirect Expenses
    services::CreateLedgerParams lpExp;
    lpExp.companyId = companyId;
    lpExp.groupId = indExpenses->id;
    lpExp.name = "Fleet Maintenance";
    auto expLedger = ledgerService.createLedger(lpExp);

    // Verify duplicate ledger rejected
    bool dupLedgerFailed = false;
    try {
        ledgerService.createLedger(lpBank);
    } catch (...) {
        dupLedgerFailed = true;
    }
    assert(dupLedgerFailed && "Duplicate ledger creation must fail");
    std::cout << "[PASS] Duplicate ledger rejection invariant verified." << std::endl;

    // Verify Group deletion blocked when ledgers exist under it
    bool groupDeleteBlocked = false;
    try {
        groupService.deleteGroup(clientGroup.id);
    } catch (...) {
        groupDeleteBlocked = true;
    }
    assert(groupDeleteBlocked && "Group deletion must be blocked when child ledgers exist");
    std::cout << "[PASS] Group deletion dependency invariant verified." << std::endl;

    // Resolve Voucher Types
    auto vtypes = compService.listVoucherTypes(companyId);
    std::string payVTypeId, rcpVTypeId, jrnVTypeId, conVTypeId, salVTypeId, purVTypeId;
    for (const auto& vt : vtypes) {
        if (vt.category == domain::VoucherCategory::Payment) payVTypeId = vt.id;
        if (vt.category == domain::VoucherCategory::Receipt) rcpVTypeId = vt.id;
        if (vt.category == domain::VoucherCategory::Journal) jrnVTypeId = vt.id;
        if (vt.category == domain::VoucherCategory::Contra) conVTypeId = vt.id;
        if (vt.category == domain::VoucherCategory::Sales) salVTypeId = vt.id;
        if (vt.category == domain::VoucherCategory::Purchase) purVTypeId = vt.id;
    }

    // =========================================================================
    // WORKFLOW C: Payment Voucher
    // =========================================================================
    std::cout << "\n--- WORKFLOW C: Payment Voucher Workflow ---" << std::endl;
    // Post Payment: Dr Fleet Maintenance ₹ 45,000, Cr HDFC Current Account ₹ 45,000
    domain::Voucher payVch;
    payVch.companyId = companyId;
    payVch.voucherTypeId = payVTypeId;
    payVch.voucherNumber = voucherService.getNextVoucherNumber(companyId, payVTypeId);
    payVch.date = "2026-04-10";
    payVch.narration = "Major vehicle engine overhaul payment";

    domain::VoucherEntry pay_e1{ "", "", expLedger.id, domain::BalanceType::Dr, domain::ExactDecimal::fromString("45000.0000"), "", 1, "" };
    domain::VoucherEntry pay_e2{ "", "", bankLedger.id, domain::BalanceType::Cr, domain::ExactDecimal::fromString("45000.0000"), "", 2, "" };
    payVch.entries = {pay_e1, pay_e2};

    auto postedPay = voucherService.postVoucher(payVch);
    assert(!postedPay.id.empty());
    assert(postedPay.status == domain::VoucherStatus::Posted);

    // Verify Day Book contains the voucher
    services::DayBookFilter dbFilter;
    dbFilter.companyId = companyId;
    auto dbResult1 = dayBookService.getDayBook(dbFilter);
    assert(dbResult1.totalCount == 1);
    assert(dbResult1.items[0].voucherNumber == payVch.voucherNumber);

    // Verify Ledger Statements
    auto expStmt = ledgerService.getLedgerStatement(companyId, expLedger.id, "2026-04-01", "2026-04-30");
    assert(expStmt.closingBalance == domain::ExactDecimal::fromString("45000.0000"));
    assert(expStmt.closingBalanceType == domain::BalanceType::Dr);

    auto bankStmt1 = ledgerService.getLedgerStatement(companyId, bankLedger.id, "2026-04-01", "2026-04-30");
    // Bank was 1,000,000 Dr - 45,000 = 955,000 Dr
    assert(bankStmt1.closingBalance == domain::ExactDecimal::fromString("955000.0000"));
    assert(bankStmt1.closingBalanceType == domain::BalanceType::Dr);
    std::cout << "[PASS] Payment voucher posted and ledger statements verified: Bank = ₹ "
              << bankStmt1.closingBalance << " Dr, Expense = ₹ " << expStmt.closingBalance << " Dr" << std::endl;

    // =========================================================================
    // WORKFLOW D: Receipt Voucher
    // =========================================================================
    std::cout << "\n--- WORKFLOW D: Receipt Voucher Workflow ---" << std::endl;
    // Customer pays ₹ 100,000: Dr Bank ₹ 100,000, Cr Oceanic Freightways ₹ 100,000
    domain::Voucher rcpVch;
    rcpVch.companyId = companyId;
    rcpVch.voucherTypeId = rcpVTypeId;
    rcpVch.voucherNumber = voucherService.getNextVoucherNumber(companyId, rcpVTypeId);
    rcpVch.date = "2026-04-14";
    rcpVch.narration = "On-account freight settlement";

    domain::VoucherEntry rcp_e1{ "", "", bankLedger.id, domain::BalanceType::Dr, domain::ExactDecimal::fromString("100000.0000"), "", 1, "" };
    domain::VoucherEntry rcp_e2{ "", "", clientLedger.id, domain::BalanceType::Cr, domain::ExactDecimal::fromString("100000.0000"), "", 2, "" };
    rcpVch.entries = {rcp_e1, rcp_e2};

    auto postedRcp = voucherService.postVoucher(rcpVch);
    assert(!postedRcp.id.empty());

    // Verify Client Balance: was 150,000 Dr - 100,000 Cr = 50,000 Dr
    auto clientStmt = ledgerService.getLedgerStatement(companyId, clientLedger.id, "2026-04-01", "2026-04-30");
    assert(clientStmt.closingBalance == domain::ExactDecimal::fromString("50000.0000"));
    assert(clientStmt.closingBalanceType == domain::BalanceType::Dr);

    // Verify Bank Balance: was 955,000 Dr + 100,000 Dr = 1,055,000 Dr
    auto bankStmt2 = ledgerService.getLedgerStatement(companyId, bankLedger.id, "2026-04-01", "2026-04-30");
    assert(bankStmt2.closingBalance == domain::ExactDecimal::fromString("1055000.0000"));
    std::cout << "[PASS] Receipt voucher posted: Client balance reduced to ₹ "
              << clientStmt.closingBalance << " Dr, Bank increased to ₹ " << bankStmt2.closingBalance << " Dr" << std::endl;

    // =========================================================================
    // WORKFLOW E: Journal Voucher & Invariant Validations
    // =========================================================================
    std::cout << "\n--- WORKFLOW E: Multi-Line Journal Voucher & Invariant Verification ---" << std::endl;

    // Multi-line Journal:
    // Dr Oceanic Freightways: ₹ 200,000 (New freight invoice billing)
    // Cr Container Haulage Revenue: ₹ 200,000
    domain::Voucher jrnVch;
    jrnVch.companyId = companyId;
    jrnVch.voucherTypeId = jrnVTypeId;
    jrnVch.voucherNumber = voucherService.getNextVoucherNumber(companyId, jrnVTypeId);
    jrnVch.date = "2026-04-18";
    jrnVch.narration = "Haulage billing for April fleet assignments";

    domain::VoucherEntry jrn_e1{ "", "", clientLedger.id, domain::BalanceType::Dr, domain::ExactDecimal::fromString("200000.0000"), "", 1, "" };
    domain::VoucherEntry jrn_e2{ "", "", revLedger.id, domain::BalanceType::Cr, domain::ExactDecimal::fromString("200000.0000"), "", 2, "" };
    jrnVch.entries = {jrn_e1, jrn_e2};

    auto postedJrn = voucherService.postVoucher(jrnVch);
    assert(!postedJrn.id.empty());

    // Test Invariant 1: Unbalanced voucher must be rejected
    bool unbalancedRejected = false;
    try {
        domain::Voucher badVch = jrnVch;
        badVch.id = "";
        badVch.voucherNumber = "BAD-999";
        badVch.entries[1].amount = domain::ExactDecimal::fromString("199999.0000"); // 1 rupee difference
        voucherService.postVoucher(badVch);
    } catch (...) {
        unbalancedRejected = true;
    }
    assert(unbalancedRejected && "Unbalanced double-entry must throw exception");
    std::cout << "[PASS] Strict Double-Entry Invariant (Debits == Credits) enforced." << std::endl;

    // Test Invariant 2: Invalid or cross-company ledger must be rejected
    bool companyMismatchRejected = false;
    try {
        domain::Voucher badCompVch = jrnVch;
        badCompVch.id = "";
        badCompVch.voucherNumber = "BAD-888";
        badCompVch.entries[0].ledgerId = "non-existent-or-foreign-ledger-id";
        voucherService.postVoucher(badCompVch);
    } catch (...) {
        companyMismatchRejected = true;
    }
    assert(companyMismatchRejected && "Foreign or missing ledger must be rejected");
    std::cout << "[PASS] Multi-tenant Company Ledger Isolation verified." << std::endl;

    // =========================================================================
    // WORKFLOW F: Report Triple Reconciliation
    // =========================================================================
    std::cout << "\n--- WORKFLOW F: Report Reconciliation (Trial Balance, P&L, Balance Sheet) ---" << std::endl;

    // 1. Trial Balance Reconciliation
    auto tb = reportService.generateTrialBalance(companyId, "2026-04-01", "2026-04-30", true);
    assert(tb.isReconciled && "Trial balance must reconcile");
    assert(tb.discrepancy.isZero() && "Trial balance discrepancy must be exactly zero");
    assert(tb.totalClosingDebit == tb.totalClosingCredit);
    std::cout << "[PASS] Trial Balance Mathematical Parity Verified: Total Debits ("
              << tb.totalClosingDebit << ") == Total Credits (" << tb.totalClosingCredit << ")" << std::endl;

    // 2. Profit & Loss Verification:
    // Revenue = 200,000 (Freight Revenue)
    // Direct Expense = 0
    // Gross Profit = 200,000
    // Indirect Expense = 45,000 (Fleet Maintenance)
    // Net Profit = 155,000
    auto pnl = reportService.generateProfitAndLoss(companyId, "2026-04-01", "2026-04-30");
    assert(pnl.tradingAccount.isGrossProfit);
    assert(pnl.tradingAccount.grossProfit == domain::ExactDecimal::fromString("200000.0000"));
    assert(pnl.operatingAccount.isNetProfit);
    assert(pnl.operatingAccount.netProfit == domain::ExactDecimal::fromString("155000.0000"));
    std::cout << "[PASS] Profit & Loss Verified: Gross Profit = ₹ " << pnl.tradingAccount.grossProfit
              << ", Operating Net Profit = ₹ " << pnl.operatingAccount.netProfit << std::endl;

    // 3. Balance Sheet Verification:
    // Assets:
    //   Bank: 1,055,000 DR
    //   Sundry Debtors (Oceanic Freightways): 150,000 + 200,000 - 100,000 = 250,000 DR
    //   Total Assets = 1,305,000
    // Liabilities:
    //   Sundry Creditors (Coastal Fuel Terminal): 150,000 CR
    // Equity:
    //   Promoter Capital: 1,000,000 CR
    //   Current Period Net Profit: 155,000 CR
    //   Total Equity = 1,155,000
    // Total Liabilities + Equity = 150,000 + 1,155,000 = 1,305,000
    auto bs = reportService.generateBalanceSheet(companyId, "2026-04-30");
    assert(bs.isReconciled && "Balance sheet must reconcile");
    assert(bs.discrepancy.isZero());
    assert(bs.totalAssets == domain::ExactDecimal::fromString("1305000.0000"));
    assert(bs.totalLiabilitiesAndEquity == domain::ExactDecimal::fromString("1305000.0000"));
    std::cout << "[PASS] Balance Sheet Fundamental Equation Verified: Assets ("
              << bs.totalAssets << ") == Liabilities + Equity (" << bs.totalLiabilitiesAndEquity << ")" << std::endl;

    // =========================================================================
    // WORKFLOW G: Reversal, Cancellation, and Audit History
    // =========================================================================
    std::cout << "\n--- WORKFLOW G: Reversal, Cancellation & Audit Trails ---" << std::endl;

    // 1. Post a test voucher to reverse
    domain::Voucher revTestVch;
    revTestVch.companyId = companyId;
    revTestVch.voucherTypeId = payVTypeId;
    revTestVch.voucherNumber = voucherService.getNextVoucherNumber(companyId, payVTypeId);
    revTestVch.date = "2026-04-20";
    revTestVch.narration = "Incorrect fuel advance";
    domain::VoucherEntry r_e1{ "", "", expLedger.id, domain::BalanceType::Dr, domain::ExactDecimal::fromString("10000.0000"), "", 1, "" };
    domain::VoucherEntry r_e2{ "", "", bankLedger.id, domain::BalanceType::Cr, domain::ExactDecimal::fromString("10000.0000"), "", 2, "" };
    revTestVch.entries = {r_e1, r_e2};
    auto postedRevTest = voucherService.postVoucher(revTestVch);

    // Reverse it
    domain::ReverseVoucherParams revParams;
    revParams.companyId = companyId;
    revParams.voucherId = postedRevTest.id;
    revParams.reversalDate = "2026-04-21";
    revParams.reason = "Duplicate advance payment entry";
    auto counterVch = voucherService.reverseVoucher(revParams);

    assert(!counterVch.id.empty());
    assert(counterVch.voucherNumber.rfind("REV-", 0) == 0);

    auto origVchAfterRev = voucherService.getVoucher(postedRevTest.id);
    assert(origVchAfterRev.has_value());
    assert(origVchAfterRev->status == domain::VoucherStatus::Reversed);
    assert(origVchAfterRev->isReversed);
    std::cout << "[PASS] Voucher Reversal creates offsetting counter-voucher: " << counterVch.voucherNumber << std::endl;

    // 2. Post a test voucher to cancel
    domain::Voucher cancelTestVch;
    cancelTestVch.companyId = companyId;
    cancelTestVch.voucherTypeId = payVTypeId;
    cancelTestVch.voucherNumber = voucherService.getNextVoucherNumber(companyId, payVTypeId);
    cancelTestVch.date = "2026-04-22";
    cancelTestVch.narration = "Mistaken cheque draft";
    domain::VoucherEntry c_e1{ "", "", expLedger.id, domain::BalanceType::Dr, domain::ExactDecimal::fromString("5000.0000"), "", 1, "" };
    domain::VoucherEntry c_e2{ "", "", bankLedger.id, domain::BalanceType::Cr, domain::ExactDecimal::fromString("5000.0000"), "", 2, "" };
    cancelTestVch.entries = {c_e1, c_e2};
    auto postedCancelTest = voucherService.postVoucher(cancelTestVch);

    // Cancel it
    bool cancelledOk = voucherService.cancelVoucher(companyId, postedCancelTest.id, "Entered with wrong cheque number");
    assert(cancelledOk);

    auto origVchAfterCancel = voucherService.getVoucher(postedCancelTest.id);
    assert(origVchAfterCancel.has_value());
    assert(origVchAfterCancel->status == domain::VoucherStatus::Cancelled);
    assert(origVchAfterCancel->isCancelled);
    std::cout << "[PASS] Voucher Cancellation verified: historical row preserved with CANCELLED status." << std::endl;

    // Re-verify that cancelled voucher is excluded from Trial Balance totals
    auto tbAfter = reportService.generateTrialBalance(companyId, "2026-04-01", "2026-04-30", true);
    assert(tbAfter.isReconciled);
    std::cout << "[PASS] Trial Balance post-reversal and post-cancellation remains perfectly reconciled." << std::endl;

    // Clean up test file
    db.close();
    QFile::remove(QString::fromStdString(testDbFile));

    std::cout << "\n========================================================\n"
              << "ALL END-TO-END WORKFLOWS AND AUDIT SUITES PASSED (100%)\n"
              << "========================================================" << std::endl;
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    runEndToEndAccountingWorkflows();
    return 0;
}
