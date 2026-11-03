#pragma once

#include <string>
#include <optional>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include "../../database/DatabaseManager.hpp"
#include "../../domain/accounting/AccountingEngine.hpp"
#include "../xml/TallyXmlDataTypes.hpp"

namespace tally::integrations::json {

class JsonExchange {
public:
    explicit JsonExchange(database::DatabaseManager& dbManager);

    QJsonObject exportMastersJson(const std::string& companyId);
    QJsonObject exportVouchersJson(const std::string& companyId,
                                   const std::optional<std::string>& fromDate = std::nullopt,
                                   const std::optional<std::string>& toDate = std::nullopt);

    xml::ImportExecutionResult importMastersJson(const std::string& companyId, const QJsonObject& rootObj);
    xml::ImportExecutionResult importVouchersJson(const std::string& companyId, const QJsonObject& rootObj);

private:
    database::DatabaseManager& m_dbManager;
    domain::AccountingEngine m_accountingEngine;
};

} // namespace tally::integrations::json
