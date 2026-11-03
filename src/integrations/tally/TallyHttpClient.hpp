#pragma once

#include <string>
#include <vector>
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace tally::integrations::tally {

struct TallyConnectionStatus {
    bool online = false;
    std::string url;
    int latencyMs = 0;
    std::string activeCompany;
    std::vector<std::string> openCompanies;
    std::string errorMessage;
};

class TallyHttpClient : public QObject {
    Q_OBJECT

public:
    explicit TallyHttpClient(const std::string& host = "127.0.0.1", int port = 9000, QObject* parent = nullptr);

    std::string baseUrl() const;
    void setHostAndPort(const std::string& host, int port);

    TallyConnectionStatus testConnection(int timeoutMs = 3000);
    std::string postRawXml(const std::string& xmlBody, int timeoutMs = 10000, std::string* outError = nullptr);

private:
    std::string m_host;
    int m_port;
    QNetworkAccessManager m_networkManager;
};

} // namespace tally::integrations::tally
