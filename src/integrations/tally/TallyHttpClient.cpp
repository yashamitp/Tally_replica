#include "TallyHttpClient.hpp"
#include <QEventLoop>
#include <QTimer>
#include <QUrl>
#include <QElapsedTimer>
#include <QXmlStreamReader>
#include "../../utilities/Logger.hpp"

namespace tally::integrations::tally {

TallyHttpClient::TallyHttpClient(const std::string& host, int port, QObject* parent)
    : QObject(parent), m_host(host), m_port(port) {}

std::string TallyHttpClient::baseUrl() const {
    return "http://" + m_host + ":" + std::to_string(m_port);
}

void TallyHttpClient::setHostAndPort(const std::string& host, int port) {
    m_host = host;
    m_port = port;
}

std::string TallyHttpClient::postRawXml(const std::string& xmlBody, int timeoutMs, std::string* outError) {
    QUrl url(QString::fromStdString(baseUrl()));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/xml; charset=utf-8");

    QNetworkReply* reply = m_networkManager.post(request, QByteArray::fromStdString(xmlBody));

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);

    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

    timer.start(timeoutMs);
    loop.exec();

    if (timer.isActive()) {
        timer.stop();
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            reply->deleteLater();
            return responseData.toStdString();
        } else {
            if (outError) *outError = reply->errorString().toStdString();
            reply->deleteLater();
            return "";
        }
    } else {
        // Timed out
        reply->abort();
        reply->deleteLater();
        if (outError) *outError = "Request timed out after " + std::to_string(timeoutMs) + "ms";
        return "";
    }
}

TallyConnectionStatus TallyHttpClient::testConnection(int timeoutMs) {
    TallyConnectionStatus status;
    status.url = baseUrl();

    std::string pingXml =
        "<ENVELOPE>"
        "<HEADER><VERSION>1</VERSION><TALLYREQUEST>Export</TALLYREQUEST>"
        "<TYPE>Collection</TYPE><ID>CompanyCollection</ID></HEADER>"
        "<BODY><DESC><STATICVARIABLES>"
        "<SVEXPORTFORMAT>$$SysName:XML</SVEXPORTFORMAT>"
        "</STATICVARIABLES><TDL><TDLMESSAGE>"
        "<COLLECTION NAME=\"CompanyCollection\" ISMODIFY=\"No\" ISFIXED=\"No\" ISINITIALIZE=\"No\" ISOPTION=\"No\" ISINTERNAL=\"No\">"
        "<TYPE>Company</TYPE><FETCH>Name</FETCH></COLLECTION>"
        "</TDLMESSAGE></TDL></DESC></BODY></ENVELOPE>";

    QElapsedTimer elapsed;
    elapsed.start();

    std::string err;
    std::string resp = postRawXml(pingXml, timeoutMs, &err);
    status.latencyMs = static_cast<int>(elapsed.elapsed());

    if (!resp.empty()) {
        status.online = true;
        // Parse company names if available
        QXmlStreamReader reader(QString::fromStdString(resp));
        while (!reader.atEnd() && !reader.hasError()) {
            if (reader.readNext() == QXmlStreamReader::StartElement) {
                if (reader.name().toString().compare("COMPANY", Qt::CaseInsensitive) == 0) {
                    if (reader.attributes().hasAttribute("NAME")) {
                        status.openCompanies.push_back(reader.attributes().value("NAME").toString().toStdString());
                    }
                } else if (reader.name().toString().compare("NAME", Qt::CaseInsensitive) == 0) {
                    status.openCompanies.push_back(reader.readElementText().trimmed().toStdString());
                }
            }
        }
        if (!status.openCompanies.empty()) {
            status.activeCompany = status.openCompanies.front();
        }
    } else {
        status.online = false;
        status.errorMessage = "Could not connect to Tally HTTP Gateway at " + baseUrl() + ": " + err;
    }

    return status;
}

} // namespace tally::integrations::tally
