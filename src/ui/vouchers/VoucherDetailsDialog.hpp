#pragma once

#include <QDialog>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <string>
#include "../../services/VoucherService.hpp"
#include "../../services/LedgerService.hpp"

namespace tally::ui {

class VoucherDetailsDialog : public QDialog {
    Q_OBJECT

public:
    explicit VoucherDetailsDialog(
        const std::string& companyId,
        const std::string& voucherId,
        services::VoucherService& voucherService,
        services::LedgerService& ledgerService,
        QWidget* parent = nullptr
    );

    bool voucherModified() const { return m_modified; }

private slots:
    void onReverseClicked();
    void onCancelClicked();

private:
    void setupUi();
    void loadVoucher();

    std::string m_companyId;
    std::string m_voucherId;
    services::VoucherService& m_voucherService;
    services::LedgerService& m_ledgerService;
    std::optional<domain::Voucher> m_voucher;
    bool m_modified = false;

    QLabel* m_headerLabel = nullptr;
    QLabel* m_statusLabel = nullptr;
    QLabel* m_totalLabel = nullptr;
    QLabel* m_narrationLabel = nullptr;
    QTableWidget* m_table = nullptr;
    QPushButton* m_reverseBtn = nullptr;
    QPushButton* m_cancelBtn = nullptr;
};

} // namespace tally::ui
