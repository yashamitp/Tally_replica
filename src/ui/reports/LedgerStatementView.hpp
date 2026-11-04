#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include "../../application/NavigationManager.hpp"
#include "../../services/LedgerService.hpp"
#include "../../services/VoucherService.hpp"

namespace tally::ui {

class LedgerStatementView : public QWidget {
    Q_OBJECT

public:
    LedgerStatementView(
        application::NavigationManager& navManager,
        services::LedgerService& ledgerService,
        services::VoucherService& voucherService,
        QWidget* parent = nullptr
    );

    void setCompanyId(const std::string& companyId);
    void setLedgerId(const std::string& ledgerId);
    void refreshData();

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onLedgerSelectionChanged(int index);
    void onRowDoubleClicked(int row, int column);

private:
    void setupUi();
    void loadLedgersCombo();

    application::NavigationManager& m_navManager;
    services::LedgerService& m_ledgerService;
    services::VoucherService& m_voucherService;
    std::string m_companyId;
    std::string m_selectedLedgerId;

    QComboBox* m_ledgerCombo;
    QLineEdit* m_fromDateEdit;
    QLineEdit* m_toDateEdit;
    QTableWidget* m_table;
    QLabel* m_openingBalanceLabel;
    QLabel* m_closingBalanceLabel;
    QLabel* m_totalDrLabel;
    QLabel* m_totalCrLabel;
};

} // namespace tally::ui
