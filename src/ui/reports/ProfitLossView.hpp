#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QLabel>
#include <QLineEdit>
#include "../../application/NavigationManager.hpp"
#include "../../services/ReportService.hpp"

namespace tally::ui {

class ProfitLossView : public QWidget {
    Q_OBJECT

public:
    ProfitLossView(
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

    application::NavigationManager& m_navManager;
    services::ReportService& m_reportService;
    std::string m_companyId;

    QLabel* m_titleLabel;
    QLineEdit* m_fromDateEdit;
    QLineEdit* m_toDateEdit;
    QTreeWidget* m_tree;
    QLabel* m_grossProfitLabel;
    QLabel* m_netProfitLabel;
};

} // namespace tally::ui
