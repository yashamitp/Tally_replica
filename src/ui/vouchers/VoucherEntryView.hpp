#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include "../../application/NavigationManager.hpp"
#include "../../services/LedgerService.hpp"
#include "../../services/VoucherService.hpp"
#include "../../services/CompanyService.hpp"

namespace tally::ui {

class VoucherEntryView : public QWidget {
    Q_OBJECT

public:
    VoucherEntryView(
        application::NavigationManager& navManager,
        services::CompanyService& companyService,
        services::LedgerService& ledgerService,
        services::VoucherService& voucherService,
        QWidget* parent = nullptr
    );

    void setCompanyId(const std::string& companyId);
    void setVoucherCategory(domain::VoucherCategory category);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onCellChanged(int row, int column);
    void onPostVoucher();
    void onCategoryButtonClicked(domain::VoucherCategory category);
    void onAddRowClicked();

private:
    void setupUi();
    void updateTotals();
    void loadLedgers();
    void addRow(const QString& defaultSide = "Dr");
    void updateVoucherTypeId();
    void updateNextVoucherNumber();
    void onRowSideChanged(int row, const QString& side);
    void updateDayName();

    application::NavigationManager& m_navManager;
    services::CompanyService& m_companyService;
    services::LedgerService& m_ledgerService;
    services::VoucherService& m_voucherService;

    std::string m_companyId;
    domain::VoucherCategory m_currentCategory = domain::VoucherCategory::Journal;
    std::string m_voucherTypeId;
    std::vector<domain::Ledger> m_availableLedgers;

    QLabel* m_vchTypeTitleLabel;
    QLineEdit* m_vchNumberEdit;
    QLineEdit* m_dateEdit;
    QLabel* m_dayLabel;
    QTableWidget* m_table;
    QLineEdit* m_narrationEdit;
    QLabel* m_totalDebitLabel;
    QLabel* m_totalCreditLabel;
    QLabel* m_balanceStatusLabel;
    QPushButton* m_postBtn;
};

} // namespace tally::ui
