#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QLabel>
#include <QPushButton>
#include <memory>
#include "../../database/DatabaseManager.hpp"
#include "../../services/CompanyService.hpp"
#include "../../services/LedgerService.hpp"
#include "../../services/VoucherService.hpp"
#include "../../services/DayBookService.hpp"
#include "../../services/ReportService.hpp"
#include "../../services/GroupService.hpp"
#include "../../application/NavigationManager.hpp"
#include "../gateway/GatewayView.hpp"
#include "../vouchers/VoucherEntryView.hpp"
#include "../reports/DayBookView.hpp"
#include "../reports/TrialBalanceView.hpp"
#include "../reports/ProfitLossView.hpp"
#include "../reports/BalanceSheetView.hpp"
#include "../reports/LedgerStatementView.hpp"
#include "../reports/GroupSummaryView.hpp"

namespace tally::ui {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    bool initialize(const std::string& dbPath = ":memory:");
    void switchActiveCompany(const std::string& companyId);
    void navigateToScreen(application::ScreenId screen, const std::string& param = "");

    services::CompanyService* companyService() const { return m_companyService.get(); }
    services::LedgerService* ledgerService() const { return m_ledgerService.get(); }
    services::VoucherService* voucherService() const { return m_voucherService.get(); }
    services::GroupService* groupService() const { return m_groupService.get(); }

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onScreenChanged(application::ScreenId newScreen, const std::string& param);
    void onOpenGoToDialog();
    void onCompanyClicked();
    void onDateClicked();
    void onBackupRestoreClicked();
    void onExchangeClicked();
    void onImportClicked();
    void onExportClicked();
    void onMasterCreationClicked();
    void onMasterAlterationClicked();
    void onShutCompany();

private:
    void setupUi();
    void setupTopBar();
    void setupRightSidebar();
    void setupBottomBar();

    std::unique_ptr<database::DatabaseManager> m_dbManager;
    std::unique_ptr<services::CompanyService> m_companyService;
    std::unique_ptr<services::GroupService> m_groupService;
    std::unique_ptr<services::LedgerService> m_ledgerService;
    std::unique_ptr<services::VoucherService> m_voucherService;
    std::unique_ptr<services::DayBookService> m_dayBookService;
    std::unique_ptr<services::ReportService> m_reportService;

    application::NavigationManager m_navManager;
    std::string m_currentCompanyId;
    std::string m_currentDate = "2026-04-01";
    std::string m_fromDate = "2026-04-01";
    std::string m_toDate = "2027-03-31";

    QStackedWidget* m_stackedWidget;
    GatewayView* m_gatewayView;
    VoucherEntryView* m_voucherEntryView;
    DayBookView* m_dayBookView;
    TrialBalanceView* m_trialBalanceView;
    ProfitLossView* m_profitLossView;
    BalanceSheetView* m_balanceSheetView;
    LedgerStatementView* m_ledgerStatementView;
    GroupSummaryView* m_groupSummaryView;

    QLabel* m_topCompanyLabel;
    QLabel* m_bottomStatusLabel;
};

} // namespace tally::ui
