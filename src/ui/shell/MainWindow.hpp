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
#include "../../application/NavigationManager.hpp"
#include "../gateway/GatewayView.hpp"
#include "../vouchers/VoucherEntryView.hpp"
#include "../reports/DayBookView.hpp"
#include "../reports/TrialBalanceView.hpp"
#include "../reports/ProfitLossView.hpp"
#include "../reports/BalanceSheetView.hpp"
#include "../reports/LedgerStatementView.hpp"

namespace tally::ui {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    bool initialize(const std::string& dbPath = ":memory:");

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onScreenChanged(application::ScreenId newScreen, const std::string& param);
    void onOpenGoToDialog();

private:
    void setupUi();
    void setupTopBar();
    void setupRightSidebar();
    void setupBottomBar();

    std::unique_ptr<database::DatabaseManager> m_dbManager;
    std::unique_ptr<services::CompanyService> m_companyService;
    std::unique_ptr<services::LedgerService> m_ledgerService;
    std::unique_ptr<services::VoucherService> m_voucherService;
    std::unique_ptr<services::DayBookService> m_dayBookService;
    std::unique_ptr<services::ReportService> m_reportService;

    application::NavigationManager m_navManager;
    std::string m_currentCompanyId;

    QStackedWidget* m_stackedWidget;
    GatewayView* m_gatewayView;
    VoucherEntryView* m_voucherEntryView;
    DayBookView* m_dayBookView;
    TrialBalanceView* m_trialBalanceView;
    ProfitLossView* m_profitLossView;
    BalanceSheetView* m_balanceSheetView;
    LedgerStatementView* m_ledgerStatementView;

    QLabel* m_topCompanyLabel;
    QLabel* m_bottomStatusLabel;
};

} // namespace tally::ui
