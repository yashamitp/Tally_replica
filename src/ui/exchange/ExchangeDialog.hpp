#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <string>
#include "../../integrations/tally/TallyHttpClient.hpp"
#include "../../database/DatabaseManager.hpp"

namespace tally::ui {

class ExchangeDialog : public QDialog {
    Q_OBJECT

public:
    explicit ExchangeDialog(
        const std::string& companyId,
        database::DatabaseManager& dbManager,
        QWidget* parent = nullptr
    );

private slots:
    void onTestConnection();
    void onExportToTally();

private:
    void setupUi();

    std::string m_companyId;
    database::DatabaseManager& m_dbManager;
    integrations::tally::TallyHttpClient m_client;

    QLineEdit* m_hostEdit = nullptr;
    QLineEdit* m_portEdit = nullptr;
    QLabel* m_statusLabel = nullptr;
    QTextEdit* m_responseEdit = nullptr;
    QPushButton* m_testBtn = nullptr;
    QPushButton* m_pushBtn = nullptr;
};

} // namespace tally::ui
