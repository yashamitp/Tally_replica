#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QComboBox>
#include <string>
#include "../../database/DatabaseManager.hpp"

namespace tally::ui {

class ImportDialog : public QDialog {
    Q_OBJECT

public:
    explicit ImportDialog(
        const std::string& companyId,
        database::DatabaseManager& dbManager,
        QWidget* parent = nullptr
    );

    bool dataImported() const { return m_imported; }

private slots:
    void onBrowseFile();
    void onPreviewClicked();
    void onImportClicked();

private:
    void setupUi();

    std::string m_companyId;
    database::DatabaseManager& m_dbManager;
    bool m_imported = false;

    QLineEdit* m_filePathEdit = nullptr;
    QComboBox* m_formatCombo = nullptr;
    QTextEdit* m_logTextEdit = nullptr;
    QPushButton* m_previewBtn = nullptr;
    QPushButton* m_importBtn = nullptr;
};

} // namespace tally::ui
