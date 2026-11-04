#include "VoucherDetailsDialog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include "../../utilities/DateUtils.hpp"

namespace tally::ui {

VoucherDetailsDialog::VoucherDetailsDialog(
    const std::string& companyId,
    const std::string& voucherId,
    services::VoucherService& voucherService,
    services::LedgerService& ledgerService,
    QWidget* parent
) : QDialog(parent),
    m_companyId(companyId),
    m_voucherId(voucherId),
    m_voucherService(voucherService),
    m_ledgerService(ledgerService) {
    setWindowTitle("Voucher Details & Audit Actions");
    setMinimumSize(650, 480);
    setupUi();
    loadVoucher();
}

void VoucherDetailsDialog::setupUi() {
    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(12);

    // Header info box
    QFrame* infoFrame = new QFrame(this);
    infoFrame->setStyleSheet("background-color: #E2ECF5; border: 1px solid #B8D0E5; padding: 6px;");
    QHBoxLayout* infoLayout = new QHBoxLayout(infoFrame);

    m_headerLabel = new QLabel("Voucher", infoFrame);
    m_headerLabel->setStyleSheet("font-size: 11pt; font-weight: bold; color: #1B4977;");
    infoLayout->addWidget(m_headerLabel);

    infoLayout->addStretch();

    m_statusLabel = new QLabel("Status: POSTED", infoFrame);
    m_statusLabel->setStyleSheet("font-weight: bold; color: #27AE60; font-size: 10pt;");
    infoLayout->addWidget(m_statusLabel);

    rootLayout->addWidget(infoFrame);

    // Entries Table
    m_table = new QTableWidget(0, 3, this);
    m_table->setHorizontalHeaderLabels({"Particulars (Ledger)", "Debit Amount (₹)", "Credit Amount (₹)"});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_table->setColumnWidth(1, 150);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_table->setColumnWidth(2, 150);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    rootLayout->addWidget(m_table);

    // Narration
    m_narrationLabel = new QLabel("Narration: None", this);
    m_narrationLabel->setStyleSheet("font-style: italic; color: #333333;");
    rootLayout->addWidget(m_narrationLabel);

    // Bottom action bar
    QHBoxLayout* btnLayout = new QHBoxLayout();

    m_reverseBtn = new QPushButton("Reverse Voucher (Counter-Voucher)", this);
    m_reverseBtn->setStyleSheet("background-color: #D35400; color: white; font-weight: bold; padding: 6px 12px;");
    connect(m_reverseBtn, &QPushButton::clicked, this, &VoucherDetailsDialog::onReverseClicked);
    btnLayout->addWidget(m_reverseBtn);

    m_cancelBtn = new QPushButton("Cancel Voucher", this);
    m_cancelBtn->setStyleSheet("background-color: #C0392B; color: white; font-weight: bold; padding: 6px 12px;");
    connect(m_cancelBtn, &QPushButton::clicked, this, &VoucherDetailsDialog::onCancelClicked);
    btnLayout->addWidget(m_cancelBtn);

    btnLayout->addStretch();

    QPushButton* closeBtn = new QPushButton("Close (Esc)", this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);

    rootLayout->addLayout(btnLayout);
}

void VoucherDetailsDialog::loadVoucher() {
    m_voucher = m_voucherService.getVoucher(m_voucherId);
    if (!m_voucher) {
        QMessageBox::warning(this, "Error", "Failed to retrieve voucher details.");
        return;
    }

    m_headerLabel->setText(QString("Voucher No: %1 | Date: %2")
        .arg(QString::fromStdString(m_voucher->voucherNumber))
        .arg(QString::fromStdString(m_voucher->date)));

    std::string statStr = domain::voucherStatusToString(m_voucher->status);
    m_statusLabel->setText(QString("Status: %1").arg(QString::fromStdString(statStr)));

    if (m_voucher->status == domain::VoucherStatus::Posted) {
        m_statusLabel->setStyleSheet("font-weight: bold; color: #27AE60; font-size: 10pt;");
        m_reverseBtn->setEnabled(true);
        m_cancelBtn->setEnabled(true);
    } else if (m_voucher->status == domain::VoucherStatus::Reversed) {
        m_statusLabel->setStyleSheet("font-weight: bold; color: #D35400; font-size: 10pt;");
        m_reverseBtn->setEnabled(false);
        m_cancelBtn->setEnabled(false);
    } else {
        m_statusLabel->setStyleSheet("font-weight: bold; color: #C0392B; font-size: 10pt;");
        m_reverseBtn->setEnabled(false);
        m_cancelBtn->setEnabled(false);
    }

    if (m_voucher->narration.has_value() && !m_voucher->narration->empty()) {
        m_narrationLabel->setText(QString("Narration: %1").arg(QString::fromStdString(*m_voucher->narration)));
    } else {
        m_narrationLabel->setText("Narration: None");
    }

    // Load ledger names for entries
    m_table->setRowCount(0);
    for (const auto& e : m_voucher->entries) {
        int r = m_table->rowCount();
        m_table->insertRow(r);

        std::string ledgerName = e.ledgerId;
        auto lOpt = m_ledgerService.getLedger(e.ledgerId);
        if (lOpt) ledgerName = lOpt->name;

        m_table->setItem(r, 0, new QTableWidgetItem(QString::fromStdString(ledgerName)));

        if (e.entryType == domain::BalanceType::Dr) {
            auto* dItem = new QTableWidgetItem(QString::fromStdString(e.amount.formatInr(false)));
            dItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            m_table->setItem(r, 1, dItem);
            m_table->setItem(r, 2, new QTableWidgetItem(""));
        } else {
            m_table->setItem(r, 1, new QTableWidgetItem(""));
            auto* cItem = new QTableWidgetItem(QString::fromStdString(e.amount.formatInr(false)));
            cItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            m_table->setItem(r, 2, cItem);
        }
    }
}

void VoucherDetailsDialog::onReverseClicked() {
    if (!m_voucher) return;

    bool ok;
    QString reason = QInputDialog::getText(
        this, "Reverse Voucher",
        "Enter reason for reversing this voucher:",
        QLineEdit::Normal, "Correction entry", &ok
    );
    if (!ok || reason.trimmed().isEmpty()) return;

    try {
        domain::ReverseVoucherParams params;
        params.companyId = m_companyId;
        params.voucherId = m_voucherId;
        params.reversalDate = utilities::DateUtils::currentIsoDate();
        params.reason = reason.trimmed().toStdString();

        auto revVch = m_voucherService.reverseVoucher(params);
        m_modified = true;
        QMessageBox::information(this, "Success",
            QString("Voucher reversed successfully!\nReversal Counter-Voucher created: %1")
                .arg(QString::fromStdString(revVch.voucherNumber)));
        loadVoucher();
    } catch (const std::exception& ex) {
        QMessageBox::critical(this, "Reversal Error", QString::fromStdString(ex.what()));
    }
}

void VoucherDetailsDialog::onCancelClicked() {
    if (!m_voucher) return;

    bool ok;
    QString reason = QInputDialog::getText(
        this, "Cancel Voucher",
        "Enter reason for cancelling this voucher:",
        QLineEdit::Normal, "Entered by mistake", &ok
    );
    if (!ok || reason.trimmed().isEmpty()) return;

    try {
        if (m_voucherService.cancelVoucher(m_companyId, m_voucherId, reason.trimmed().toStdString())) {
            m_modified = true;
            QMessageBox::information(this, "Success", "Voucher cancelled successfully! Transaction history is preserved.");
            loadVoucher();
        } else {
            QMessageBox::critical(this, "Error", "Failed to cancel voucher.");
        }
    } catch (const std::exception& ex) {
        QMessageBox::critical(this, "Cancellation Error", QString::fromStdString(ex.what()));
    }
}

} // namespace tally::ui
