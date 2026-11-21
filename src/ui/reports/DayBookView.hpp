#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include "../../application/NavigationManager.hpp"
#include "../../services/DayBookService.hpp"
#include "../../services/VoucherService.hpp"
#include "../../services/LedgerService.hpp"

namespace tally::ui {

class DayBookView : public QWidget {
    Q_OBJECT

public:
    DayBookView(
        application::NavigationManager& navManager,
        services::DayBookService& dayBookService,
        services::VoucherService& voucherService,
        services::LedgerService& ledgerService,
        QWidget* parent = nullptr
    );

    void setCompanyId(const std::string& companyId);
    void refreshData();

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onRowDoubleClicked(int row, int column);

private:
    void setupUi();

    application::NavigationManager& m_navManager;
    services::DayBookService& m_dayBookService;
    services::VoucherService& m_voucherService;
    services::LedgerService& m_ledgerService;
    std::string m_companyId;

    QLineEdit* m_fromDateEdit;
    QLineEdit* m_toDateEdit;
    QComboBox* m_vchTypeCombo;
    QTableWidget* m_table;
    QLabel* m_totalDrLabel;
    QLabel* m_totalCrLabel;
};

} // namespace tally::ui
