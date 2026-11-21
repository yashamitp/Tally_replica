#pragma once

#include <QDialog>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QLabel>
#include <string>
#include <vector>
#include "../../services/CompanyService.hpp"

namespace tally::ui {

class CompanyDialog : public QDialog {
    Q_OBJECT

public:
    explicit CompanyDialog(
        services::CompanyService& companyService,
        const std::string& currentCompanyId,
        QWidget* parent = nullptr
    );

    std::string selectedCompanyId() const { return m_selectedCompanyId; }
    bool companyChanged() const { return m_companyChanged; }

private slots:
    void onSelectCompany();
    void onCreateCompany();
    void onAlterCompany();
    void onShutCompany();
    void onDeleteCompany();
    void refreshCompanyList();

private:
    void setupSelectTab();
    void setupCreateTab();
    void setupAlterTab();

    services::CompanyService& m_companyService;
    std::string m_currentCompanyId;
    std::string m_selectedCompanyId;
    bool m_companyChanged = false;

    QTabWidget* m_tabWidget = nullptr;

    // Select Tab
    QListWidget* m_companyListWidget = nullptr;
    QPushButton* m_selectBtn = nullptr;
    QPushButton* m_shutBtn = nullptr;
    QPushButton* m_deleteBtn = nullptr;

    // Create Tab
    QLineEdit* m_createNameEdit = nullptr;
    QLineEdit* m_createFormalNameEdit = nullptr;
    QLineEdit* m_createAddressEdit = nullptr;
    QLineEdit* m_createCityEdit = nullptr;
    QLineEdit* m_createStateEdit = nullptr;
    QLineEdit* m_createPincodeEdit = nullptr;
    QLineEdit* m_createGstinEdit = nullptr;
    QLineEdit* m_createPanEdit = nullptr;
    QLineEdit* m_createFyStartEdit = nullptr;
    QLineEdit* m_createBooksBeginningEdit = nullptr;
    QLineEdit* m_createCurrencySymbolEdit = nullptr;
    QLineEdit* m_createCurrencyFormalEdit = nullptr;
    QLineEdit* m_createDecimalPlacesEdit = nullptr;

    // Alter Tab
    QLineEdit* m_alterNameEdit = nullptr;
    QLineEdit* m_alterFormalNameEdit = nullptr;
    QLineEdit* m_alterAddressEdit = nullptr;
    QLineEdit* m_alterCityEdit = nullptr;
    QLineEdit* m_alterStateEdit = nullptr;
    QLineEdit* m_alterPincodeEdit = nullptr;
    QLineEdit* m_alterGstinEdit = nullptr;
    QLineEdit* m_alterPanEdit = nullptr;
};

} // namespace tally::ui
