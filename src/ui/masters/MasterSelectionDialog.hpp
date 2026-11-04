#pragma once

#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <string>
#include "../../services/CompanyService.hpp"
#include "../../services/GroupService.hpp"
#include "../../services/LedgerService.hpp"

namespace tally::ui {

enum class MasterMode {
    Create,
    Alter
};

class MasterSelectionDialog : public QDialog {
    Q_OBJECT

public:
    explicit MasterSelectionDialog(
        MasterMode mode,
        const std::string& companyId,
        services::CompanyService& companyService,
        services::GroupService& groupService,
        services::LedgerService& ledgerService,
        QWidget* parent = nullptr
    );

private slots:
    void onMasterTypeSelected();
    void onSpecificItemSelected();

private:
    void setupUi();
    void showGroupSelection();
    void showLedgerSelection();

    MasterMode m_mode;
    std::string m_companyId;
    services::CompanyService& m_companyService;
    services::GroupService& m_groupService;
    services::LedgerService& m_ledgerService;

    QLabel* m_titleLabel = nullptr;
    QListWidget* m_typeListWidget = nullptr;
    QListWidget* m_itemsListWidget = nullptr;
    QLineEdit* m_searchEdit = nullptr;
    QPushButton* m_actionBtn = nullptr;
    QPushButton* m_backBtn = nullptr;

    bool m_selectingSpecificItem = false;
    std::string m_selectedMasterType; // "Group" or "Ledger"
};

} // namespace tally::ui
