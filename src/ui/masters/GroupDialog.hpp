#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <string>
#include <optional>
#include "../../domain/groups/AccountingGroup.hpp"
#include "../../services/GroupService.hpp"

namespace tally::ui {

class GroupDialog : public QDialog {
    Q_OBJECT

public:
    // Create Mode
    explicit GroupDialog(
        const std::string& companyId,
        services::GroupService& groupService,
        QWidget* parent = nullptr
    );

    // Alter Mode
    explicit GroupDialog(
        const std::string& companyId,
        const domain::AccountingGroup& existingGroup,
        services::GroupService& groupService,
        QWidget* parent = nullptr
    );

    bool isSaved() const { return m_saved; }

private slots:
    void onSave();

private:
    void setupUi();
    void populateParentGroups();

    std::string m_companyId;
    std::optional<domain::AccountingGroup> m_existingGroup;
    services::GroupService& m_groupService;
    bool m_saved = false;

    QLineEdit* m_nameEdit = nullptr;
    QComboBox* m_parentCombo = nullptr;
    QComboBox* m_natureCombo = nullptr;
    QCheckBox* m_affectsGrossProfitCheck = nullptr;
};

} // namespace tally::ui
