#include "GroupDialog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QLabel>

namespace tally::ui {

GroupDialog::GroupDialog(
    const std::string& companyId,
    services::GroupService& groupService,
    QWidget* parent
) : QDialog(parent),
    m_companyId(companyId),
    m_groupService(groupService) {
    setWindowTitle("Group Creation");
    setupUi();
}

GroupDialog::GroupDialog(
    const std::string& companyId,
    const domain::AccountingGroup& existingGroup,
    services::GroupService& groupService,
    QWidget* parent
) : QDialog(parent),
    m_companyId(companyId),
    m_existingGroup(existingGroup),
    m_groupService(groupService) {
    setWindowTitle("Group Alteration");
    setupUi();
}

void GroupDialog::setupUi() {
    setMinimumWidth(440);
    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(12);

    QLabel* header = new QLabel(m_existingGroup ? "Group Alteration" : "Group Creation", this);
    header->setStyleSheet("font-size: 12pt; font-weight: bold; color: #1B4977;");
    rootLayout->addWidget(header);

    QFormLayout* form = new QFormLayout();
    form->setSpacing(8);

    m_nameEdit = new QLineEdit(this);
    form->addRow("Group Name *:", m_nameEdit);

    m_parentCombo = new QComboBox(this);
    populateParentGroups();
    form->addRow("Under (Parent Group) *:", m_parentCombo);

    m_natureCombo = new QComboBox(this);
    m_natureCombo->addItem("Assets", "ASSET");
    m_natureCombo->addItem("Liabilities", "LIABILITY");
    m_natureCombo->addItem("Income", "INCOME");
    m_natureCombo->addItem("Expenses", "EXPENSE");
    m_natureCombo->addItem("Equity", "EQUITY");
    form->addRow("Group Nature:", m_natureCombo);

    m_affectsGrossProfitCheck = new QCheckBox("Affects Gross Profit (Trading Account)", this);
    form->addRow("", m_affectsGrossProfitCheck);

    rootLayout->addLayout(form);

    if (m_existingGroup) {
        m_nameEdit->setText(QString::fromStdString(m_existingGroup->name));
        if (m_existingGroup->parentId) {
            for (int i = 0; i < m_parentCombo->count(); ++i) {
                if (m_parentCombo->itemData(i).toString().toStdString() == *m_existingGroup->parentId) {
                    m_parentCombo->setCurrentIndex(i);
                    break;
                }
            }
        } else {
            m_parentCombo->setCurrentIndex(0); // Primary
        }

        std::string natStr = domain::groupNatureToString(m_existingGroup->nature);
        for (int i = 0; i < m_natureCombo->count(); ++i) {
            if (m_natureCombo->itemData(i).toString().toStdString() == natStr) {
                m_natureCombo->setCurrentIndex(i);
                break;
            }
        }
        m_affectsGrossProfitCheck->setChecked(m_existingGroup->affectsGrossProfit);
    }

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    QPushButton* cancelBtn = new QPushButton("Cancel (Esc)", this);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn);

    QPushButton* saveBtn = new QPushButton(m_existingGroup ? "Save Alteration (Enter)" : "Create Group (Enter)", this);
    saveBtn->setStyleSheet("background-color: #F6C343; font-weight: bold; padding: 6px 16px; border: 1px solid #1B4977;");
    connect(saveBtn, &QPushButton::clicked, this, &GroupDialog::onSave);
    btnLayout->addWidget(saveBtn);

    rootLayout->addLayout(btnLayout);
    m_nameEdit->setFocus();
}

void GroupDialog::populateParentGroups() {
    m_parentCombo->clear();
    m_parentCombo->addItem("-- Primary --", "");
    auto list = m_groupService.listGroups(m_companyId);
    for (const auto& g : list) {
        if (!m_existingGroup || m_existingGroup->id != g.id) {
            m_parentCombo->addItem(QString::fromStdString(g.name), QString::fromStdString(g.id));
        }
    }
}

void GroupDialog::onSave() {
    QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Validation", "Group name is required.");
        return;
    }

    std::optional<std::string> parentId = std::nullopt;
    if (m_parentCombo->currentIndex() > 0) {
        parentId = m_parentCombo->currentData().toString().toStdString();
    }

    std::string natStr = m_natureCombo->currentData().toString().toStdString();
    domain::GroupNature nature = domain::groupNatureFromString(natStr);
    bool affectsGp = m_affectsGrossProfitCheck->isChecked();

    try {
        if (m_existingGroup) {
            domain::AccountingGroup g = *m_existingGroup;
            g.name = name.toStdString();
            g.parentId = parentId;
            g.nature = nature;
            g.affectsGrossProfit = affectsGp;

            if (m_groupService.alterGroup(g)) {
                m_saved = true;
                QMessageBox::information(this, "Success", "Group updated successfully.");
                accept();
            } else {
                QMessageBox::critical(this, "Error", "Failed to update group.");
            }
        } else {
            services::CreateGroupParams gp;
            gp.companyId = m_companyId;
            gp.name = name.toStdString();
            gp.parentId = parentId;
            gp.nature = nature;
            gp.affectsGrossProfit = affectsGp;
            gp.isPrimary = !parentId.has_value();

            m_groupService.createGroup(gp);
            m_saved = true;
            QMessageBox::information(this, "Success", "Group created successfully.");
            accept();
        }
    } catch (const std::exception& ex) {
        QMessageBox::critical(this, "Error", QString::fromStdString(ex.what()));
    }
}

} // namespace tally::ui
