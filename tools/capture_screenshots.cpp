#include <QApplication>
#include <QPixmap>
#include <QDir>
#include <cstdlib>
#include <iostream>
#include <filesystem>
#include "../src/ui/shell/MainWindow.hpp"
#include "../src/ui/theme/TallyTheme.hpp"
#include "../src/services/CompanyService.hpp"
#include "../src/services/LedgerService.hpp"
#include "../src/services/VoucherService.hpp"
#include "../src/services/GroupService.hpp"
#include "../src/domain/decimal/ExactDecimal.hpp"
#include "../src/domain/voucher/Voucher.hpp"
#include "../src/domain/ledger/Ledger.hpp"

using namespace tally;

int main(int argc, char* argv[]) {
    setenv("QT_QPA_PLATFORM", "offscreen", 1);
    QApplication app(argc, argv);
    app.setStyleSheet(ui::TallyTheme::getApplicationStyleSheet());

    std::filesystem::create_directories("screenshots");
    std::filesystem::create_directories("screenshot");

    const std::string dbPath = "screenshot_demo.sqlite3";
    std::filesystem::remove(dbPath);

    ui::MainWindow window;
    if (!window.initialize(dbPath)) {
        std::cerr << "Failed to init window" << std::endl;
        return 1;
    }

    auto* cs = window.companyService();
    auto* ls = window.ledgerService();
    auto* vs = window.voucherService();
    auto* gs = window.groupService();

    auto companies = cs->listCompanies();
    if (companies.empty()) {
        std::cerr << "No company created!" << std::endl;
        return 1;
    }
    std::string compId = companies[0].id;
    std::cout << "Using company ID: " << compId << " (" << companies[0].name << ")" << std::endl;

    auto sundryDebtors = gs->getGroupByName(compId, "Sundry Debtors");
    auto bankAccounts = gs->getGroupByName(compId, "Bank Accounts");
    auto directExpenses = gs->getGroupByName(compId, "Direct Expenses");
    auto salesAccounts = gs->getGroupByName(compId, "Sales Accounts");
    auto capitalAccounts = gs->getGroupByName(compId, "Capital Account");

    if (!sundryDebtors || !bankAccounts || !directExpenses || !salesAccounts || !capitalAccounts) {
        std::cerr << "One or more default groups not found!" << std::endl;
        return 1;
    }

    // Create realistic ledgers
    services::CreateLedgerParams lpBank;
    lpBank.companyId = compId;
    lpBank.groupId = bankAccounts->id;
    lpBank.name = "HDFC Bank Current A/c";
    lpBank.openingBalance = domain::ExactDecimal::fromString("1250000.0000");
    lpBank.openingBalanceType = domain::BalanceType::Dr;
    auto bankLedger = ls->createLedger(lpBank);

    services::CreateLedgerParams lpDebtor;
    lpDebtor.companyId = compId;
    lpDebtor.groupId = sundryDebtors->id;
    lpDebtor.name = "Infosys Technologies Ltd";
    lpDebtor.openingBalance = domain::ExactDecimal::fromString("420000.0000");
    lpDebtor.openingBalanceType = domain::BalanceType::Dr;
    auto debtorLedger = ls->createLedger(lpDebtor);

    services::CreateLedgerParams lpCap;
    lpCap.companyId = compId;
    lpCap.groupId = capitalAccounts->id;
    lpCap.name = "Promoter Capital";
    lpCap.openingBalance = domain::ExactDecimal::fromString("1670000.0000");
    lpCap.openingBalanceType = domain::BalanceType::Cr;
    auto capitalLedger = ls->createLedger(lpCap);

    services::CreateLedgerParams lpSales;
    lpSales.companyId = compId;
    lpSales.groupId = salesAccounts->id;
    lpSales.name = "Software Development Revenue";
    lpSales.openingBalance = domain::ExactDecimal::zero();
    lpSales.openingBalanceType = domain::BalanceType::Cr;
    auto salesLedger = ls->createLedger(lpSales);

    services::CreateLedgerParams lpExp;
    lpExp.companyId = compId;
    lpExp.groupId = directExpenses->id;
    lpExp.name = "Cloud Server Hosting (AWS)";
    lpExp.openingBalance = domain::ExactDecimal::zero();
    lpExp.openingBalanceType = domain::BalanceType::Dr;
    auto expLedger = ls->createLedger(lpExp);

    auto vtypes = cs->listVoucherTypes(compId);
    std::string payVTypeId, rcpVTypeId, salVTypeId;
    for (const auto& vt : vtypes) {
        if (vt.category == domain::VoucherCategory::Payment) payVTypeId = vt.id;
        if (vt.category == domain::VoucherCategory::Receipt) rcpVTypeId = vt.id;
        if (vt.category == domain::VoucherCategory::Sales) salVTypeId = vt.id;
    }

    if (!payVTypeId.empty()) {
        domain::Voucher payVch;
        payVch.companyId = compId;
        payVch.voucherTypeId = payVTypeId;
        payVch.voucherNumber = vs->getNextVoucherNumber(compId, payVTypeId);
        payVch.date = "2026-04-10";
        payVch.narration = "Payment for AWS production cluster hosting - April 2026";
        domain::VoucherEntry e1{ "", "", expLedger.id, domain::BalanceType::Dr, domain::ExactDecimal::fromString("85000.0000"), "", 1, "" };
        domain::VoucherEntry e2{ "", "", bankLedger.id, domain::BalanceType::Cr, domain::ExactDecimal::fromString("85000.0000"), "", 2, "" };
        payVch.entries = {e1, e2};
        vs->postVoucher(payVch);
    }

    if (!salVTypeId.empty()) {
        domain::Voucher salVch;
        salVch.companyId = compId;
        salVch.voucherTypeId = salVTypeId;
        salVch.voucherNumber = vs->getNextVoucherNumber(compId, salVTypeId);
        salVch.date = "2026-04-15";
        salVch.narration = "Invoice for Enterprise Cloud Migration Services";
        domain::VoucherEntry e1{ "", "", debtorLedger.id, domain::BalanceType::Dr, domain::ExactDecimal::fromString("550000.0000"), "", 1, "" };
        domain::VoucherEntry e2{ "", "", salesLedger.id, domain::BalanceType::Cr, domain::ExactDecimal::fromString("550000.0000"), "", 2, "" };
        salVch.entries = {e1, e2};
        vs->postVoucher(salVch);
    }

    if (!rcpVTypeId.empty()) {
        domain::Voucher rcpVch;
        rcpVch.companyId = compId;
        rcpVch.voucherTypeId = rcpVTypeId;
        rcpVch.voucherNumber = vs->getNextVoucherNumber(compId, rcpVTypeId);
        rcpVch.date = "2026-04-20";
        rcpVch.narration = "Cheque received against Invoice #INV-2026-001";
        domain::VoucherEntry e1{ "", "", bankLedger.id, domain::BalanceType::Dr, domain::ExactDecimal::fromString("300000.0000"), "", 1, "" };
        domain::VoucherEntry e2{ "", "", debtorLedger.id, domain::BalanceType::Cr, domain::ExactDecimal::fromString("300000.0000"), "", 2, "" };
        rcpVch.entries = {e1, e2};
        vs->postVoucher(rcpVch);
    }

    window.switchActiveCompany(compId);
    window.resize(1280, 800);
    window.show();
    app.processEvents();

    auto capture = [&](const std::string& name, application::ScreenId screen) {
        window.navigateToScreen(screen);
        app.processEvents();
        QPixmap pixmap(window.size());
        window.render(&pixmap);
        pixmap.save(QString::fromStdString("screenshots/" + name + ".png"));
        pixmap.save(QString::fromStdString("screenshot/" + name + ".png"));
        std::cout << "Captured: " << name << std::endl;
    };

    capture("01_gateway_of_tally", application::ScreenId::Gateway);
    capture("02_accounting_voucher_entry", application::ScreenId::VoucherEntry);
    capture("03_day_book", application::ScreenId::DayBook);
    capture("04_trial_balance", application::ScreenId::TrialBalance);
    capture("05_profit_and_loss", application::ScreenId::ProfitLoss);
    capture("06_balance_sheet", application::ScreenId::BalanceSheet);
    capture("07_group_summary", application::ScreenId::GroupSummary);

    std::filesystem::remove(dbPath);
    std::cout << "All screenshots captured successfully!" << std::endl;
    return 0;
}
