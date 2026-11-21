#include "ProfitLossView.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QKeyEvent>

namespace tally::ui {

ProfitLossView::ProfitLossView(
    application::NavigationManager& navManager,
    services::ReportService& reportService,
    QWidget* parent
) : QWidget(parent), m_navManager(navManager), m_reportService(reportService) {
    setupUi();
}

void ProfitLossView::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 8, 12, 8);
    mainLayout->setSpacing(8);

    // Filter Bar
    QFrame* filterFrame = new QFrame(this);
    filterFrame->setStyleSheet("background-color: #E2ECF5; border: 1px solid #B8D0E5; padding: 4px;");
    QHBoxLayout* fLayout = new QHBoxLayout(filterFrame);

    m_titleLabel = new QLabel("Profit & Loss A/c", filterFrame);
    m_titleLabel->setStyleSheet("font-size: 12pt; font-weight: bold; color: #1B4977;");
    fLayout->addWidget(m_titleLabel);

    fLayout->addStretch();

    fLayout->addWidget(new QLabel("From:", filterFrame));
    m_fromDateEdit = new QLineEdit("2026-04-01", filterFrame);
    m_fromDateEdit->setFixedWidth(100);
    fLayout->addWidget(m_fromDateEdit);

    fLayout->addWidget(new QLabel("To:", filterFrame));
    m_toDateEdit = new QLineEdit("2027-03-31", filterFrame);
    m_toDateEdit->setFixedWidth(100);
    fLayout->addWidget(m_toDateEdit);

    QPushButton* refreshBtn = new QPushButton("Refresh", filterFrame);
    refreshBtn->setStyleSheet("background-color: #1B4977; color: #FFFFFF; font-weight: bold; padding: 4px 12px;");
    connect(refreshBtn, &QPushButton::clicked, this, &ProfitLossView::refreshData);
    fLayout->addWidget(refreshBtn);

    mainLayout->addWidget(filterFrame);

    // Tree View
    m_tree = new QTreeWidget(this);
    m_tree->setHeaderLabels({"Particulars", "Amount (₹)"});
    m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_tree->header()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_tree->setColumnWidth(1, 180);

    connect(m_tree, &QTreeWidget::itemDoubleClicked, [this](QTreeWidgetItem* item, int) {
        if (!item) return;
        QString lid = item->data(0, Qt::UserRole).toString();
        if (!lid.isEmpty()) {
            m_navManager.navigateTo(application::ScreenId::LedgerStatement, lid.toStdString());
        }
    });

    mainLayout->addWidget(m_tree);

    // Footer Summary
    QFrame* footerFrame = new QFrame(this);
    footerFrame->setStyleSheet("background-color: #E2ECF5; border: 1px solid #B8D0E5; padding: 4px;");
    QHBoxLayout* footLayout = new QHBoxLayout(footerFrame);

    footLayout->addWidget(new QLabel("Gross Profit: ₹ ", footerFrame));
    m_grossProfitLabel = new QLabel("0.00", footerFrame);
    m_grossProfitLabel->setStyleSheet("font-weight: bold; font-size: 11pt; color: #1B4977;");
    footLayout->addWidget(m_grossProfitLabel);

    footLayout->addSpacing(32);

    footLayout->addWidget(new QLabel("Net Profit: ₹ ", footerFrame));
    m_netProfitLabel = new QLabel("0.00", footerFrame);
    m_netProfitLabel->setStyleSheet("font-weight: bold; font-size: 11pt; color: #27AE60;");
    footLayout->addWidget(m_netProfitLabel);
    footLayout->addStretch();

    mainLayout->addWidget(footerFrame);
}

void ProfitLossView::setCompanyId(const std::string& companyId) {
    m_companyId = companyId;
    refreshData();
}

void ProfitLossView::refreshData() {
    if (m_companyId.empty()) return;

    std::string fromDate = m_fromDateEdit->text().trimmed().toStdString();
    std::string toDate = m_toDateEdit->text().trimmed().toStdString();

    auto report = m_reportService.generateProfitAndLoss(m_companyId, fromDate, toDate);
    m_titleLabel->setText(QString("Profit & Loss A/c: %1").arg(QString::fromStdString(report.companyName)));
    m_tree->clear();

    std::function<void(QTreeWidgetItem*, const services::PnLGroupSection&)> populateSection =
        [&](QTreeWidgetItem* parent, const services::PnLGroupSection& sec) {
        if (sec.totalAmount.isZero() && sec.ledgers.empty() && sec.subGroups.empty()) return;

        QTreeWidgetItem* container = parent;
        if (!sec.subGroups.empty() || sec.ledgers.size() > 1) {
            QTreeWidgetItem* grpItem = new QTreeWidgetItem(parent);
            grpItem->setText(0, QString::fromStdString(sec.groupName));
            grpItem->setText(1, QString::fromStdString(sec.totalAmount.formatInr(false)));
            grpItem->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
            QFont gf = grpItem->font(0);
            gf.setBold(true);
            grpItem->setFont(0, gf);
            grpItem->setExpanded(true);
            container = grpItem;
        }

        for (const auto& l : sec.ledgers) {
            if (l.amount.isZero()) continue;
            QTreeWidgetItem* li = new QTreeWidgetItem(container);
            li->setText(0, QString::fromStdString("   " + l.ledgerName));
            li->setData(0, Qt::UserRole, QString::fromStdString(l.ledgerId));
            li->setText(1, QString::fromStdString(l.amount.formatInr(false)));
            li->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
        }

        for (const auto& child : sec.subGroups) {
            populateSection(container, child);
        }
    };

    // 1. Trading Account
    QTreeWidgetItem* tradingHeader = new QTreeWidgetItem(m_tree);
    tradingHeader->setText(0, "TRADING ACCOUNT");
    QFont thf = tradingHeader->font(0);
    thf.setBold(true);
    tradingHeader->setFont(0, thf);

    // Direct Incomes
    QTreeWidgetItem* diHeader = new QTreeWidgetItem(tradingHeader);
    diHeader->setText(0, "Direct Incomes (Sales)");
    diHeader->setText(1, QString::fromStdString(report.tradingAccount.directIncomeTotal.formatInr(false)));
    diHeader->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
    for (const auto& sec : report.tradingAccount.directIncomeSections) {
        populateSection(diHeader, sec);
    }

    // Direct Expenses
    QTreeWidgetItem* deHeader = new QTreeWidgetItem(tradingHeader);
    deHeader->setText(0, "Direct Expenses (Purchases)");
    deHeader->setText(1, QString::fromStdString(report.tradingAccount.directExpensesTotal.formatInr(false)));
    deHeader->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
    for (const auto& sec : report.tradingAccount.directExpenseSections) {
        populateSection(deHeader, sec);
    }

    // Gross Profit
    QTreeWidgetItem* gpItem = new QTreeWidgetItem(tradingHeader);
    gpItem->setText(0, report.tradingAccount.isGrossProfit ? "Gross Profit c/o" : "Gross Loss c/o");
    gpItem->setText(1, QString::fromStdString(
        report.tradingAccount.isGrossProfit ? report.tradingAccount.grossProfit.formatInr(false) : report.tradingAccount.grossLoss.formatInr(false)));
    gpItem->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
    QFont gpf = gpItem->font(0);
    gpf.setBold(true);
    gpItem->setFont(0, gpf);
    gpItem->setFont(1, gpf);

    tradingHeader->setExpanded(true);
    diHeader->setExpanded(true);
    deHeader->setExpanded(true);

    // 2. Operating Account
    QTreeWidgetItem* opHeader = new QTreeWidgetItem(m_tree);
    opHeader->setText(0, "INCOME STATEMENT (OPERATING)");
    opHeader->setFont(0, thf);

    // Indirect Incomes
    if (report.operatingAccount.indirectIncomeTotal > domain::ExactDecimal::zero()) {
        QTreeWidgetItem* iiHeader = new QTreeWidgetItem(opHeader);
        iiHeader->setText(0, "Indirect Incomes");
        iiHeader->setText(1, QString::fromStdString(report.operatingAccount.indirectIncomeTotal.formatInr(false)));
        iiHeader->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
        for (const auto& sec : report.operatingAccount.indirectIncomeSections) {
            populateSection(iiHeader, sec);
        }
        iiHeader->setExpanded(true);
    }

    // Indirect Expenses
    QTreeWidgetItem* ieHeader = new QTreeWidgetItem(opHeader);
    ieHeader->setText(0, "Indirect Expenses (Overheads)");
    ieHeader->setText(1, QString::fromStdString(report.operatingAccount.indirectExpensesTotal.formatInr(false)));
    ieHeader->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
    for (const auto& sec : report.operatingAccount.indirectExpenseSections) {
        populateSection(ieHeader, sec);
    }
    ieHeader->setExpanded(true);

    // Net Profit
    QTreeWidgetItem* npItem = new QTreeWidgetItem(opHeader);
    npItem->setText(0, report.operatingAccount.isNetProfit ? "Nett Profit" : "Nett Loss");
    npItem->setText(1, QString::fromStdString(
        report.operatingAccount.isNetProfit ? report.operatingAccount.netProfit.formatInr(false) : report.operatingAccount.netLoss.formatInr(false)));
    npItem->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
    npItem->setFont(0, gpf);
    npItem->setFont(1, gpf);

    opHeader->setExpanded(true);

    m_grossProfitLabel->setText(QString::fromStdString(
        report.tradingAccount.isGrossProfit ? report.tradingAccount.grossProfit.formatInr(false) : "-" + report.tradingAccount.grossLoss.formatInr(false)));
    m_netProfitLabel->setText(QString::fromStdString(
        report.operatingAccount.isNetProfit ? report.operatingAccount.netProfit.formatInr(false) : "-" + report.operatingAccount.netLoss.formatInr(false)));
}

void ProfitLossView::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        m_navManager.navigateBack();
    } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        auto* item = m_tree->currentItem();
        if (item) {
            QString lid = item->data(0, Qt::UserRole).toString();
            if (!lid.isEmpty()) {
                m_navManager.navigateTo(application::ScreenId::LedgerStatement, lid.toStdString());
                return;
            }
        }
        QWidget::keyPressEvent(event);
    } else {
        QWidget::keyPressEvent(event);
    }
}

} // namespace tally::ui
