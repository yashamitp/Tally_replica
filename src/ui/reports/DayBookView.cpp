#include "DayBookView.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QComboBox>
#include "../vouchers/VoucherDetailsDialog.hpp"

namespace tally::ui {

DayBookView::DayBookView(
    application::NavigationManager& navManager,
    services::DayBookService& dayBookService,
    services::VoucherService& voucherService,
    services::LedgerService& ledgerService,
    QWidget* parent
) : QWidget(parent),
    m_navManager(navManager),
    m_dayBookService(dayBookService),
    m_voucherService(voucherService),
    m_ledgerService(ledgerService) {
    setupUi();
}

void DayBookView::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 8, 12, 8);
    mainLayout->setSpacing(8);

    // Top Filter Bar
    QFrame* filterFrame = new QFrame(this);
    filterFrame->setStyleSheet("background-color: #E2ECF5; border: 1px solid #B8D0E5; padding: 4px;");
    QHBoxLayout* fLayout = new QHBoxLayout(filterFrame);

    QLabel* titleLbl = new QLabel("Day Book", filterFrame);
    titleLbl->setStyleSheet("font-size: 12pt; font-weight: bold; color: #1B4977;");
    fLayout->addWidget(titleLbl);

    fLayout->addStretch();

    QLabel* fromLbl = new QLabel("From:", filterFrame);
    fLayout->addWidget(fromLbl);
    m_fromDateEdit = new QLineEdit("2026-04-01", filterFrame);
    m_fromDateEdit->setFixedWidth(100);
    fLayout->addWidget(m_fromDateEdit);

    QLabel* toLbl = new QLabel("To:", filterFrame);
    fLayout->addWidget(toLbl);
    m_toDateEdit = new QLineEdit("2026-04-30", filterFrame);
    m_toDateEdit->setFixedWidth(100);
    fLayout->addWidget(m_toDateEdit);

    QLabel* typeLbl = new QLabel("Type:", filterFrame);
    fLayout->addWidget(typeLbl);
    m_vchTypeCombo = new QComboBox(filterFrame);
    m_vchTypeCombo->addItem("All Types", "");
    m_vchTypeCombo->addItem("Payment", "Payment");
    m_vchTypeCombo->addItem("Receipt", "Receipt");
    m_vchTypeCombo->addItem("Contra", "Contra");
    m_vchTypeCombo->addItem("Journal", "Journal");
    m_vchTypeCombo->addItem("Sales", "Sales");
    m_vchTypeCombo->addItem("Purchase", "Purchase");
    connect(m_vchTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DayBookView::refreshData);
    fLayout->addWidget(m_vchTypeCombo);

    QPushButton* refreshBtn = new QPushButton("Refresh", filterFrame);
    refreshBtn->setStyleSheet("background-color: #1B4977; color: #FFFFFF; font-weight: bold; padding: 4px 12px;");
    connect(refreshBtn, &QPushButton::clicked, this, &DayBookView::refreshData);
    fLayout->addWidget(refreshBtn);

    mainLayout->addWidget(filterFrame);

    // Table
    m_table = new QTableWidget(0, 6, this);
    m_table->setHorizontalHeaderLabels({"Date", "Particulars", "Vch Type", "Vch No.", "Debit Amount (₹)", "Credit Amount (₹)"});
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
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &DayBookView::onRowDoubleClicked);
    mainLayout->addWidget(m_table);

    // Bottom Summary
    QFrame* footerFrame = new QFrame(this);
    footerFrame->setStyleSheet("background-color: #E2ECF5; border: 1px solid #B8D0E5; padding: 4px;");
    QHBoxLayout* footLayout = new QHBoxLayout(footerFrame);

    footLayout->addStretch();
    QLabel* drLbl = new QLabel("Total Debits: ₹ ", footerFrame);
    footLayout->addWidget(drLbl);
    m_totalDrLabel = new QLabel("0.00", footerFrame);
    m_totalDrLabel->setStyleSheet("font-weight: bold; font-size: 11pt;");
    footLayout->addWidget(m_totalDrLabel);

    footLayout->addSpacing(24);
    QLabel* crLbl = new QLabel("Total Credits: ₹ ", footerFrame);
    footLayout->addWidget(crLbl);
    m_totalCrLabel = new QLabel("0.00", footerFrame);
    m_totalCrLabel->setStyleSheet("font-weight: bold; font-size: 11pt;");
    footLayout->addWidget(m_totalCrLabel);

    mainLayout->addWidget(footerFrame);
}

void DayBookView::setCompanyId(const std::string& companyId) {
    m_companyId = companyId;
    refreshData();
}

void DayBookView::refreshData() {
    if (m_companyId.empty()) return;

    services::DayBookFilter filter;
    filter.companyId = m_companyId;
    filter.fromDate = m_fromDateEdit->text().trimmed().toStdString();
    filter.toDate = m_toDateEdit->text().trimmed().toStdString();
    if (m_vchTypeCombo && m_vchTypeCombo->currentIndex() > 0) {
        std::string catStr = m_vchTypeCombo->currentData().toString().toStdString();
        filter.category = domain::voucherCategoryFromString(catStr);
    }
    filter.limit = 500;

    auto result = m_dayBookService.getDayBook(filter);

    m_table->setSortingEnabled(false);
    m_table->setRowCount(0);
    for (const auto& item : result.items) {
        int r = m_table->rowCount();
        m_table->insertRow(r);

        bool isCancelled = (item.status == domain::VoucherStatus::Cancelled);

        auto* dateItem = new QTableWidgetItem(QString::fromStdString(item.date));
        dateItem->setData(Qt::UserRole, QString::fromStdString(item.id));
        m_table->setItem(r, 0, dateItem);
        m_table->setItem(r, 1, new QTableWidgetItem(QString::fromStdString(item.particulars)));
        m_table->setItem(r, 2, new QTableWidgetItem(QString::fromStdString(item.voucherTypeName)));
        m_table->setItem(r, 3, new QTableWidgetItem(QString::fromStdString(item.voucherNumber)));

        QString drText = "";
        QString crText = "";
        if (isCancelled) {
            drText = "0.00";
            crText = "0.00";
        } else if (item.voucherCategory == domain::VoucherCategory::Payment || item.voucherCategory == domain::VoucherCategory::Purchase) {
            drText = QString::fromStdString(item.totalAmount.formatInr(false));
        } else if (item.voucherCategory == domain::VoucherCategory::Receipt || item.voucherCategory == domain::VoucherCategory::Sales) {
            crText = QString::fromStdString(item.totalAmount.formatInr(false));
        } else {
            drText = QString::fromStdString(item.totalAmount.formatInr(false));
            crText = QString::fromStdString(item.totalAmount.formatInr(false));
        }

        auto* dItem = new QTableWidgetItem(drText);
        dItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(r, 4, dItem);

        auto* cItem = new QTableWidgetItem(crText);
        cItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(r, 5, cItem);

        if (isCancelled) {
            for (int col = 0; col < 6; ++col) {
                auto* itm = m_table->item(r, col);
                if (itm) {
                    itm->setForeground(QBrush(QColor("#888888")));
                    QFont f = itm->font();
                    f.setItalic(true);
                    itm->setFont(f);
                }
            }
        }
    }
    m_table->setSortingEnabled(true);

    m_totalDrLabel->setText(QString::fromStdString(result.totalDebitSum.formatInr(false)));
    m_totalCrLabel->setText(QString::fromStdString(result.totalCreditSum.formatInr(false)));
}

void DayBookView::onRowDoubleClicked(int row, int column) {
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

void DayBookView::keyPressEvent(QKeyEvent* event) {
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
