#include "VoucherEntryView.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QComboBox>
#include <QMessageBox>
#include <QKeyEvent>
#include <QUuid>
#include "../../utilities/DateUtils.hpp"

namespace tally::ui {

VoucherEntryView::VoucherEntryView(
    application::NavigationManager& navManager,
    services::CompanyService& companyService,
    services::LedgerService& ledgerService,
    services::VoucherService& voucherService,
    QWidget* parent
) : QWidget(parent),
    m_navManager(navManager),
    m_companyService(companyService),
    m_ledgerService(ledgerService),
    m_voucherService(voucherService) {
    setupUi();
}

void VoucherEntryView::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 8, 12, 8);
    mainLayout->setSpacing(6);

    // Top Header: Voucher Header Info
    QFrame* headerFrame = new QFrame(this);
    headerFrame->setStyleSheet("background-color: #E2ECF5; border: 1px solid #B8D0E5; padding: 4px;");
    QHBoxLayout* hLayout = new QHBoxLayout(headerFrame);
    hLayout->setContentsMargins(6, 4, 6, 4);

    m_vchTypeTitleLabel = new QLabel("Accounting Voucher Creation: Journal", headerFrame);
    m_vchTypeTitleLabel->setStyleSheet("font-size: 11pt; font-weight: bold; color: #1B4977;");
    hLayout->addWidget(m_vchTypeTitleLabel);

    hLayout->addStretch();

    QLabel* noLabel = new QLabel("No.:", headerFrame);
    noLabel->setStyleSheet("font-weight: bold;");
    hLayout->addWidget(noLabel);

    m_vchNumberEdit = new QLineEdit("1", headerFrame);
    m_vchNumberEdit->setFixedWidth(90);
    hLayout->addWidget(m_vchNumberEdit);

    hLayout->addSpacing(16);

    QLabel* dateLabel = new QLabel("Date:", headerFrame);
    dateLabel->setStyleSheet("font-weight: bold;");
    hLayout->addWidget(dateLabel);

    m_dateEdit = new QLineEdit("2026-04-01", headerFrame);
    m_dateEdit->setFixedWidth(100);
    hLayout->addWidget(m_dateEdit);

    m_dayLabel = new QLabel("Wednesday", headerFrame);
    m_dayLabel->setStyleSheet("color: #556B82; font-style: italic;");
    hLayout->addWidget(m_dayLabel);

    mainLayout->addWidget(headerFrame);

    // Entries Table (Dr/Cr, Particulars, Debit Amount, Credit Amount)
    m_table = new QTableWidget(6, 4, this);
    m_table->setHorizontalHeaderLabels({"Dr/Cr", "Particulars (Ledger)", "Debit Amount", "Credit Amount"});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_table->setColumnWidth(0, 80);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_table->setColumnWidth(2, 140);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    m_table->setColumnWidth(3, 140);

    for (int r = 0; r < m_table->rowCount(); ++r) {
        QComboBox* drcrCombo = new QComboBox(m_table);
        drcrCombo->addItems({"Dr", "Cr"});
        if (r == 1) drcrCombo->setCurrentIndex(1); // Second row default Cr
        m_table->setCellWidget(r, 0, drcrCombo);

        QComboBox* ledgerCombo = new QComboBox(m_table);
        m_table->setCellWidget(r, 1, ledgerCombo);

        m_table->setItem(r, 2, new QTableWidgetItem(""));
        m_table->setItem(r, 3, new QTableWidgetItem(""));
    }

    mainLayout->addWidget(m_table);

    // Narration Bar
    QHBoxLayout* nLayout = new QHBoxLayout();
    QLabel* narLabel = new QLabel("Narration:", this);
    narLabel->setStyleSheet("font-weight: bold;");
    nLayout->addWidget(narLabel);

    m_narrationEdit = new QLineEdit(this);
    m_narrationEdit->setPlaceholderText("Enter optional narration...");
    nLayout->addWidget(m_narrationEdit);
    mainLayout->addLayout(nLayout);

    // Bottom Summary & Actions
    QFrame* footerFrame = new QFrame(this);
    footerFrame->setStyleSheet("background-color: #E2ECF5; border: 1px solid #B8D0E5; padding: 4px;");
    QHBoxLayout* fLayout = new QHBoxLayout(footerFrame);

    m_balanceStatusLabel = new QLabel("Status: Unbalanced", footerFrame);
    m_balanceStatusLabel->setStyleSheet("font-weight: bold; color: #C0392B;");
    fLayout->addWidget(m_balanceStatusLabel);

    fLayout->addStretch();

    QLabel* dTotLbl = new QLabel("Total Debit: ₹ ", footerFrame);
    fLayout->addWidget(dTotLbl);
    m_totalDebitLabel = new QLabel("0.00", footerFrame);
    m_totalDebitLabel->setStyleSheet("font-weight: bold; font-size: 11pt;");
    fLayout->addWidget(m_totalDebitLabel);

    fLayout->addSpacing(20);

    QLabel* cTotLbl = new QLabel("Total Credit: ₹ ", footerFrame);
    fLayout->addWidget(cTotLbl);
    m_totalCreditLabel = new QLabel("0.00", footerFrame);
    m_totalCreditLabel->setStyleSheet("font-weight: bold; font-size: 11pt;");
    fLayout->addWidget(m_totalCreditLabel);

    fLayout->addSpacing(20);

    m_postBtn = new QPushButton("Accept (Save)", footerFrame);
    m_postBtn->setStyleSheet("background-color: #F6C343; font-weight: bold; padding: 6px 16px; border: 1px solid #1B4977;");
    fLayout->addWidget(m_postBtn);

    mainLayout->addWidget(footerFrame);

    connect(m_table, &QTableWidget::cellChanged, this, &VoucherEntryView::onCellChanged);
    connect(m_postBtn, &QPushButton::clicked, this, &VoucherEntryView::onPostVoucher);
}

void VoucherEntryView::setCompanyId(const std::string& companyId) {
    m_companyId = companyId;
    loadLedgers();
}

void VoucherEntryView::setVoucherCategory(domain::VoucherCategory category) {
    m_currentCategory = category;
    std::string catName = domain::voucherCategoryToString(category);
    m_vchTypeTitleLabel->setText(QString("Accounting Voucher Creation: %1").arg(QString::fromStdString(catName)));

    // Find voucher type ID
    if (!m_companyId.empty()) {
        auto vtypes = m_companyService.listVoucherTypes(m_companyId);
        for (const auto& vt : vtypes) {
            if (vt.category == category) {
                m_voucherTypeId = vt.id;
                break;
            }
        }
    }
}

void VoucherEntryView::loadLedgers() {
    if (m_companyId.empty()) return;
    m_availableLedgers = m_ledgerService.listLedgers(m_companyId);

    for (int r = 0; r < m_table->rowCount(); ++r) {
        auto* combo = qobject_cast<QComboBox*>(m_table->cellWidget(r, 1));
        if (combo) {
            combo->clear();
            combo->addItem("-- Select Ledger --", "");
            for (const auto& l : m_availableLedgers) {
                combo->addItem(QString::fromStdString(l.name), QString::fromStdString(l.id));
            }
        }
    }
}

void VoucherEntryView::onCellChanged(int row, int column) {
    Q_UNUSED(row);
    Q_UNUSED(column);
    updateTotals();
}

void VoucherEntryView::updateTotals() {
    domain::ExactDecimal sumDr = domain::ExactDecimal::zero();
    domain::ExactDecimal sumCr = domain::ExactDecimal::zero();

    for (int r = 0; r < m_table->rowCount(); ++r) {
        auto* drcrCombo = qobject_cast<QComboBox*>(m_table->cellWidget(r, 0));
        QString side = drcrCombo ? drcrCombo->currentText() : "Dr";

        auto* dItem = m_table->item(r, 2);
        auto* cItem = m_table->item(r, 3);

        if (dItem && !dItem->text().trimmed().isEmpty()) {
            try {
                domain::ExactDecimal amt = domain::ExactDecimal::fromString(dItem->text().trimmed().toStdString());
                sumDr = sumDr.add(amt);
            } catch (...) {}
        }
        if (cItem && !cItem->text().trimmed().isEmpty()) {
            try {
                domain::ExactDecimal amt = domain::ExactDecimal::fromString(cItem->text().trimmed().toStdString());
                sumCr = sumCr.add(amt);
            } catch (...) {}
        }
    }

    m_totalDebitLabel->setText(QString::fromStdString(sumDr.formatInr(false)));
    m_totalCreditLabel->setText(QString::fromStdString(sumCr.formatInr(false)));

    if (sumDr > domain::ExactDecimal::zero() && sumDr == sumCr) {
        m_balanceStatusLabel->setText("Status: Balanced ✓");
        m_balanceStatusLabel->setStyleSheet("font-weight: bold; color: #27AE60;");
    } else {
        domain::ExactDecimal diff = (sumDr.sub(sumCr)).abs();
        m_balanceStatusLabel->setText(QString("Status: Out of Balance (Diff: ₹ %1)").arg(QString::fromStdString(diff.formatInr(false))));
        m_balanceStatusLabel->setStyleSheet("font-weight: bold; color: #C0392B;");
    }
}

void VoucherEntryView::onPostVoucher() {
    std::vector<domain::VoucherEntry> entries;

    for (int r = 0; r < m_table->rowCount(); ++r) {
        auto* lCombo = qobject_cast<QComboBox*>(m_table->cellWidget(r, 1));
        if (!lCombo || lCombo->currentIndex() <= 0) continue;

        std::string ledgerId = lCombo->currentData().toString().toStdString();
        auto* drcrCombo = qobject_cast<QComboBox*>(m_table->cellWidget(r, 0));
        domain::BalanceType btype = (drcrCombo && drcrCombo->currentText() == "Cr") ? domain::BalanceType::Cr : domain::BalanceType::Dr;

        auto* dItem = m_table->item(r, 2);
        auto* cItem = m_table->item(r, 3);
        QString amtStr = (btype == domain::BalanceType::Dr && dItem) ? dItem->text().trimmed() : ((cItem) ? cItem->text().trimmed() : "");

        if (amtStr.isEmpty()) continue;

        try {
            domain::ExactDecimal amt = domain::ExactDecimal::fromString(amtStr.toStdString());
            if (amt.isPositive()) {
                domain::VoucherEntry e;
                e.ledgerId = ledgerId;
                e.entryType = btype;
                e.amount = amt;
                entries.push_back(e);
            }
        } catch (...) {}
    }

    if (entries.size() < 2) {
        QMessageBox::warning(this, "Validation Error", "A valid voucher requires at least two line items (at least one Debit and one Credit).");
        return;
    }

    // Prompt user with Tally style Accept? Yes / No
    auto reply = QMessageBox::question(this, "Accept?", "Accept? (Yes / No)", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if (reply != QMessageBox::Yes) {
        return;
    }

    try {
        domain::Voucher v;
        v.companyId = m_companyId;
        v.voucherTypeId = m_voucherTypeId;
        v.voucherNumber = m_vchNumberEdit->text().trimmed().toStdString();
        v.date = m_dateEdit->text().trimmed().toStdString();
        v.narration = m_narrationEdit->text().trimmed().toStdString();
        v.entries = std::move(entries);

        m_voucherService.postVoucher(v);

        QMessageBox::information(this, "Success", "Voucher posted successfully!");

        // Increment voucher number
        int nextNo = 1;
        try { nextNo = std::stoi(v.voucherNumber) + 1; } catch (...) {}
        m_vchNumberEdit->setText(QString::number(nextNo));

        // Clear entries
        for (int r = 0; r < m_table->rowCount(); ++r) {
            auto* lCombo = qobject_cast<QComboBox*>(m_table->cellWidget(r, 1));
            if (lCombo) lCombo->setCurrentIndex(0);
            if (m_table->item(r, 2)) m_table->item(r, 2)->setText("");
            if (m_table->item(r, 3)) m_table->item(r, 3)->setText("");
        }
        m_narrationEdit->clear();
        updateTotals();
    } catch (const std::exception& ex) {
        QMessageBox::critical(this, "Posting Error", QString::fromStdString(ex.what()));
    }
}

void VoucherEntryView::onCategoryButtonClicked(domain::VoucherCategory category) {
    setVoucherCategory(category);
}

void VoucherEntryView::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_F4) setVoucherCategory(domain::VoucherCategory::Contra);
    else if (event->key() == Qt::Key_F5) setVoucherCategory(domain::VoucherCategory::Payment);
    else if (event->key() == Qt::Key_F6) setVoucherCategory(domain::VoucherCategory::Receipt);
    else if (event->key() == Qt::Key_F7) setVoucherCategory(domain::VoucherCategory::Journal);
    else if (event->key() == Qt::Key_F8) setVoucherCategory(domain::VoucherCategory::Sales);
    else if (event->key() == Qt::Key_F9) setVoucherCategory(domain::VoucherCategory::Purchase);
    else if (event->key() == Qt::Key_Escape) m_navManager.navigateBack();
    else QWidget::keyPressEvent(event);
}

} // namespace tally::ui
