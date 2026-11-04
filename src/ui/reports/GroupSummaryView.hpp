#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <string>
#include "../../application/NavigationManager.hpp"
#include "../../services/ReportService.hpp"
#include "../../services/GroupService.hpp"

namespace tally::ui {

class GroupSummaryView : public QWidget {
    Q_OBJECT

public:
    explicit GroupSummaryView(
        application::NavigationManager& navManager,
        services::ReportService& reportService,
        services::GroupService& groupService,
        QWidget* parent = nullptr
    );

    void setCompanyId(const std::string& companyId);
    void setGroupId(const std::string& groupId);

public slots:
    void refreshData();

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onGroupSelectionChanged(int index);
    void onRowDoubleClicked(int row, int column);

private:
    void setupUi();
    void loadGroupsCombo();

    application::NavigationManager& m_navManager;
    services::ReportService& m_reportService;
    services::GroupService& m_groupService;

    std::string m_companyId;
    std::string m_selectedGroupId;

    QLabel* m_titleLabel = nullptr;
    QComboBox* m_groupCombo = nullptr;
    QLineEdit* m_fromDateEdit = nullptr;
    QLineEdit* m_toDateEdit = nullptr;
    QTableWidget* m_table = nullptr;

    QLabel* m_totOpenDrLabel = nullptr;
    QLabel* m_totOpenCrLabel = nullptr;
    QLabel* m_totPeriodDrLabel = nullptr;
    QLabel* m_totPeriodCrLabel = nullptr;
    QLabel* m_totCloseDrLabel = nullptr;
    QLabel* m_totCloseCrLabel = nullptr;
};

} // namespace tally::ui
