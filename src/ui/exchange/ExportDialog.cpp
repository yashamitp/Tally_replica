#include "ExportDialog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFileDialog>
#include <QFile>
#include <QMessageBox>
#include "../../integrations/xml/TallyXmlExporter.hpp"
#include "../../integrations/json/JsonExchange.hpp"

namespace tally::ui {

ExportDialog::ExportDialog(
    const std::string& companyId,
    database::DatabaseManager& dbManager,
    QWidget* parent
) : QDialog(parent),
    m_companyId(companyId),
    m_dbManager(dbManager) {
    setWindowTitle("Export Data (Alt+E)");
    setFixedWidth(500);
    setupUi();
}

void ExportDialog::setupUi() {
    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(12);

    QLabel* header = new QLabel("Export Accounting Data", this);
    header->setStyleSheet("font-size: 11pt; font-weight: bold; color: #1B4977;");
    rootLayout->addWidget(header);

    QFormLayout* form = new QFormLayout();
    form->setSpacing(8);

    m_formatCombo = new QComboBox(this);
    m_formatCombo->addItem("Tally XML Envelope (*.xml)", "XML");
    m_formatCombo->addItem("Canonical JSON Document (*.json)", "JSON");
    form->addRow("Export Format:", m_formatCombo);

    m_scopeCombo = new QComboBox(this);
    m_scopeCombo->addItem("Masters Only (Groups & Ledgers)", "MASTERS");
    m_scopeCombo->addItem("Vouchers Only", "VOUCHERS");
    m_scopeCombo->addItem("Complete Snapshot (Masters & Vouchers)", "ALL");
    form->addRow("Data Scope:", m_scopeCombo);

    QHBoxLayout* fileLayout = new QHBoxLayout();
    m_targetPathEdit = new QLineEdit("tally_export.xml", this);
    fileLayout->addWidget(m_targetPathEdit);

    QPushButton* browseBtn = new QPushButton("Browse...", this);
    connect(browseBtn, &QPushButton::clicked, this, &ExportDialog::onBrowseFile);
    fileLayout->addWidget(browseBtn);
    form->addRow("Destination File *:", fileLayout);

    rootLayout->addLayout(form);

    connect(m_formatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int idx) {
        QString current = m_targetPathEdit->text();
        if (idx == 0 && current.endsWith(".json")) {
            m_targetPathEdit->setText(current.chopped(5) + ".xml");
        } else if (idx == 1 && current.endsWith(".xml")) {
            m_targetPathEdit->setText(current.chopped(4) + ".json");
        }
    });

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    QPushButton* cancelBtn = new QPushButton("Cancel (Esc)", this);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn);

    m_exportBtn = new QPushButton("Export (Enter)", this);
    m_exportBtn->setStyleSheet("background-color: #F6C343; font-weight: bold; padding: 6px 16px; border: 1px solid #1B4977;");
    connect(m_exportBtn, &QPushButton::clicked, this, &ExportDialog::onExecuteExport);
    btnLayout->addWidget(m_exportBtn);

    rootLayout->addLayout(btnLayout);
}

void ExportDialog::onBrowseFile() {
    bool isXml = m_formatCombo->currentData().toString() == "XML";
    QString filter = isXml ? "XML Files (*.xml);;All Files (*)" : "JSON Files (*.json);;All Files (*)";
    QString defaultName = isXml ? "tally_export.xml" : "tally_export.json";
    QString path = QFileDialog::getSaveFileName(this, "Select Export Target", defaultName, filter);
    if (!path.isEmpty()) {
        m_targetPathEdit->setText(path);
    }
}

void ExportDialog::onExecuteExport() {
    QString targetPath = m_targetPathEdit->text().trimmed();
    if (targetPath.isEmpty()) {
        QMessageBox::warning(this, "Validation", "Destination file path is required.");
        return;
    }

    std::string format = m_formatCombo->currentData().toString().toStdString();
    std::string scope = m_scopeCombo->currentData().toString().toStdString();

    try {
        QFile file(targetPath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::critical(this, "File Error", "Could not open target file for writing: " + file.errorString());
            return;
        }

        if (format == "XML") {
            integrations::xml::TallyXmlExporter exporter(m_dbManager);
            std::string content;
            int count = 0;

            if (scope == "MASTERS") {
                content = exporter.exportMasters(m_companyId, &count);
            } else if (scope == "VOUCHERS") {
                integrations::xml::ExportVouchersFilter filter;
                filter.companyId = m_companyId;
                content = exporter.exportVouchers(filter, &count);
            } else {
                int mCount = 0;
                int vCount = 0;
                std::string mXml = exporter.exportMasters(m_companyId, &mCount);
                integrations::xml::ExportVouchersFilter filter;
                filter.companyId = m_companyId;
                std::string vXml = exporter.exportVouchers(filter, &vCount);
                content = mXml + "\n" + vXml;
                count = mCount + vCount;
            }

            file.write(content.c_str(), content.size());
            file.close();

            QMessageBox::information(this, "Export Success",
                QString("Successfully exported %1 records to:\n%2").arg(count).arg(targetPath));
            accept();
        } else {
            // JSON export
            integrations::json::JsonExchange jsonExchange(m_dbManager);
            QJsonObject root;

            if (scope == "MASTERS" || scope == "ALL") {
                QJsonObject mObj = jsonExchange.exportMastersJson(m_companyId);
                if (mObj.contains("groups")) root["groups"] = mObj["groups"];
                if (mObj.contains("ledgers")) root["ledgers"] = mObj["ledgers"];
            }

            if (scope == "VOUCHERS" || scope == "ALL") {
                QJsonObject vObj = jsonExchange.exportVouchersJson(m_companyId);
                if (vObj.contains("vouchers")) root["vouchers"] = vObj["vouchers"];
            }

            QJsonDocument doc(root);
            file.write(doc.toJson(QJsonDocument::Indented));
            file.close();

            QMessageBox::information(this, "Export Success",
                QString("Successfully exported JSON snapshot to:\n%1").arg(targetPath));
            accept();
        }
    } catch (const std::exception& ex) {
        QMessageBox::critical(this, "Export Error", QString::fromStdString(ex.what()));
    }
}

} // namespace tally::ui
