#include "ImportDialog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QFile>
#include <QMessageBox>
#include "../../integrations/xml/TallyXmlImporter.hpp"
#include "../../integrations/json/JsonExchange.hpp"

namespace tally::ui {

ImportDialog::ImportDialog(
    const std::string& companyId,
    database::DatabaseManager& dbManager,
    QWidget* parent
) : QDialog(parent),
    m_companyId(companyId),
    m_dbManager(dbManager) {
    setWindowTitle("Import Data (Alt+O)");
    setMinimumSize(580, 420);
    setupUi();
}

void ImportDialog::setupUi() {
    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(12);

    QLabel* header = new QLabel("Import Tally XML / Canonical JSON Data", this);
    header->setStyleSheet("font-size: 11pt; font-weight: bold; color: #1B4977;");
    rootLayout->addWidget(header);

    // Format and File selection
    QHBoxLayout* fileLayout = new QHBoxLayout();

    m_formatCombo = new QComboBox(this);
    m_formatCombo->addItem("Tally XML (*.xml)", "XML");
    m_formatCombo->addItem("Canonical JSON (*.json)", "JSON");
    fileLayout->addWidget(m_formatCombo);

    m_filePathEdit = new QLineEdit(this);
    m_filePathEdit->setPlaceholderText("Select file to import...");
    fileLayout->addWidget(m_filePathEdit, 1);

    QPushButton* browseBtn = new QPushButton("Browse...", this);
    connect(browseBtn, &QPushButton::clicked, this, &ImportDialog::onBrowseFile);
    fileLayout->addWidget(browseBtn);

    rootLayout->addLayout(fileLayout);

    // Preview / Execution buttons
    QHBoxLayout* actionLayout = new QHBoxLayout();

    m_previewBtn = new QPushButton("Preview Import", this);
    m_previewBtn->setStyleSheet("background-color: #2A6592; color: white; font-weight: bold; padding: 6px 14px;");
    connect(m_previewBtn, &QPushButton::clicked, this, &ImportDialog::onPreviewClicked);
    actionLayout->addWidget(m_previewBtn);

    m_importBtn = new QPushButton("Execute Import", this);
    m_importBtn->setStyleSheet("background-color: #F6C343; font-weight: bold; padding: 6px 14px; border: 1px solid #1B4977;");
    connect(m_importBtn, &QPushButton::clicked, this, &ImportDialog::onImportClicked);
    actionLayout->addWidget(m_importBtn);

    actionLayout->addStretch();
    rootLayout->addLayout(actionLayout);

    // Log Output Area
    QLabel* logLabel = new QLabel("Import Analysis & Audit Log:", this);
    logLabel->setStyleSheet("font-weight: bold;");
    rootLayout->addWidget(logLabel);

    m_logTextEdit = new QTextEdit(this);
    m_logTextEdit->setReadOnly(true);
    m_logTextEdit->setStyleSheet("background-color: #F9FBFD; font-family: monospace; font-size: 9pt; border: 1px solid #B8D0E5;");
    rootLayout->addWidget(m_logTextEdit);

    // Close button
    QHBoxLayout* closeLayout = new QHBoxLayout();
    closeLayout->addStretch();
    QPushButton* closeBtn = new QPushButton("Close (Esc)", this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    closeLayout->addWidget(closeBtn);
    rootLayout->addLayout(closeLayout);
}

void ImportDialog::onBrowseFile() {
    QString filter = m_formatCombo->currentData().toString() == "XML" ? "XML Files (*.xml);;All Files (*)" : "JSON Files (*.json);;All Files (*)";
    QString path = QFileDialog::getOpenFileName(this, "Select File to Import", "", filter);
    if (!path.isEmpty()) {
        m_filePathEdit->setText(path);
        if (path.endsWith(".json", Qt::CaseInsensitive)) {
            m_formatCombo->setCurrentIndex(1);
        } else if (path.endsWith(".xml", Qt::CaseInsensitive)) {
            m_formatCombo->setCurrentIndex(0);
        }
    }
}

void ImportDialog::onPreviewClicked() {
    QString path = m_filePathEdit->text().trimmed();
    if (path.isEmpty()) {
        QMessageBox::warning(this, "Validation", "Please select a file to import.");
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "File Error", "Could not open file for reading: " + file.errorString());
        return;
    }

    QString content = QString::fromUtf8(file.readAll());
    file.close();

    m_logTextEdit->clear();
    m_logTextEdit->append("=== PRE-IMPORT VALIDATION PREVIEW ===");
    m_logTextEdit->append(QString("File: %1 (%2 bytes)").arg(path).arg(content.size()));

    if (m_formatCombo->currentData().toString() == "XML") {
        try {
            integrations::xml::TallyXmlImporter importer(m_dbManager);
            auto previewResult = importer.preview(m_companyId, content.toStdString());

            m_logTextEdit->append(QString("Status: %1").arg(previewResult.canCommit ? "VALID (Can Commit) ✓" : "WARNINGS/ERRORS ✗"));
            m_logTextEdit->append(QString("Total Masters Detected: %1").arg(previewResult.groupsCount + previewResult.ledgersCount));
            m_logTextEdit->append(QString("  - Accounting Groups: %1").arg(previewResult.groupsCount));
            m_logTextEdit->append(QString("  - Ledgers: %1").arg(previewResult.ledgersCount));
            m_logTextEdit->append(QString("Total Vouchers Detected: %1").arg(previewResult.vouchersCount));
            m_logTextEdit->append(QString("Items to Process: %1 (Accepted: %2, Skipped: %3, Rejected: %4)")
                .arg(previewResult.items.size())
                .arg(previewResult.acceptedCount)
                .arg(previewResult.skippedCount)
                .arg(previewResult.rejectedCount));

            for (const auto& item : previewResult.items) {
                if (item.action != integrations::xml::PreviewAction::Create) {
                    m_logTextEdit->append(QString("  [%1] %2: %3").arg(QString::fromStdString(item.type), QString::fromStdString(item.identifier), QString::fromStdString(item.reason)));
                }
            }

            for (const auto& w : previewResult.warnings) {
                m_logTextEdit->append(QString("  [Warning] %1").arg(QString::fromStdString(w)));
            }

            for (const auto& e : previewResult.errors) {
                m_logTextEdit->append(QString("  [Error] %1").arg(QString::fromStdString(e)));
            }

            if (previewResult.canCommit) {
                m_logTextEdit->append("\nPreview completed successfully. Ready to execute import.");
            }
        } catch (const std::exception& ex) {
            m_logTextEdit->append(QString("Exception during preview: %1").arg(ex.what()));
        }
    } else {
        // JSON preview
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(content.toUtf8(), &err);
        if (err.error != QJsonParseError::NoError) {
            m_logTextEdit->append(QString("JSON Parse Error: %1").arg(err.errorString()));
            return;
        }

        QJsonObject root = doc.object();
        m_logTextEdit->append("Valid JSON document structure detected.");
        if (root.contains("groups")) m_logTextEdit->append(QString("Groups found: %1").arg(root["groups"].toArray().size()));
        if (root.contains("ledgers")) m_logTextEdit->append(QString("Ledgers found: %1").arg(root["ledgers"].toArray().size()));
        if (root.contains("vouchers")) m_logTextEdit->append(QString("Vouchers found: %1").arg(root["vouchers"].toArray().size()));
        m_logTextEdit->append("\nReady to execute JSON import.");
    }
}

void ImportDialog::onImportClicked() {
    QString path = m_filePathEdit->text().trimmed();
    if (path.isEmpty()) {
        QMessageBox::warning(this, "Validation", "Please select a file to import.");
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "File Error", "Could not open file for reading: " + file.errorString());
        return;
    }

    QString content = QString::fromUtf8(file.readAll());
    file.close();

    m_logTextEdit->append("\n=== EXECUTING TRANSACTIONAL IMPORT ===");

    if (m_formatCombo->currentData().toString() == "XML") {
        try {
            integrations::xml::TallyXmlImporter importer(m_dbManager);
            auto execResult = importer.execute(m_companyId, content.toStdString());

            m_logTextEdit->append(QString("Execution Status: %1").arg(execResult.success ? "SUCCESS ✓" : "FAILED ✗"));
            m_logTextEdit->append(QString("Groups Created: %1").arg(execResult.createdGroups));
            m_logTextEdit->append(QString("Ledgers Created: %1").arg(execResult.createdLedgers));
            m_logTextEdit->append(QString("Vouchers Created: %1").arg(execResult.createdVouchers));
            m_logTextEdit->append(QString("Skipped Count: %1").arg(execResult.skippedCount));
            m_logTextEdit->append(QString("Failed / Rejected: %1").arg(execResult.failedCount));

            for (const auto& err : execResult.errors) {
                m_logTextEdit->append(QString("  [Error] %1").arg(QString::fromStdString(err)));
            }

            if (execResult.success) {
                m_imported = true;
                QMessageBox::information(this, "Import Complete",
                    QString("Import finished successfully!\n%1 groups, %2 ledgers, %3 vouchers imported.")
                        .arg(execResult.createdGroups)
                        .arg(execResult.createdLedgers)
                        .arg(execResult.createdVouchers));
            }
        } catch (const std::exception& ex) {
            m_logTextEdit->append(QString("Exception during import: %1").arg(ex.what()));
            QMessageBox::critical(this, "Import Error", QString::fromStdString(ex.what()));
        }
    } else {
        // JSON Import
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(content.toUtf8(), &err);
        if (err.error != QJsonParseError::NoError) {
            m_logTextEdit->append(QString("JSON Parse Error: %1").arg(err.errorString()));
            return;
        }

        try {
            integrations::json::JsonExchange jsonExchange(m_dbManager);
            QJsonObject root = doc.object();

            int mastersCreated = 0;
            int vouchersCreated = 0;

            if (root.contains("groups") || root.contains("ledgers")) {
                auto mRes = jsonExchange.importMastersJson(m_companyId, root);
                mastersCreated += (mRes.createdGroups + mRes.createdLedgers);
                m_logTextEdit->append(QString("Masters Created: %1 (Groups: %2, Ledgers: %3)")
                    .arg(mastersCreated).arg(mRes.createdGroups).arg(mRes.createdLedgers));
            }

            if (root.contains("vouchers")) {
                auto vRes = jsonExchange.importVouchersJson(m_companyId, root);
                vouchersCreated += vRes.createdVouchers;
                m_logTextEdit->append(QString("Vouchers Created: %1").arg(vRes.createdVouchers));
            }

            m_imported = true;
            m_logTextEdit->append("JSON import completed successfully.");
            QMessageBox::information(this, "Import Complete",
                QString("JSON import finished successfully!\n%1 masters, %2 vouchers imported.")
                    .arg(mastersCreated)
                    .arg(vouchersCreated));
        } catch (const std::exception& ex) {
            m_logTextEdit->append(QString("Exception during JSON import: %1").arg(ex.what()));
            QMessageBox::critical(this, "Import Error", QString::fromStdString(ex.what()));
        }
    }
}

} // namespace tally::ui
