#pragma once

#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include "../../application/NavigationManager.hpp"
#include "../../services/CompanyService.hpp"

namespace tally::ui {

class GatewayView : public QWidget {
    Q_OBJECT

public:
    GatewayView(
        application::NavigationManager& navManager,
        services::CompanyService& companyService,
        QWidget* parent = nullptr
    );

    void refreshCompanyInfo(const std::string& companyId);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onMenuItemActivated(QListWidgetItem* item);

private:
    void setupUi();
    void populateMenu();

    application::NavigationManager& m_navManager;
    services::CompanyService& m_companyService;

    QLabel* m_companyNameLabel;
    QLabel* m_periodLabel;
    QLabel* m_lastDateLabel;
    QListWidget* m_menuList;
};

} // namespace tally::ui
