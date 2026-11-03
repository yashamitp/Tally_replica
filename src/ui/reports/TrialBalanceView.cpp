#include "TrialBalanceView.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QKeyEvent>

namespace tally::ui {

TrialBalanceView::TrialBalanceView(
    application::NavigationManager& navManager,
    services::ReportService& reportService,
    QWidget* parent
) : QWidget(parent), m_navManager(navManager), m_reportService(reportService) {
    setupUi();
}

void TrialBalanceView::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 8, 12, 8);
    mainLayout->setSpacing(8);

    // Filter Bar
    QFrame* filterFrame = new QFrame(this);
    filterFrame->setStyleSheet("background-color: #E2ECF5; border: 1px solid #B8D0E5; padding: 4px;");
    QHBoxLayout* fLayout = new QHBoxLayout(filterFrame);

    m_companyHeaderLabel = new QLabel("Trial Balance", filterFrame);
    m_companyHeaderLabel->setStyleSheet("font-size: 12pt; font-weight: bold; color: #1B4977;");
    fLayout->addWidget(m_companyHeaderLabel);

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
    connect(refreshBtn, &QPushButton::clicked, this, &TrialBalanceView::refreshData);
    fLayout->addWidget(refreshBtn);

    mainLayout->addWidget(filterFrame);

    // Hierarchical Tree
    m_tree = new QTreeWidget(this);
    m_tree->setHeaderLabels({"Particulars (Groups & Ledgers)", "Debit (₹)", "Credit (₹)"});
    m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_tree->header()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_tree->setColumnWidth(1, 160);
    m_tree->header()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_tree->setColumnWidth(2, 160);
    mainLayout->addWidget(m_tree);

    // Footer Summary
    QFrame* footerFrame = new QFrame(this);
    footerFrame->setStyleSheet("background-color: #E2ECF5; border: 1px solid #B8D0E5; padding: 4px;");
    QHBoxLayout* footLayout = new QHBoxLayout(footerFrame);

    m_statusLabel = new QLabel("Status: Reconciled ✓", footerFrame);
    m_statusLabel->setStyleSheet("font-weight: bold; color: #27AE60;");
    footLayout->addWidget(m_statusLabel);

    footLayout->addStretch();

    footLayout->addWidget(new QLabel("Total Closing Debits: ₹ ", footerFrame));
    m_totCloseDrLabel = new QLabel("0.00", footerFrame);
    m_totCloseDrLabel->setStyleSheet("font-weight: bold; font-size: 11pt;");
    footLayout->addWidget(m_totCloseDrLabel);

    footLayout->addSpacing(24);

    footLayout->addWidget(new QLabel("Total Closing Credits: ₹ ", footerFrame));
    m_totCloseCrLabel = new QLabel("0.00", footerFrame);
    m_totCloseCrLabel->setStyleSheet("font-weight: bold; font-size: 11pt;");
    footLayout->addWidget(m_totCloseCrLabel);

    mainLayout->addWidget(footerFrame);
}

void TrialBalanceView::setCompanyId(const std::string& companyId) {
    m_companyId = companyId;
    refreshData();
}

void TrialBalanceView::populateNode(QTreeWidgetItem* parentItem, const services::TrialBalanceGroupNode& node) {
    QTreeWidgetItem* item = parentItem ? new QTreeWidgetItem(parentItem) : new QTreeWidgetItem(m_tree);
    item->setText(0, QString::fromStdString(node.groupName));
    QFont font = item->font(0);
    font.setBold(true);
    item->setFont(0, font);

    if (node.closingDebit > domain::ExactDecimal::zero()) {
        item->setText(1, QString::fromStdString(node.closingDebit.formatInr(false)));
        item->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
    }
    if (node.closingCredit > domain::ExactDecimal::zero()) {
        item->setText(2, QString::fromStdString(node.closingCredit.formatInr(false)));
        item->setTextAlignment(2, Qt::AlignRight | Qt::AlignVCenter);
    }

    for (const auto& l : node.ledgers) {
        QTreeWidgetItem* lItem = new QTreeWidgetItem(item);
        lItem->setText(0, QString::fromStdString("   " + l.ledgerName));
        if (l.closingDebit > domain::ExactDecimal::zero()) {
            lItem->setText(1, QString::fromStdString(l.closingDebit.formatInr(false)));
            lItem->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
        }
        if (l.closingCredit > domain::ExactDecimal::zero()) {
            lItem->setText(2, QString::fromStdString(l.closingCredit.formatInr(false)));
            lItem->setTextAlignment(2, Qt::AlignRight | Qt::AlignVCenter);
        }
    }

    for (const auto& sg : node.subGroups) {
        populateNode(item, sg);
    }

    item->setExpanded(true);
}

void TrialBalanceView::refreshData() {
    if (m_companyId.empty()) return;

    std::string fromDate = m_fromDateEdit->text().trimmed().toStdString();
    std::string toDate = m_toDateEdit->text().trimmed().toStdString();

    auto report = m_reportService.generateTrialBalance(m_companyId, fromDate, toDate, true);

    m_companyHeaderLabel->setText(QString("Trial Balance: %1").arg(QString::fromStdString(report.companyName)));
    m_tree->clear();

    for (const auto& g : report.groups) {
        populateNode(nullptr, g);
    }

    m_totCloseDrLabel->setText(QString::fromStdString(report.totalClosingDebit.formatInr(false)));
    m_totCloseCrLabel->setText(QString::fromStdString(report.totalClosingCredit.formatInr(false)));

    if (report.isReconciled) {
        m_statusLabel->setText("Status: Reconciled ✓ (Debits == Credits)");
        m_statusLabel->setStyleSheet("font-weight: bold; color: #27AE60;");
    } else {
        m_statusLabel->setText(QString("Status: Out of Balance (Discrepancy: ₹ %1)")
            .arg(QString::fromStdString(report.discrepancy.formatInr(false))));
        m_statusLabel->setStyleSheet("font-weight: bold; color: #C0392B;");
    }
}

void TrialBalanceView::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        m_navManager.navigateBack();
    } else {
        QWidget::keyPressEvent(event);
    }
}

} // namespace tally::ui
