#include "GatewayView.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QKeyEvent>
#include <QApplication>

namespace tally::ui {

GatewayView::GatewayView(
    application::NavigationManager& navManager,
    services::CompanyService& companyService,
    QWidget* parent
) : QWidget(parent), m_navManager(navManager), m_companyService(companyService) {
    setupUi();
    populateMenu();
}

void GatewayView::setupUi() {
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(24);

    // Left Panel: Company Status Box
    QGroupBox* leftBox = new QGroupBox("Current Company Details", this);
    leftBox->setStyleSheet(R"(
        QGroupBox {
            background-color: #FFFFFF;
            border: 2px solid #1B4977;
            font-weight: bold;
            margin-top: 10px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 4px;
            color: #1B4977;
        }
    )");
    leftBox->setFixedWidth(320);

    QVBoxLayout* leftLayout = new QVBoxLayout(leftBox);
    leftLayout->setContentsMargins(16, 20, 16, 16);
    leftLayout->setSpacing(12);

    QLabel* compHeader = new QLabel("CURRENT COMPANY", leftBox);
    compHeader->setStyleSheet("font-size: 8.5pt; color: #556B82; font-weight: bold;");
    leftLayout->addWidget(compHeader);

    m_companyNameLabel = new QLabel("National Enterprises", leftBox);
    m_companyNameLabel->setStyleSheet("font-size: 13pt; font-weight: bold; color: #1B4977;");
    leftLayout->addWidget(m_companyNameLabel);

    leftLayout->addSpacing(12);

    QLabel* periodHeader = new QLabel("CURRENT PERIOD", leftBox);
    periodHeader->setStyleSheet("font-size: 8.5pt; color: #556B82; font-weight: bold;");
    leftLayout->addWidget(periodHeader);

    m_periodLabel = new QLabel("1-Apr-2026 to 31-Mar-2027", leftBox);
    m_periodLabel->setStyleSheet("font-size: 10.5pt; font-weight: bold;");
    leftLayout->addWidget(m_periodLabel);

    leftLayout->addSpacing(12);

    QLabel* dateHeader = new QLabel("DATE OF LAST ENTRY", leftBox);
    dateHeader->setStyleSheet("font-size: 8.5pt; color: #556B82; font-weight: bold;");
    leftLayout->addWidget(dateHeader);

    m_lastDateLabel = new QLabel("1-Apr-2026", leftBox);
    m_lastDateLabel->setStyleSheet("font-size: 10.5pt;");
    leftLayout->addWidget(m_lastDateLabel);

    leftLayout->addStretch();
    mainLayout->addWidget(leftBox);

    // Right Panel: Gateway of Tally Menu Box
    QFrame* rightBox = new QFrame(this);
    rightBox->setObjectName("gatewayBox");
    rightBox->setFixedWidth(360);

    QVBoxLayout* rightLayout = new QVBoxLayout(rightBox);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    QLabel* gwHeader = new QLabel("Gateway of Tally", rightBox);
    gwHeader->setObjectName("gatewayHeader");
    rightLayout->addWidget(gwHeader);

    m_menuList = new QListWidget(rightBox);
    m_menuList->setObjectName("gatewayList");
    m_menuList->setFocusPolicy(Qt::StrongFocus);
    rightLayout->addWidget(m_menuList);

    mainLayout->addWidget(rightBox);
    mainLayout->addStretch();

    connect(m_menuList, &QListWidget::itemActivated, this, &GatewayView::onMenuItemActivated);
    connect(m_menuList, &QListWidget::itemDoubleClicked, this, &GatewayView::onMenuItemActivated);
}

void GatewayView::populateMenu() {
    struct MenuDef {
        QString text;
        application::ScreenId screen;
        bool isHeader;
        char hotkey;
    };

    const std::vector<MenuDef> items = {
        {"--- MASTERS ---", application::ScreenId::Gateway, true, '\0'},
        {"Create", application::ScreenId::MasterCreation, false, 'C'},
        {"Alter", application::ScreenId::MasterAlteration, false, 'A'},
        {"Chart of Accounts", application::ScreenId::TrialBalance, false, 'H'},
        {"--- TRANSACTIONS ---", application::ScreenId::Gateway, true, '\0'},
        {"Vouchers", application::ScreenId::VoucherEntry, false, 'V'},
        {"Day Book", application::ScreenId::DayBook, false, 'K'},
        {"--- REPORTS ---", application::ScreenId::Gateway, true, '\0'},
        {"Balance Sheet", application::ScreenId::BalanceSheet, false, 'B'},
        {"Profit & Loss A/c", application::ScreenId::ProfitLoss, false, 'P'},
        {"Trial Balance", application::ScreenId::TrialBalance, false, 'T'},
        {"Cash / Bank Book", application::ScreenId::LedgerStatement, false, 'S'},
        {"Ledger Statement", application::ScreenId::LedgerStatement, false, 'L'},
        {"Group Summary", application::ScreenId::GroupSummary, false, 'G'},
        {"Quit", application::ScreenId::Gateway, false, 'Q'}
    };

    m_menuList->clear();
    for (const auto& item : items) {
        QListWidgetItem* listItem = new QListWidgetItem(item.text, m_menuList);
        if (item.isHeader) {
            listItem->setFlags(Qt::NoItemFlags);
            listItem->setForeground(QColor("#1B4977"));
            QFont font = listItem->font();
            font.setBold(true);
            font.setPointSize(8);
            listItem->setFont(font);
        } else {
            listItem->setData(Qt::UserRole, static_cast<int>(item.screen));
            listItem->setData(Qt::UserRole + 1, static_cast<int>(item.hotkey));
        }
    }

    // Select first actionable item (Vouchers)
    for (int i = 0; i < m_menuList->count(); ++i) {
        if (m_menuList->item(i)->flags() & Qt::ItemIsEnabled) {
            m_menuList->setCurrentRow(i);
            break;
        }
    }
}

void GatewayView::refreshCompanyInfo(const std::string& companyId) {
    auto comp = m_companyService.getCompany(companyId);
    if (comp) {
        m_companyNameLabel->setText(QString::fromStdString(comp->name));
    }
}

void GatewayView::onMenuItemActivated(QListWidgetItem* item) {
    if (!item || !(item->flags() & Qt::ItemIsEnabled)) return;
    int hotkey = item->data(Qt::UserRole + 1).toInt();
    if (hotkey == 'Q') {
        QApplication::quit();
        return;
    }
    auto screen = static_cast<application::ScreenId>(item->data(Qt::UserRole).toInt());
    m_navManager.navigateTo(screen);
}

void GatewayView::keyPressEvent(QKeyEvent* event) {
    QString text = event->text().toUpper();
    if (!text.isEmpty()) {
        char keyChar = text.toUtf8().at(0);
        for (int i = 0; i < m_menuList->count(); ++i) {
            auto* item = m_menuList->item(i);
            if (item && (item->flags() & Qt::ItemIsEnabled)) {
                char hotkey = static_cast<char>(item->data(Qt::UserRole + 1).toInt());
                if (hotkey == keyChar) {
                    m_menuList->setCurrentRow(i);
                    onMenuItemActivated(item);
                    return;
                }
            }
        }
    }
    QWidget::keyPressEvent(event);
}

} // namespace tally::ui
