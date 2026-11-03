#include "BalanceSheetView.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QKeyEvent>

namespace tally::ui {

BalanceSheetView::BalanceSheetView(
    application::NavigationManager& navManager,
    services::ReportService& reportService,
    QWidget* parent
) : QWidget(parent), m_navManager(navManager), m_reportService(reportService) {
    setupUi();
}

void BalanceSheetView::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 8, 12, 8);
    mainLayout->setSpacing(8);

    // Filter Bar
    QFrame* filterFrame = new QFrame(this);
    filterFrame->setStyleSheet("background-color: #E2ECF5; border: 1px solid #B8D0E5; padding: 4px;");
    QHBoxLayout* fLayout = new QHBoxLayout(filterFrame);

    m_titleLabel = new QLabel("Balance Sheet", filterFrame);
    m_titleLabel->setStyleSheet("font-size: 12pt; font-weight: bold; color: #1B4977;");
    fLayout->addWidget(m_titleLabel);

    fLayout->addStretch();

    fLayout->addWidget(new QLabel("As of Date:", filterFrame));
    m_asOfDateEdit = new QLineEdit("2027-03-31", filterFrame);
    m_asOfDateEdit->setFixedWidth(110);
    fLayout->addWidget(m_asOfDateEdit);

    QPushButton* refreshBtn = new QPushButton("Refresh", filterFrame);
    refreshBtn->setStyleSheet("background-color: #1B4977; color: #FFFFFF; font-weight: bold; padding: 4px 12px;");
    connect(refreshBtn, &QPushButton::clicked, this, &BalanceSheetView::refreshData);
    fLayout->addWidget(refreshBtn);

    mainLayout->addWidget(filterFrame);

    // Dual-Pane T-Format (Liabilities on Left, Assets on Right)
    QHBoxLayout* splitLayout = new QHBoxLayout();

    // Left Pane: Capital & Liabilities
    m_liabilitiesTree = new QTreeWidget(this);
    m_liabilitiesTree->setHeaderLabels({"Liabilities & Capital", "Amount (₹)"});
    m_liabilitiesTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_liabilitiesTree->header()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_liabilitiesTree->setColumnWidth(1, 140);
    splitLayout->addWidget(m_liabilitiesTree);

    // Right Pane: Assets
    m_assetsTree = new QTreeWidget(this);
    m_assetsTree->setHeaderLabels({"Assets", "Amount (₹)"});
    m_assetsTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_assetsTree->header()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_assetsTree->setColumnWidth(1, 140);
    splitLayout->addWidget(m_assetsTree);

    mainLayout->addLayout(splitLayout);

    // Bottom Summary
    QFrame* footerFrame = new QFrame(this);
    footerFrame->setStyleSheet("background-color: #E2ECF5; border: 1px solid #B8D0E5; padding: 4px;");
    QHBoxLayout* footLayout = new QHBoxLayout(footerFrame);

    m_reconciledLabel = new QLabel("Status: Balanced ✓ (Assets == Liabilities + Equity)", footerFrame);
    m_reconciledLabel->setStyleSheet("font-weight: bold; color: #27AE60;");
    footLayout->addWidget(m_reconciledLabel);

    footLayout->addStretch();

    footLayout->addWidget(new QLabel("Total Liabilities & Equity: ₹ ", footerFrame));
    m_totLiabLabel = new QLabel("0.00", footerFrame);
    m_totLiabLabel->setStyleSheet("font-weight: bold; font-size: 11pt;");
    footLayout->addWidget(m_totLiabLabel);

    footLayout->addSpacing(32);

    footLayout->addWidget(new QLabel("Total Assets: ₹ ", footerFrame));
    m_totAssetLabel = new QLabel("0.00", footerFrame);
    m_totAssetLabel->setStyleSheet("font-weight: bold; font-size: 11pt;");
    footLayout->addWidget(m_totAssetLabel);

    mainLayout->addWidget(footerFrame);
}

void BalanceSheetView::setCompanyId(const std::string& companyId) {
    m_companyId = companyId;
    refreshData();
}

void BalanceSheetView::refreshData() {
    if (m_companyId.empty()) return;

    std::string asOfDate = m_asOfDateEdit->text().trimmed().toStdString();
    auto report = m_reportService.generateBalanceSheet(m_companyId, asOfDate);

    m_titleLabel->setText(QString("Balance Sheet: %1 (as of %2)").arg(QString::fromStdString(report.companyName)).arg(QString::fromStdString(report.asOfDate)));
    m_liabilitiesTree->clear();
    m_assetsTree->clear();

    // 1. Populate Liabilities & Capital
    // Capital Account
    QTreeWidgetItem* capRoot = new QTreeWidgetItem(m_liabilitiesTree);
    capRoot->setText(0, "Capital Account");
    capRoot->setText(1, QString::fromStdString(report.equity.totalCapital.formatInr(false)));
    capRoot->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
    QFont bf = capRoot->font(0);
    bf.setBold(true);
    capRoot->setFont(0, bf);

    for (const auto& sec : report.equity.sections) {
        for (const auto& l : sec.ledgers) {
            QTreeWidgetItem* li = new QTreeWidgetItem(capRoot);
            li->setText(0, QString::fromStdString("   " + l.ledgerName));
            li->setText(1, QString::fromStdString(l.amount.formatInr(false)));
            li->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
        }
    }

    // Profit & Loss Current Period
    QTreeWidgetItem* pnlItem = new QTreeWidgetItem(m_liabilitiesTree);
    pnlItem->setText(0, report.equity.isCurrentPeriodProfit ? "Profit & Loss A/c (Current Period Net Profit)" : "Profit & Loss A/c (Current Period Net Loss)");
    pnlItem->setText(1, QString::fromStdString(report.equity.currentPeriodProfitOrLoss.formatInr(false)));
    pnlItem->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
    pnlItem->setFont(0, bf);

    // Liabilities
    for (const auto& sec : report.liabilities.sections) {
        QTreeWidgetItem* secItem = new QTreeWidgetItem(m_liabilitiesTree);
        secItem->setText(0, QString::fromStdString(sec.groupName));
        secItem->setText(1, QString::fromStdString(sec.totalAmount.formatInr(false)));
        secItem->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
        secItem->setFont(0, bf);

        for (const auto& l : sec.ledgers) {
            QTreeWidgetItem* li = new QTreeWidgetItem(secItem);
            li->setText(0, QString::fromStdString("   " + l.ledgerName));
            li->setText(1, QString::fromStdString(l.amount.formatInr(false)));
            li->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
        }
        secItem->setExpanded(true);
    }
    capRoot->setExpanded(true);

    // 2. Populate Assets
    for (const auto& sec : report.assets.sections) {
        QTreeWidgetItem* secItem = new QTreeWidgetItem(m_assetsTree);
        secItem->setText(0, QString::fromStdString(sec.groupName));
        secItem->setText(1, QString::fromStdString(sec.totalAmount.formatInr(false)));
        secItem->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
        secItem->setFont(0, bf);

        for (const auto& l : sec.ledgers) {
            QTreeWidgetItem* li = new QTreeWidgetItem(secItem);
            li->setText(0, QString::fromStdString("   " + l.ledgerName));
            li->setText(1, QString::fromStdString(l.amount.formatInr(false)));
            li->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
        }
        secItem->setExpanded(true);
    }

    m_totLiabLabel->setText(QString::fromStdString(report.totalLiabilitiesAndEquity.formatInr(false)));
    m_totAssetLabel->setText(QString::fromStdString(report.totalAssets.formatInr(false)));

    if (report.isReconciled) {
        m_reconciledLabel->setText("Status: Balanced ✓ (Assets == Liabilities + Equity)");
        m_reconciledLabel->setStyleSheet("font-weight: bold; color: #27AE60;");
    } else {
        m_reconciledLabel->setText(QString("Status: Out of Balance (Diff: ₹ %1)")
            .arg(QString::fromStdString(report.discrepancy.formatInr(false))));
        m_reconciledLabel->setStyleSheet("font-weight: bold; color: #C0392B;");
    }
}

void BalanceSheetView::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        m_navManager.navigateBack();
    } else {
        QWidget::keyPressEvent(event);
    }
}

} // namespace tally::ui
