#include "LedgerStatementView.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QKeyEvent>
#include "../vouchers/VoucherDetailsDialog.hpp"

namespace tally::ui {

LedgerStatementView::LedgerStatementView(
    application::NavigationManager& navManager,
    services::LedgerService& ledgerService,
    services::VoucherService& voucherService,
    QWidget* parent
) : QWidget(parent),
    m_navManager(navManager),
    m_ledgerService(ledgerService),
    m_voucherService(voucherService) {
    setupUi();
}

void LedgerStatementView::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 8, 12, 8);
    mainLayout->setSpacing(8);

    // Top Header & Selector Bar
    QFrame* headerFrame = new QFrame(this);
    headerFrame->setStyleSheet("background-color: #E2ECF5; border: 1px solid #B8D0E5; padding: 4px;");
    QHBoxLayout* hLayout = new QHBoxLayout(headerFrame);

    QLabel* titleLbl = new QLabel("Ledger Account Statement", headerFrame);
    titleLbl->setStyleSheet("font-size: 12pt; font-weight: bold; color: #1B4977;");
    hLayout->addWidget(titleLbl);

    hLayout->addSpacing(16);
    hLayout->addWidget(new QLabel("Ledger:", headerFrame));

    m_ledgerCombo = new QComboBox(headerFrame);
    m_ledgerCombo->setMinimumWidth(220);
    connect(m_ledgerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &LedgerStatementView::onLedgerSelectionChanged);
    hLayout->addWidget(m_ledgerCombo);

    hLayout->addStretch();

    hLayout->addWidget(new QLabel("From:", headerFrame));
    m_fromDateEdit = new QLineEdit("2026-04-01", headerFrame);
    m_fromDateEdit->setFixedWidth(100);
    hLayout->addWidget(m_fromDateEdit);

    hLayout->addWidget(new QLabel("To:", headerFrame));
    m_toDateEdit = new QLineEdit("2027-03-31", headerFrame);
    m_toDateEdit->setFixedWidth(100);
    hLayout->addWidget(m_toDateEdit);

    QPushButton* refreshBtn = new QPushButton("Refresh", headerFrame);
    refreshBtn->setStyleSheet("background-color: #1B4977; color: #FFFFFF; font-weight: bold; padding: 4px 12px;");
    connect(refreshBtn, &QPushButton::clicked, this, &LedgerStatementView::refreshData);
    hLayout->addWidget(refreshBtn);

    mainLayout->addWidget(headerFrame);

    // Opening Balance Banner
    QFrame* opFrame = new QFrame(this);
    opFrame->setStyleSheet("background-color: #FFFFFF; border: 1px solid #B8D0E5; padding: 4px;");
    QHBoxLayout* opLayout = new QHBoxLayout(opFrame);
    m_openingBalanceLabel = new QLabel("Opening Balance: ₹ 0.00 Dr", opFrame);
    m_openingBalanceLabel->setStyleSheet("font-weight: bold; color: #1B4977;");
    opLayout->addWidget(m_openingBalanceLabel);
    opLayout->addStretch();
    mainLayout->addWidget(opFrame);

    // Transactions Table
    m_table = new QTableWidget(0, 6, this);
    m_table->setHorizontalHeaderLabels({"Date", "Particulars", "Vch Type", "Vch No.", "Debit (₹)", "Credit (₹)"});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_table->setColumnWidth(0, 100);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_table->setColumnWidth(2, 110);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    m_table->setColumnWidth(3, 100);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    m_table->setColumnWidth(4, 140);
    m_table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);
    m_table->setColumnWidth(5, 140);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &LedgerStatementView::onRowDoubleClicked);
    mainLayout->addWidget(m_table);

    // Footer Summary
    QFrame* footFrame = new QFrame(this);
    footFrame->setStyleSheet("background-color: #E2ECF5; border: 1px solid #B8D0E5; padding: 4px;");
    QHBoxLayout* footLayout = new QHBoxLayout(footFrame);

    m_closingBalanceLabel = new QLabel("Closing Balance: ₹ 0.00 Dr", footFrame);
    m_closingBalanceLabel->setStyleSheet("font-weight: bold; font-size: 11pt; color: #1B4977;");
    footLayout->addWidget(m_closingBalanceLabel);

    footLayout->addStretch();

    footLayout->addWidget(new QLabel("Total Debits: ₹ ", footFrame));
    m_totalDrLabel = new QLabel("0.00", footFrame);
    m_totalDrLabel->setStyleSheet("font-weight: bold; font-size: 11pt;");
    footLayout->addWidget(m_totalDrLabel);

    footLayout->addSpacing(24);

    footLayout->addWidget(new QLabel("Total Credits: ₹ ", footFrame));
    m_totalCrLabel = new QLabel("0.00", footFrame);
    m_totalCrLabel->setStyleSheet("font-weight: bold; font-size: 11pt;");
    footLayout->addWidget(m_totalCrLabel);

    mainLayout->addWidget(footFrame);
}

void LedgerStatementView::setCompanyId(const std::string& companyId) {
    m_companyId = companyId;
    loadLedgersCombo();
    refreshData();
}

void LedgerStatementView::setLedgerId(const std::string& ledgerId) {
    m_selectedLedgerId = ledgerId;
    for (int i = 0; i < m_ledgerCombo->count(); ++i) {
        if (m_ledgerCombo->itemData(i).toString().toStdString() == ledgerId) {
            m_ledgerCombo->setCurrentIndex(i);
            break;
        }
    }
    refreshData();
}

void LedgerStatementView::loadLedgersCombo() {
    m_ledgerCombo->clear();
    auto list = m_ledgerService.listLedgers(m_companyId);
    for (const auto& l : list) {
        m_ledgerCombo->addItem(QString::fromStdString(l.name), QString::fromStdString(l.id));
    }
}

void LedgerStatementView::onLedgerSelectionChanged(int index) {
    if (index >= 0) {
        m_selectedLedgerId = m_ledgerCombo->itemData(index).toString().toStdString();
        refreshData();
    }
}

void LedgerStatementView::refreshData() {
    if (m_companyId.empty() || m_selectedLedgerId.empty()) return;

    std::string fromDate = m_fromDateEdit->text().trimmed().toStdString();
    std::string toDate = m_toDateEdit->text().trimmed().toStdString();

    try {
        auto stmt = m_ledgerService.getLedgerStatement(m_companyId, m_selectedLedgerId, fromDate, toDate);

        m_openingBalanceLabel->setText(QString("Opening Balance: ₹ %1 %2")
            .arg(QString::fromStdString(stmt.openingBalance.formatInr(false)))
            .arg(QString::fromStdString(domain::balanceTypeToString(stmt.openingBalanceType))));

        m_closingBalanceLabel->setText(QString("Closing Balance: ₹ %1 %2")
            .arg(QString::fromStdString(stmt.closingBalance.formatInr(false)))
            .arg(QString::fromStdString(domain::balanceTypeToString(stmt.closingBalanceType))));

        m_totalDrLabel->setText(QString::fromStdString(stmt.totalDebits.formatInr(false)));
        m_totalCrLabel->setText(QString::fromStdString(stmt.totalCredits.formatInr(false)));

        m_table->setRowCount(0);
        for (const auto& line : stmt.transactions) {
            int r = m_table->rowCount();
            m_table->insertRow(r);

            auto* dateItem = new QTableWidgetItem(QString::fromStdString(line.date));
            dateItem->setData(Qt::UserRole, QString::fromStdString(line.voucherId));
            m_table->setItem(r, 0, dateItem);
            m_table->setItem(r, 1, new QTableWidgetItem(QString::fromStdString(line.particulars)));
            m_table->setItem(r, 2, new QTableWidgetItem(QString::fromStdString(line.voucherTypeName)));
            m_table->setItem(r, 3, new QTableWidgetItem(QString::fromStdString(line.voucherNumber)));

            auto* dItem = new QTableWidgetItem(line.debitAmount ? QString::fromStdString(line.debitAmount->formatInr(false)) : "");
            dItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            m_table->setItem(r, 4, dItem);

            auto* cItem = new QTableWidgetItem(line.creditAmount ? QString::fromStdString(line.creditAmount->formatInr(false)) : "");
            cItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            m_table->setItem(r, 5, cItem);
        }
    } catch (...) {}
}

void LedgerStatementView::onRowDoubleClicked(int row, int column) {
    Q_UNUSED(column);
    auto* item = m_table->item(row, 0);
    if (!item) return;

    std::string voucherId = item->data(Qt::UserRole).toString().toStdString();
    if (voucherId.empty()) return;

    VoucherDetailsDialog dlg(m_companyId, voucherId, m_voucherService, m_ledgerService, this);
    if (dlg.exec() == QDialog::Accepted || dlg.voucherModified()) {
        refreshData();
    }
}

void LedgerStatementView::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        m_navManager.navigateBack();
    } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        int r = m_table->currentRow();
        if (r >= 0) onRowDoubleClicked(r, 0);
    } else {
        QWidget::keyPressEvent(event);
    }
}

} // namespace tally::ui
