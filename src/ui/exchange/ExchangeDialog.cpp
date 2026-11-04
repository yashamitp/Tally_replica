#include "ExchangeDialog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include "../../integrations/xml/TallyXmlExporter.hpp"

namespace tally::ui {

ExchangeDialog::ExchangeDialog(
    const std::string& companyId,
    database::DatabaseManager& dbManager,
    QWidget* parent
) : QDialog(parent),
    m_companyId(companyId),
    m_dbManager(dbManager) {
    setWindowTitle("Tally Gateway Data Exchange (Alt+Z / Port 9000)");
    setMinimumSize(560, 440);
    setupUi();
}

void ExchangeDialog::setupUi() {
    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(12);

    QLabel* header = new QLabel("Live Tally HTTP Gateway Exchange", this);
    header->setStyleSheet("font-size: 11pt; font-weight: bold; color: #1B4977;");
    rootLayout->addWidget(header);

    QFormLayout* form = new QFormLayout();
    form->setSpacing(8);

    m_hostEdit = new QLineEdit("127.0.0.1", this);
    form->addRow("Tally Host IP:", m_hostEdit);

    m_portEdit = new QLineEdit("9000", this);
    form->addRow("Tally Port:", m_portEdit);

    rootLayout->addLayout(form);

    QHBoxLayout* actionLayout = new QHBoxLayout();

    m_testBtn = new QPushButton("Test Connection", this);
    m_testBtn->setStyleSheet("background-color: #2A6592; color: white; font-weight: bold; padding: 6px 14px;");
    connect(m_testBtn, &QPushButton::clicked, this, &ExchangeDialog::onTestConnection);
    actionLayout->addWidget(m_testBtn);

    m_pushBtn = new QPushButton("Push Active Data to Tally", this);
    m_pushBtn->setStyleSheet("background-color: #F6C343; font-weight: bold; padding: 6px 14px; border: 1px solid #1B4977;");
    connect(m_pushBtn, &QPushButton::clicked, this, &ExchangeDialog::onExportToTally);
    actionLayout->addWidget(m_pushBtn);

    actionLayout->addStretch();
    rootLayout->addLayout(actionLayout);

    m_statusLabel = new QLabel("Status: Idle", this);
    m_statusLabel->setStyleSheet("font-weight: bold; color: #555555;");
    rootLayout->addWidget(m_statusLabel);

    QLabel* respTitle = new QLabel("Server Response / Diagnostic Log:", this);
    respTitle->setStyleSheet("font-weight: bold;");
    rootLayout->addWidget(respTitle);

    m_responseEdit = new QTextEdit(this);
    m_responseEdit->setReadOnly(true);
    m_responseEdit->setStyleSheet("background-color: #F9FBFD; font-family: monospace; font-size: 9pt; border: 1px solid #B8D0E5;");
    rootLayout->addWidget(m_responseEdit);

    QHBoxLayout* closeLayout = new QHBoxLayout();
    closeLayout->addStretch();
    QPushButton* closeBtn = new QPushButton("Close (Esc)", this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    closeLayout->addWidget(closeBtn);
    rootLayout->addLayout(closeLayout);
}

void ExchangeDialog::onTestConnection() {
    std::string host = m_hostEdit->text().trimmed().toStdString();
    int port = m_portEdit->text().toInt();
    if (port <= 0) port = 9000;

    m_client.setHostAndPort(host, port);
    m_statusLabel->setText("Pinging Tally at " + QString::fromStdString(m_client.baseUrl()) + "...");
    m_responseEdit->clear();

    auto status = m_client.testConnection(3000);
    if (status.online) {
        m_statusLabel->setText(QString("Status: ONLINE ✓ (%1 ms)").arg(status.latencyMs));
        m_statusLabel->setStyleSheet("font-weight: bold; color: #27AE60;");
        m_responseEdit->append("Successfully connected to Tally instance!");
        m_responseEdit->append(QString("Target URL: %1").arg(QString::fromStdString(status.url)));
        m_responseEdit->append(QString("Active Company: %1").arg(QString::fromStdString(status.activeCompany)));
        for (const auto& c : status.openCompanies) {
            m_responseEdit->append(QString("  - %1").arg(QString::fromStdString(c)));
        }
    } else {
        m_statusLabel->setText("Status: OFFLINE ✗");
        m_statusLabel->setStyleSheet("font-weight: bold; color: #C0392B;");
        m_responseEdit->append("Failed to connect to Tally HTTP Gateway.");
        m_responseEdit->append(QString("Target URL: %1").arg(QString::fromStdString(status.url)));
        m_responseEdit->append(QString("Error: %1").arg(QString::fromStdString(status.errorMessage)));
        m_responseEdit->append("\nPlease check that TallyPrime or Tally.ERP 9 is running and Tally Gateway Server is enabled on port " + QString::number(port) + " (F12 -> Advanced Configuration -> Tally is acting as: Server/Both).");
    }
}

void ExchangeDialog::onExportToTally() {
    std::string host = m_hostEdit->text().trimmed().toStdString();
    int port = m_portEdit->text().toInt();
    if (port <= 0) port = 9000;

    m_client.setHostAndPort(host, port);

    try {
        integrations::xml::TallyXmlExporter exporter(m_dbManager);
        int count = 0;
        std::string mastersXml = exporter.exportMasters(m_companyId, &count);

        m_responseEdit->append(QString("\nSending %1 masters in Tally XML envelope to %2...").arg(count).arg(QString::fromStdString(m_client.baseUrl())));

        std::string errMsg;
        std::string responseXml = m_client.postRawXml(mastersXml, 10000, &errMsg);

        if (!errMsg.empty()) {
            m_responseEdit->append(QString("Error pushing data to Tally: %1").arg(QString::fromStdString(errMsg)));
            QMessageBox::warning(this, "Exchange Notice", QString::fromStdString(errMsg));
        } else {
            m_responseEdit->append("Response received from Tally:\n" + QString::fromStdString(responseXml));
            QMessageBox::information(this, "Success", "Data transmission completed.");
        }
    } catch (const std::exception& ex) {
        m_responseEdit->append(QString("Exception: %1").arg(ex.what()));
        QMessageBox::critical(this, "Error", QString::fromStdString(ex.what()));
    }
}

} // namespace tally::ui
