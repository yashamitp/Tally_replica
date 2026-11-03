#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QLabel>
#include <QLineEdit>
#include "../../application/NavigationManager.hpp"
#include "../../services/ReportService.hpp"

namespace tally::ui {

class BalanceSheetView : public QWidget {
    Q_OBJECT

public:
    BalanceSheetView(
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
    QLineEdit* m_asOfDateEdit;
    QTreeWidget* m_liabilitiesTree;
    QTreeWidget* m_assetsTree;
    QLabel* m_totLiabLabel;
    QLabel* m_totAssetLabel;
    QLabel* m_reconciledLabel;
};

} // namespace tally::ui
