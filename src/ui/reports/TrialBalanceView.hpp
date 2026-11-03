#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QLabel>
#include <QLineEdit>
#include "../../application/NavigationManager.hpp"
#include "../../services/ReportService.hpp"

namespace tally::ui {

class TrialBalanceView : public QWidget {
    Q_OBJECT

public:
    TrialBalanceView(
        application::NavigationManager& navManager,
        services::ReportService& reportService,
        QWidget* parent = nullptr
    );

    void setCompanyId(const std::string& companyId);
    void refreshData();

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    void setupUi();
    void populateNode(QTreeWidgetItem* parentItem, const services::TrialBalanceGroupNode& node);

    application::NavigationManager& m_navManager;
    services::ReportService& m_reportService;
    std::string m_companyId;

    QLabel* m_companyHeaderLabel;
    QLineEdit* m_fromDateEdit;
    QLineEdit* m_toDateEdit;
    QTreeWidget* m_tree;
    QLabel* m_statusLabel;
    QLabel* m_totOpenDrLabel;
    QLabel* m_totOpenCrLabel;
    QLabel* m_totPerDrLabel;
    QLabel* m_totPerCrLabel;
    QLabel* m_totCloseDrLabel;
    QLabel* m_totCloseCrLabel;
};

} // namespace tally::ui
