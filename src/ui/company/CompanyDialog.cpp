#include "CompanyDialog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QHeaderView>

namespace tally::ui {

CompanyDialog::CompanyDialog(
    services::CompanyService& companyService,
    const std::string& currentCompanyId,
    QWidget* parent
) : QDialog(parent),
    m_companyService(companyService),
    m_currentCompanyId(currentCompanyId),
    m_selectedCompanyId(currentCompanyId) {
    setWindowTitle("Company Management (Alt+K / F3)");
    setMinimumSize(560, 480);

    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    m_tabWidget = new QTabWidget(this);

    setupSelectTab();
    setupCreateTab();
    setupAlterTab();

    rootLayout->addWidget(m_tabWidget);

    refreshCompanyList();
}

void CompanyDialog::setupSelectTab() {
    QWidget* tab = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QLabel* infoLbl = new QLabel("Select Company from available companies:", tab);
    infoLbl->setStyleSheet("font-weight: bold; color: #1B4977;");
    layout->addWidget(infoLbl);

    m_companyListWidget = new QListWidget(tab);
    m_companyListWidget->setStyleSheet("font-size: 10pt;");
    layout->addWidget(m_companyListWidget);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    m_selectBtn = new QPushButton("Select Company (Enter)", tab);
    m_selectBtn->setStyleSheet("background-color: #1B4977; color: white; font-weight: bold; padding: 6px 16px;");
    connect(m_selectBtn, &QPushButton::clicked, this, &CompanyDialog::onSelectCompany);
    connect(m_companyListWidget, &QListWidget::itemDoubleClicked, this, &CompanyDialog::onSelectCompany);
    btnLayout->addWidget(m_selectBtn);

    layout->addLayout(btnLayout);
    m_tabWidget->addTab(tab, "Select Company");
}

void CompanyDialog::setupCreateTab() {
    QWidget* tab = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QFormLayout* form = new QFormLayout();
    form->setSpacing(8);

    m_createNameEdit = new QLineEdit(tab);
    form->addRow("Company Name *:", m_createNameEdit);

    m_createFormalNameEdit = new QLineEdit(tab);
    form->addRow("Formal / Trade Name:", m_createFormalNameEdit);

    m_createAddressEdit = new QLineEdit(tab);
    form->addRow("Address:", m_createAddressEdit);

    m_createCityEdit = new QLineEdit(tab);
    form->addRow("City:", m_createCityEdit);

    m_createStateEdit = new QLineEdit(tab);
    m_createStateEdit->setText("Maharashtra");
    form->addRow("State:", m_createStateEdit);

    m_createPincodeEdit = new QLineEdit(tab);
    form->addRow("Pincode:", m_createPincodeEdit);

    m_createGstinEdit = new QLineEdit(tab);
    m_createGstinEdit->setPlaceholderText("27AAAAA0000A1Z5");
    form->addRow("GSTIN:", m_createGstinEdit);

    m_createPanEdit = new QLineEdit(tab);
    form->addRow("PAN:", m_createPanEdit);

    m_createFyStartEdit = new QLineEdit("2026-04-01", tab);
    form->addRow("Financial Year Begins From:", m_createFyStartEdit);

    layout->addLayout(form);
    layout->addStretch();

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    QPushButton* createBtn = new QPushButton("Create Company", tab);
    createBtn->setStyleSheet("background-color: #F6C343; font-weight: bold; padding: 6px 16px; border: 1px solid #1B4977;");
    connect(createBtn, &QPushButton::clicked, this, &CompanyDialog::onCreateCompany);
    btnLayout->addWidget(createBtn);

    layout->addLayout(btnLayout);
    m_tabWidget->addTab(tab, "Create Company");
}

void CompanyDialog::setupAlterTab() {
    QWidget* tab = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(tab);

    auto comp = m_companyService.getCompany(m_currentCompanyId);

    QFormLayout* form = new QFormLayout();
    form->setSpacing(8);

    m_alterNameEdit = new QLineEdit(tab);
    if (comp) m_alterNameEdit->setText(QString::fromStdString(comp->name));
    form->addRow("Company Name *:", m_alterNameEdit);

    m_alterFormalNameEdit = new QLineEdit(tab);
    if (comp && comp->formalName) m_alterFormalNameEdit->setText(QString::fromStdString(*comp->formalName));
    form->addRow("Formal / Trade Name:", m_alterFormalNameEdit);

    m_alterAddressEdit = new QLineEdit(tab);
    if (comp && comp->addressLine1) m_alterAddressEdit->setText(QString::fromStdString(*comp->addressLine1));
    form->addRow("Address:", m_alterAddressEdit);

    m_alterCityEdit = new QLineEdit(tab);
    if (comp && comp->city) m_alterCityEdit->setText(QString::fromStdString(*comp->city));
    form->addRow("City:", m_alterCityEdit);

    m_alterStateEdit = new QLineEdit(tab);
    if (comp && comp->state) m_alterStateEdit->setText(QString::fromStdString(*comp->state));
    form->addRow("State:", m_alterStateEdit);

    m_alterPincodeEdit = new QLineEdit(tab);
    if (comp && comp->pincode) m_alterPincodeEdit->setText(QString::fromStdString(*comp->pincode));
    form->addRow("Pincode:", m_alterPincodeEdit);

    m_alterGstinEdit = new QLineEdit(tab);
    if (comp && comp->gstin) m_alterGstinEdit->setText(QString::fromStdString(*comp->gstin));
    form->addRow("GSTIN:", m_alterGstinEdit);

    m_alterPanEdit = new QLineEdit(tab);
    if (comp && comp->pan) m_alterPanEdit->setText(QString::fromStdString(*comp->pan));
    form->addRow("PAN:", m_alterPanEdit);

    layout->addLayout(form);
    layout->addStretch();

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    QPushButton* saveBtn = new QPushButton("Save Changes", tab);
    saveBtn->setStyleSheet("background-color: #1B4977; color: white; font-weight: bold; padding: 6px 16px;");
    connect(saveBtn, &QPushButton::clicked, this, &CompanyDialog::onAlterCompany);
    btnLayout->addWidget(saveBtn);

    layout->addLayout(btnLayout);
    m_tabWidget->addTab(tab, "Alter Company");
}

void CompanyDialog::refreshCompanyList() {
    m_companyListWidget->clear();
    auto list = m_companyService.listCompanies();
    int currentIdx = -1;
    for (size_t i = 0; i < list.size(); ++i) {
        const auto& c = list[i];
        QString label = QString::fromStdString(c.name);
        if (c.id == m_currentCompanyId) {
            label += " (Active)";
            currentIdx = static_cast<int>(i);
        }
        QListWidgetItem* item = new QListWidgetItem(label, m_companyListWidget);
        item->setData(Qt::UserRole, QString::fromStdString(c.id));
    }
    if (currentIdx >= 0) {
        m_companyListWidget->setCurrentRow(currentIdx);
    } else if (m_companyListWidget->count() > 0) {
        m_companyListWidget->setCurrentRow(0);
    }
}

void CompanyDialog::onSelectCompany() {
    auto* item = m_companyListWidget->currentItem();
    if (!item) return;
    m_selectedCompanyId = item->data(Qt::UserRole).toString().toStdString();
    m_companyChanged = true;
    accept();
}

void CompanyDialog::onCreateCompany() {
    QString name = m_createNameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Validation", "Company Name cannot be empty.");
        return;
    }

    try {
        services::CreateCompanyParams cp;
        cp.name = name.toStdString();
        if (!m_createFormalNameEdit->text().trimmed().isEmpty()) cp.formalName = m_createFormalNameEdit->text().trimmed().toStdString();
        if (!m_createAddressEdit->text().trimmed().isEmpty()) cp.addressLine1 = m_createAddressEdit->text().trimmed().toStdString();
        if (!m_createCityEdit->text().trimmed().isEmpty()) cp.city = m_createCityEdit->text().trimmed().toStdString();
        if (!m_createStateEdit->text().trimmed().isEmpty()) cp.state = m_createStateEdit->text().trimmed().toStdString();
        if (!m_createPincodeEdit->text().trimmed().isEmpty()) cp.pincode = m_createPincodeEdit->text().trimmed().toStdString();
        if (!m_createGstinEdit->text().trimmed().isEmpty()) cp.gstin = m_createGstinEdit->text().trimmed().toStdString();
        if (!m_createPanEdit->text().trimmed().isEmpty()) cp.pan = m_createPanEdit->text().trimmed().toStdString();
        cp.financialYearStart = m_createFyStartEdit->text().trimmed().toStdString();
        cp.booksBeginningFrom = cp.financialYearStart;

        auto comp = m_companyService.createCompany(cp);
        m_selectedCompanyId = comp.id;
        m_companyChanged = true;
        QMessageBox::information(this, "Success", "Company created successfully with standard Chart of Accounts!");
        accept();
    } catch (const std::exception& ex) {
        QMessageBox::critical(this, "Creation Error", QString::fromStdString(ex.what()));
    }
}

void CompanyDialog::onAlterCompany() {
    auto compOpt = m_companyService.getCompany(m_currentCompanyId);
    if (!compOpt) {
        QMessageBox::warning(this, "Error", "Current company not found.");
        return;
    }

    auto comp = *compOpt;
    comp.name = m_alterNameEdit->text().trimmed().toStdString();
    if (comp.name.empty()) {
        QMessageBox::warning(this, "Validation", "Company Name cannot be empty.");
        return;
    }

    comp.formalName = m_alterFormalNameEdit->text().trimmed().toStdString();
    comp.addressLine1 = m_alterAddressEdit->text().trimmed().toStdString();
    comp.city = m_alterCityEdit->text().trimmed().toStdString();
    comp.state = m_alterStateEdit->text().trimmed().toStdString();
    comp.pincode = m_alterPincodeEdit->text().trimmed().toStdString();
    comp.gstin = m_alterGstinEdit->text().trimmed().toStdString();
    comp.pan = m_alterPanEdit->text().trimmed().toStdString();

    try {
        if (m_companyService.alterCompany(comp)) {
            m_companyChanged = true;
            QMessageBox::information(this, "Success", "Company details updated successfully.");
            accept();
        } else {
            QMessageBox::critical(this, "Error", "Failed to update company.");
        }
    } catch (const std::exception& ex) {
        QMessageBox::critical(this, "Error", QString::fromStdString(ex.what()));
    }
}

} // namespace tally::ui
