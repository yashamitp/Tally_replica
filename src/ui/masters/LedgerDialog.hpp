#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <string>
#include <optional>
#include "../../domain/ledger/Ledger.hpp"
#include "../../services/LedgerService.hpp"
#include "../../services/CompanyService.hpp"

namespace tally::ui {

class LedgerDialog : public QDialog {
    Q_OBJECT

public:
    // Create Mode Constructor
    explicit LedgerDialog(
        const std::string& companyId,
        services::CompanyService& companyService,
        services::LedgerService& ledgerService,
        QWidget* parent = nullptr
    );

    // Alter Mode Constructor
    explicit LedgerDialog(
        const std::string& companyId,
        const domain::Ledger& existingLedger,
        services::CompanyService& companyService,
        services::LedgerService& ledgerService,
        QWidget* parent = nullptr
    );

    bool isSaved() const { return m_saved; }

private slots:
    void onSave();

private:
    void setupUi();
    void populateGroups();

    std::string m_companyId;
    std::optional<domain::Ledger> m_existingLedger;
    services::CompanyService& m_companyService;
    services::LedgerService& m_ledgerService;
    bool m_saved = false;

    QLineEdit* m_nameEdit = nullptr;
    QLineEdit* m_aliasEdit = nullptr;
    QComboBox* m_groupCombo = nullptr;
    QLineEdit* m_opBalanceEdit = nullptr;
    QComboBox* m_opBalTypeCombo = nullptr;
    QLineEdit* m_gstinEdit = nullptr;
    QLineEdit* m_panEdit = nullptr;
    QLineEdit* m_addressEdit = nullptr;
    QLineEdit* m_stateEdit = nullptr;
    QLineEdit* m_pincodeEdit = nullptr;
};

} // namespace tally::ui
