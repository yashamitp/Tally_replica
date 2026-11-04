#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <string>
#include "../../database/DatabaseManager.hpp"

namespace tally::ui {

class ExportDialog : public QDialog {
    Q_OBJECT

public:
    explicit ExportDialog(
        const std::string& companyId,
        database::DatabaseManager& dbManager,
        QWidget* parent = nullptr
    );

private slots:
    void onBrowseFile();
    void onExecuteExport();

private:
    void setupUi();

    std::string m_companyId;
    database::DatabaseManager& m_dbManager;

    QComboBox* m_formatCombo = nullptr;
    QComboBox* m_scopeCombo = nullptr;
    QLineEdit* m_targetPathEdit = nullptr;
    QPushButton* m_exportBtn = nullptr;
};

} // namespace tally::ui
