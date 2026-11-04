#include "LedgerDialog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>

namespace tally::ui {

LedgerDialog::LedgerDialog(
    const std::string& companyId,
    services::CompanyService& companyService,
    services::LedgerService& ledgerService,
    QWidget* parent
) : QDialog(parent),
    m_companyId(companyId),
    m_companyService(companyService),
    m_ledgerService(ledgerService) {
    setWindowTitle("Ledger Creation");
    setupUi();
}

LedgerDialog::LedgerDialog(
    const std::string& companyId,
    const domain::Ledger& existingLedger,
    services::CompanyService& companyService,
    services::LedgerService& ledgerService,
    QWidget* parent
) : QDialog(parent),
    m_companyId(companyId),
    m_existingLedger(existingLedger),
    m_companyService(companyService),
    m_ledgerService(ledgerService) {
    setWindowTitle("Ledger Alteration");
    setupUi();
}

void LedgerDialog::setupUi() {
    setMinimumWidth(480);
    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(12);

    QLabel* header = new QLabel(m_existingLedger ? "Ledger Alteration" : "Ledger Creation", this);
    header->setStyleSheet("font-size: 12pt; font-weight: bold; color: #1B4977;");
    rootLayout->addWidget(header);

    QFormLayout* form = new QFormLayout();
    form->setSpacing(8);

    m_nameEdit = new QLineEdit(this);
    form->addRow("Name *:", m_nameEdit);

    m_aliasEdit = new QLineEdit(this);
    form->addRow("Alias (optional):", m_aliasEdit);

    m_groupCombo = new QComboBox(this);
    populateGroups();
    form->addRow("Under (Group) *:", m_groupCombo);

    QHBoxLayout* balLayout = new QHBoxLayout();
    m_opBalanceEdit = new QLineEdit("0.00", this);
    m_opBalTypeCombo = new QComboBox(this);
    m_opBalTypeCombo->addItems({"Dr", "Cr"});
    balLayout->addWidget(m_opBalanceEdit);
    balLayout->addWidget(m_opBalTypeCombo);
    form->addRow("Opening Balance:", balLayout);

    m_gstinEdit = new QLineEdit(this);
    m_gstinEdit->setPlaceholderText("GSTIN (e.g. 27ABCDE1234F1Z5)");
    form->addRow("GSTIN / UIN:", m_gstinEdit);

    m_panEdit = new QLineEdit(this);
    form->addRow("PAN / IT No.:", m_panEdit);

    m_addressEdit = new QLineEdit(this);
    form->addRow("Address:", m_addressEdit);

    m_stateEdit = new QLineEdit("Maharashtra", this);
    form->addRow("State:", m_stateEdit);

    m_pincodeEdit = new QLineEdit(this);
    form->addRow("Pincode:", m_pincodeEdit);

    rootLayout->addLayout(form);

    // If alter mode, populate existing values
    if (m_existingLedger) {
        m_nameEdit->setText(QString::fromStdString(m_existingLedger->name));
        if (m_existingLedger->alias) m_aliasEdit->setText(QString::fromStdString(*m_existingLedger->alias));
        for (int i = 0; i < m_groupCombo->count(); ++i) {
            if (m_groupCombo->itemData(i).toString().toStdString() == m_existingLedger->groupId) {
                m_groupCombo->setCurrentIndex(i);
                break;
            }
        }
        m_opBalanceEdit->setText(QString::fromStdString(m_existingLedger->openingBalance.formatInr(false)));
        m_opBalTypeCombo->setCurrentText(m_existingLedger->openingBalanceType == domain::BalanceType::Dr ? "Dr" : "Cr");
        if (m_existingLedger->gstin) m_gstinEdit->setText(QString::fromStdString(*m_existingLedger->gstin));
        if (m_existingLedger->pan) m_panEdit->setText(QString::fromStdString(*m_existingLedger->pan));
        if (m_existingLedger->address) m_addressEdit->setText(QString::fromStdString(*m_existingLedger->address));
        if (m_existingLedger->state) m_stateEdit->setText(QString::fromStdString(*m_existingLedger->state));
        if (m_existingLedger->pincode) m_pincodeEdit->setText(QString::fromStdString(*m_existingLedger->pincode));
    }

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    QPushButton* cancelBtn = new QPushButton("Cancel (Esc)", this);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn);

    QPushButton* saveBtn = new QPushButton(m_existingLedger ? "Save Alteration (Enter)" : "Create Ledger (Enter)", this);
    saveBtn->setStyleSheet("background-color: #F6C343; font-weight: bold; padding: 6px 16px; border: 1px solid #1B4977;");
    connect(saveBtn, &QPushButton::clicked, this, &LedgerDialog::onSave);
    btnLayout->addWidget(saveBtn);

    rootLayout->addLayout(btnLayout);
    m_nameEdit->setFocus();
}

void LedgerDialog::populateGroups() {
    m_groupCombo->clear();
    auto groups = m_companyService.listGroups(m_companyId);
    for (const auto& g : groups) {
        m_groupCombo->addItem(QString::fromStdString(g.name), QString::fromStdString(g.id));
    }
}

void LedgerDialog::onSave() {
    QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Validation", "Ledger name is required.");
        return;
    }

    if (m_groupCombo->currentIndex() < 0) {
        QMessageBox::warning(this, "Validation", "Please select a parent Group.");
        return;
    }

    std::string groupId = m_groupCombo->currentData().toString().toStdString();
    domain::ExactDecimal opBal = domain::ExactDecimal::zero();
    try {
        if (!m_opBalanceEdit->text().trimmed().isEmpty()) {
            opBal = domain::ExactDecimal::fromString(m_opBalanceEdit->text().trimmed().toStdString());
        }
    } catch (...) {
        QMessageBox::warning(this, "Validation", "Invalid opening balance amount.");
        return;
    }

    domain::BalanceType balType = m_opBalTypeCombo->currentText() == "Cr" ? domain::BalanceType::Cr : domain::BalanceType::Dr;

    try {
        if (m_existingLedger) {
            domain::Ledger l = *m_existingLedger;
            l.name = name.toStdString();
            l.groupId = groupId;
            l.openingBalance = opBal;
            l.openingBalanceType = balType;
            l.alias = m_aliasEdit->text().trimmed().isEmpty() ? std::nullopt : std::optional<std::string>(m_aliasEdit->text().trimmed().toStdString());
            l.gstin = m_gstinEdit->text().trimmed().isEmpty() ? std::nullopt : std::optional<std::string>(m_gstinEdit->text().trimmed().toStdString());
            l.pan = m_panEdit->text().trimmed().isEmpty() ? std::nullopt : std::optional<std::string>(m_panEdit->text().trimmed().toStdString());
            l.address = m_addressEdit->text().trimmed().isEmpty() ? std::nullopt : std::optional<std::string>(m_addressEdit->text().trimmed().toStdString());
            l.state = m_stateEdit->text().trimmed().isEmpty() ? std::nullopt : std::optional<std::string>(m_stateEdit->text().trimmed().toStdString());
            l.pincode = m_pincodeEdit->text().trimmed().isEmpty() ? std::nullopt : std::optional<std::string>(m_pincodeEdit->text().trimmed().toStdString());

            if (m_ledgerService.alterLedger(l)) {
                m_saved = true;
                QMessageBox::information(this, "Success", "Ledger updated successfully.");
                accept();
            } else {
                QMessageBox::critical(this, "Error", "Failed to update ledger.");
            }
        } else {
            services::CreateLedgerParams lp;
            lp.companyId = m_companyId;
            lp.groupId = groupId;
            lp.name = name.toStdString();
            if (!m_aliasEdit->text().trimmed().isEmpty()) lp.alias = m_aliasEdit->text().trimmed().toStdString();
            lp.openingBalance = opBal;
            lp.openingBalanceType = balType;
            if (!m_gstinEdit->text().trimmed().isEmpty()) lp.gstin = m_gstinEdit->text().trimmed().toStdString();
            if (!m_panEdit->text().trimmed().isEmpty()) lp.pan = m_panEdit->text().trimmed().toStdString();
            if (!m_addressEdit->text().trimmed().isEmpty()) lp.address = m_addressEdit->text().trimmed().toStdString();
            if (!m_stateEdit->text().trimmed().isEmpty()) lp.state = m_stateEdit->text().trimmed().toStdString();
            if (!m_pincodeEdit->text().trimmed().isEmpty()) lp.pincode = m_pincodeEdit->text().trimmed().toStdString();

            m_ledgerService.createLedger(lp);
            m_saved = true;
            QMessageBox::information(this, "Success", "Ledger created successfully.");
            accept();
        }
    } catch (const std::exception& ex) {
        QMessageBox::critical(this, "Error", QString::fromStdString(ex.what()));
    }
}

} // namespace tally::ui
