#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include "../../application/NavigationManager.hpp"
#include "../../services/DayBookService.hpp"

namespace tally::ui {

class DayBookView : public QWidget {
    Q_OBJECT

public:
    DayBookView(
        application::NavigationManager& navManager,
        services::DayBookService& dayBookService,
        QWidget* parent = nullptr
    );

    void setCompanyId(const std::string& companyId);
    void refreshData();

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    void setupUi();

    application::NavigationManager& m_navManager;
    services::DayBookService& m_dayBookService;
    std::string m_companyId;

    QLineEdit* m_fromDateEdit;
    QLineEdit* m_toDateEdit;
    QTableWidget* m_table;
    QLabel* m_totalDrLabel;
    QLabel* m_totalCrLabel;
};

} // namespace tally::ui
