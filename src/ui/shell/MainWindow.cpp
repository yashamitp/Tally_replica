#include "MainWindow.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QKeyEvent>
#include <QMessageBox>
#include <QApplication>
#include "../theme/TallyTheme.hpp"
#include "../dialogs/GoToDialog.hpp"
#include "../dialogs/DateDialog.hpp"
#include "../company/CompanyDialog.hpp"
#include "../masters/MasterSelectionDialog.hpp"
#include "../exchange/ImportDialog.hpp"
#include "../exchange/ExportDialog.hpp"
#include "../exchange/ExchangeDialog.hpp"
#include "../backup/BackupRestoreDialog.hpp"
#include "../../utilities/Logger.hpp"

namespace tally::ui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    setWindowTitle("Antigravity Accounting - TallyPrime Compatible System (Native C++20 / Qt 6)");
    resize(1200, 750);
    setStyleSheet(TallyTheme::getApplicationStyleSheet());
}

MainWindow::~MainWindow() = default;

bool MainWindow::initialize(const std::string& dbPath) {
    m_dbManager = std::make_unique<database::DatabaseManager>("MainAppConnection");
    if (!m_dbManager->open(dbPath)) {
        utilities::Logger::error("Failed to initialize database at " + dbPath);
        return false;
    }

    m_companyService = std::make_unique<services::CompanyService>(*m_dbManager);
    m_groupService = std::make_unique<services::GroupService>(*m_dbManager);
    m_ledgerService = std::make_unique<services::LedgerService>(*m_dbManager);
    m_voucherService = std::make_unique<services::VoucherService>(*m_dbManager);
    m_dayBookService = std::make_unique<services::DayBookService>(*m_dbManager);
    m_reportService = std::make_unique<services::ReportService>(*m_dbManager);

    // Check or create default company
    auto companies = m_companyService->listCompanies();
    if (companies.empty()) {
        services::CreateCompanyParams cp;
        cp.name = "National Enterprises";
        cp.financialYearStart = "2026-04-01";
        auto comp = m_companyService->createCompany(cp);
        m_currentCompanyId = comp.id;
    } else {
        m_currentCompanyId = companies[0].id;
    }

    setupUi();

    connect(&m_navManager, &application::NavigationManager::screenChanged, this, &MainWindow::onScreenChanged);

    switchActiveCompany(m_currentCompanyId);

    utilities::Logger::info("MainWindow initialized successfully with company: " + m_currentCompanyId);
    return true;
}

void MainWindow::setupUi() {
    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* rootLayout = new QVBoxLayout(centralWidget);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // 1. Top Bar
    setupTopBar();
    rootLayout->addWidget(findChild<QFrame*>("topBar"));

    // 2. Middle (Central Stack + Right Function-Key Sidebar)
    QHBoxLayout* middleLayout = new QHBoxLayout();
    middleLayout->setContentsMargins(0, 0, 0, 0);
    middleLayout->setSpacing(0);

    m_stackedWidget = new QStackedWidget(centralWidget);

    m_gatewayView = new GatewayView(m_navManager, *m_companyService, m_stackedWidget);
    m_voucherEntryView = new VoucherEntryView(m_navManager, *m_companyService, *m_ledgerService, *m_voucherService, m_stackedWidget);
    m_dayBookView = new DayBookView(m_navManager, *m_dayBookService, *m_voucherService, *m_ledgerService, m_stackedWidget);
    m_trialBalanceView = new TrialBalanceView(m_navManager, *m_reportService, m_stackedWidget);
    m_profitLossView = new ProfitLossView(m_navManager, *m_reportService, m_stackedWidget);
    m_balanceSheetView = new BalanceSheetView(m_navManager, *m_reportService, m_stackedWidget);
    m_ledgerStatementView = new LedgerStatementView(m_navManager, *m_ledgerService, *m_voucherService, m_stackedWidget);
    m_groupSummaryView = new GroupSummaryView(m_navManager, *m_reportService, *m_groupService, m_stackedWidget);

    m_stackedWidget->addWidget(m_gatewayView);           // Index 0: Gateway
    m_stackedWidget->addWidget(m_voucherEntryView);      // Index 1: VoucherEntry
    m_stackedWidget->addWidget(m_dayBookView);           // Index 2: DayBook
    m_stackedWidget->addWidget(m_trialBalanceView);      // Index 3: TrialBalance
    m_stackedWidget->addWidget(m_profitLossView);        // Index 4: ProfitLoss
    m_stackedWidget->addWidget(m_balanceSheetView);      // Index 5: BalanceSheet
    m_stackedWidget->addWidget(m_ledgerStatementView);   // Index 6: LedgerStatement
    m_stackedWidget->addWidget(m_groupSummaryView);      // Index 7: GroupSummary

    middleLayout->addWidget(m_stackedWidget, 1);

    // Right Sidebar
    setupRightSidebar();
    middleLayout->addWidget(findChild<QFrame*>("rightSidebar"));

    rootLayout->addLayout(middleLayout, 1);

    // 3. Bottom Status Bar
    setupBottomBar();
    rootLayout->addWidget(findChild<QFrame*>("bottomBar"));

    setCentralWidget(centralWidget);
}

void MainWindow::setupTopBar() {
    QFrame* topBar = new QFrame(this);
    topBar->setObjectName("topBar");
    topBar->setFixedHeight(42);

    QHBoxLayout* layout = new QHBoxLayout(topBar);
    layout->setContentsMargins(12, 0, 12, 0);
    layout->setSpacing(10);

    QLabel* logo = new QLabel("TallyPrime", topBar);
    logo->setStyleSheet("font-size: 13pt; font-weight: bold; color: #FFFFFF;");
    layout->addWidget(logo);

    layout->addSpacing(16);

    QPushButton* compBtn = new QPushButton("K: Company (Alt+K)", topBar);
    connect(compBtn, &QPushButton::clicked, this, &MainWindow::onCompanyClicked);

    QPushButton* dataBtn = new QPushButton("Y: Data (Alt+Y)", topBar);
    connect(dataBtn, &QPushButton::clicked, this, &MainWindow::onBackupRestoreClicked);

    QPushButton* exchBtn = new QPushButton("Z: Exchange (Alt+Z)", topBar);
    connect(exchBtn, &QPushButton::clicked, this, &MainWindow::onExchangeClicked);

    QPushButton* goToBtn = new QPushButton("G: Go To (Alt+G)", topBar);
    goToBtn->setStyleSheet("background-color: #2A6592; font-weight: bold; padding: 4px 10px; border-radius: 3px;");
    connect(goToBtn, &QPushButton::clicked, this, &MainWindow::onOpenGoToDialog);

    QPushButton* impBtn = new QPushButton("O: Import (Alt+O)", topBar);
    connect(impBtn, &QPushButton::clicked, this, &MainWindow::onImportClicked);

    QPushButton* expBtn = new QPushButton("E: Export (Alt+E)", topBar);
    connect(expBtn, &QPushButton::clicked, this, &MainWindow::onExportClicked);

    QPushButton* helpBtn = new QPushButton("F1: Help", topBar);
    connect(helpBtn, &QPushButton::clicked, [this]() {
        QMessageBox::about(this, "About Antigravity Accounting",
            "<h3>Antigravity Accounting</h3>"
            "<p>A native C++20 and Qt 6 desktop accounting system inspired by TallyPrime.</p>"
            "<p>Enforces transactional double-entry invariants, exact decimal arithmetic, and real-time financial reconciliation.</p>");
    });

    layout->addWidget(compBtn);
    layout->addWidget(dataBtn);
    layout->addWidget(exchBtn);
    layout->addWidget(goToBtn);
    layout->addWidget(impBtn);
    layout->addWidget(expBtn);
    layout->addWidget(helpBtn);

    layout->addStretch();

    m_topCompanyLabel = new QLabel("National Enterprises (2026-2027)", topBar);
    m_topCompanyLabel->setStyleSheet("font-size: 9.5pt; color: #FFE082; font-weight: bold;");
    layout->addWidget(m_topCompanyLabel);
}

void MainWindow::setupRightSidebar() {
    QFrame* sidebar = new QFrame(this);
    sidebar->setObjectName("rightSidebar");
    sidebar->setFixedWidth(130);

    QVBoxLayout* layout = new QVBoxLayout(sidebar);
    layout->setContentsMargins(4, 8, 4, 8);
    layout->setSpacing(4);

    struct FKeyDef {
        QString label;
        std::function<void()> action;
    };

    const std::vector<FKeyDef> buttons = {
        {"F2: Date", [this]() { onDateClicked(); }},
        {"F3: Company", [this]() { onCompanyClicked(); }},
        {"F4: Contra", [this]() {
            m_navManager.navigateTo(application::ScreenId::VoucherEntry);
            m_voucherEntryView->setVoucherCategory(domain::VoucherCategory::Contra);
        }},
        {"F5: Payment", [this]() {
            m_navManager.navigateTo(application::ScreenId::VoucherEntry);
            m_voucherEntryView->setVoucherCategory(domain::VoucherCategory::Payment);
        }},
        {"F6: Receipt", [this]() {
            m_navManager.navigateTo(application::ScreenId::VoucherEntry);
            m_voucherEntryView->setVoucherCategory(domain::VoucherCategory::Receipt);
        }},
        {"F7: Journal", [this]() {
            m_navManager.navigateTo(application::ScreenId::VoucherEntry);
            m_voucherEntryView->setVoucherCategory(domain::VoucherCategory::Journal);
        }},
        {"F8: Sales", [this]() {
            m_navManager.navigateTo(application::ScreenId::VoucherEntry);
            m_voucherEntryView->setVoucherCategory(domain::VoucherCategory::Sales);
        }},
        {"F9: Purchase", [this]() {
            m_navManager.navigateTo(application::ScreenId::VoucherEntry);
            m_voucherEntryView->setVoucherCategory(domain::VoucherCategory::Purchase);
        }},
        {"F10: Other", [this]() { onOpenGoToDialog(); }},
        {"F12: Configure", [this]() {
            QMessageBox::information(this, "F12: Configure", "TallyPrime Configuration Settings Dialog");
        }}
    };

    for (const auto& b : buttons) {
        QPushButton* btn = new QPushButton(b.label, sidebar);
        connect(btn, &QPushButton::clicked, b.action);
        layout->addWidget(btn);
    }

    layout->addStretch();
}

void MainWindow::setupBottomBar() {
    QFrame* bottomBar = new QFrame(this);
    bottomBar->setObjectName("bottomBar");
    bottomBar->setFixedHeight(26);

    QHBoxLayout* layout = new QHBoxLayout(bottomBar);
    layout->setContentsMargins(12, 0, 12, 0);

    m_bottomStatusLabel = new QLabel("Gateway of Tally | Esc: Back / Quit | Alt+G: Go To | F1: Help", bottomBar);
    layout->addWidget(m_bottomStatusLabel);

    layout->addStretch();

    QLabel* verLabel = new QLabel("Antigravity Accounting v1.0 (C++20/Qt 6)", bottomBar);
    verLabel->setStyleSheet("color: #B8D0E5; font-size: 8pt;");
    layout->addWidget(verLabel);
}

void MainWindow::switchActiveCompany(const std::string& companyId) {
    m_currentCompanyId = companyId;
    auto comp = m_companyService->getCompany(companyId);
    if (comp) {
        m_topCompanyLabel->setText(QString::fromStdString(comp->name + " (2026-2027)"));
    }

    m_gatewayView->refreshCompanyInfo(m_currentCompanyId);
    m_voucherEntryView->setCompanyId(m_currentCompanyId);
    m_dayBookView->setCompanyId(m_currentCompanyId);
    m_trialBalanceView->setCompanyId(m_currentCompanyId);
    m_profitLossView->setCompanyId(m_currentCompanyId);
    m_balanceSheetView->setCompanyId(m_currentCompanyId);
    m_ledgerStatementView->setCompanyId(m_currentCompanyId);
    m_groupSummaryView->setCompanyId(m_currentCompanyId);
}

void MainWindow::onScreenChanged(application::ScreenId newScreen, const std::string& param) {
    switch (newScreen) {
        case application::ScreenId::Gateway:
            m_stackedWidget->setCurrentIndex(0);
            m_gatewayView->refreshCompanyInfo(m_currentCompanyId);
            m_bottomStatusLabel->setText("Gateway of Tally | Esc: Quit | Alt+G: Go To");
            break;
        case application::ScreenId::VoucherEntry:
            m_stackedWidget->setCurrentIndex(1);
            m_voucherEntryView->setCompanyId(m_currentCompanyId);
            m_bottomStatusLabel->setText("Voucher Creation | F4-F9: Switch Voucher Type | Esc: Back");
            break;
        case application::ScreenId::DayBook:
            m_stackedWidget->setCurrentIndex(2);
            m_dayBookView->setCompanyId(m_currentCompanyId);
            m_bottomStatusLabel->setText("Day Book | Esc: Back to Gateway | Double-click row for voucher details");
            break;
        case application::ScreenId::TrialBalance:
            m_stackedWidget->setCurrentIndex(3);
            m_trialBalanceView->setCompanyId(m_currentCompanyId);
            m_bottomStatusLabel->setText("Trial Balance | Esc: Back to Gateway | Double-click ledger for statement");
            break;
        case application::ScreenId::ProfitLoss:
            m_stackedWidget->setCurrentIndex(4);
            m_profitLossView->setCompanyId(m_currentCompanyId);
            m_bottomStatusLabel->setText("Profit & Loss A/c | Esc: Back to Gateway | Double-click ledger for statement");
            break;
        case application::ScreenId::BalanceSheet:
            m_stackedWidget->setCurrentIndex(5);
            m_balanceSheetView->setCompanyId(m_currentCompanyId);
            m_bottomStatusLabel->setText("Balance Sheet | Esc: Back to Gateway | Double-click ledger for statement");
            break;
        case application::ScreenId::LedgerStatement:
            m_stackedWidget->setCurrentIndex(6);
            m_ledgerStatementView->setCompanyId(m_currentCompanyId);
            if (!param.empty()) m_ledgerStatementView->setLedgerId(param);
            m_bottomStatusLabel->setText("Ledger Account Statement | Esc: Back to Gateway | Double-click row for voucher details");
            break;
        case application::ScreenId::GroupSummary:
            m_stackedWidget->setCurrentIndex(7);
            m_groupSummaryView->setCompanyId(m_currentCompanyId);
            if (!param.empty()) m_groupSummaryView->setGroupId(param);
            m_bottomStatusLabel->setText("Group Summary | Esc: Back to Gateway | Double-click row to drill down");
            break;
        case application::ScreenId::MasterCreation:
            onMasterCreationClicked();
            break;
        case application::ScreenId::MasterAlteration:
            onMasterAlterationClicked();
            break;
        default:
            m_stackedWidget->setCurrentIndex(0);
            break;
    }
}

void MainWindow::onOpenGoToDialog() {
    GoToDialog dlg(m_navManager, this);
    dlg.exec();
}

void MainWindow::onCompanyClicked() {
    CompanyDialog dlg(*m_companyService, m_currentCompanyId, this);
    if (dlg.exec() == QDialog::Accepted && dlg.companyChanged()) {
        switchActiveCompany(dlg.selectedCompanyId());
    }
}

void MainWindow::onDateClicked() {
    DateDialog dlg(m_currentDate, m_fromDate, m_toDate, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_currentDate = dlg.currentDate();
        m_fromDate = dlg.fromDate();
        m_toDate = dlg.toDate();

        auto comp = m_companyService->getCompany(m_currentCompanyId);
        if (comp) {
            m_topCompanyLabel->setText(QString::fromStdString(comp->name + " (" + m_fromDate + " to " + m_toDate + ")"));
        }

        m_gatewayView->refreshCompanyInfo(m_currentCompanyId);
        m_trialBalanceView->refreshData();
        m_profitLossView->refreshData();
        m_balanceSheetView->refreshData();
        m_dayBookView->refreshData();
        m_ledgerStatementView->refreshData();
        m_groupSummaryView->refreshData();
    }
}

void MainWindow::onBackupRestoreClicked() {
    BackupRestoreDialog dlg(*m_dbManager, this);
    if (dlg.exec() == QDialog::Accepted && dlg.databaseRestored()) {
        auto list = m_companyService->listCompanies();
        if (!list.empty()) {
            switchActiveCompany(list[0].id);
        }
    }
}

void MainWindow::onExchangeClicked() {
    ExchangeDialog dlg(m_currentCompanyId, *m_dbManager, this);
    dlg.exec();
}

void MainWindow::onImportClicked() {
    ImportDialog dlg(m_currentCompanyId, *m_dbManager, this);
    if (dlg.exec() == QDialog::Accepted && dlg.dataImported()) {
        switchActiveCompany(m_currentCompanyId);
    }
}

void MainWindow::onExportClicked() {
    ExportDialog dlg(m_currentCompanyId, *m_dbManager, this);
    dlg.exec();
}

void MainWindow::onMasterCreationClicked() {
    MasterSelectionDialog dlg(MasterMode::Create, m_currentCompanyId, *m_companyService, *m_groupService, *m_ledgerService, this);
    dlg.exec();
    m_navManager.navigateTo(application::ScreenId::Gateway);
}

void MainWindow::onMasterAlterationClicked() {
    MasterSelectionDialog dlg(MasterMode::Alter, m_currentCompanyId, *m_companyService, *m_groupService, *m_ledgerService, this);
    dlg.exec();
    m_navManager.navigateTo(application::ScreenId::Gateway);
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
    if (event->modifiers() & Qt::AltModifier) {
        if (event->key() == Qt::Key_G) {
            onOpenGoToDialog();
            return;
        } else if (event->key() == Qt::Key_K) {
            onCompanyClicked();
            return;
        } else if (event->key() == Qt::Key_Y) {
            onBackupRestoreClicked();
            return;
        } else if (event->key() == Qt::Key_Z) {
            onExchangeClicked();
            return;
        } else if (event->key() == Qt::Key_O) {
            onImportClicked();
            return;
        } else if (event->key() == Qt::Key_E) {
            onExportClicked();
            return;
        }
    }

    if (event->key() == Qt::Key_F2) {
        onDateClicked();
        return;
    } else if (event->key() == Qt::Key_F3) {
        onCompanyClicked();
        return;
    } else if (event->key() == Qt::Key_F1) {
        QMessageBox::about(this, "About Antigravity Accounting",
            "<h3>Antigravity Accounting</h3>"
            "<p>A native C++20 and Qt 6 desktop accounting system inspired by TallyPrime.</p>"
            "<p>Enforces transactional double-entry invariants, exact decimal arithmetic, and real-time financial reconciliation.</p>");
        return;
    } else if (event->key() == Qt::Key_Escape) {
        bool navigated = m_navManager.navigateBack();
        if (!navigated && m_navManager.currentScreen() == application::ScreenId::Gateway) {
            auto res = QMessageBox::question(this, "Quit", "Quit? (Yes/No)", QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (res == QMessageBox::Yes) {
                QApplication::quit();
            }
        }
        return;
    }

    QMainWindow::keyPressEvent(event);
}

} // namespace tally::ui
