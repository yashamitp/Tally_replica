#pragma once

#include <string>
#include <vector>
#include "TallyXmlDataTypes.hpp"
#include "../../database/DatabaseManager.hpp"
#include "../../domain/accounting/AccountingEngine.hpp"

namespace tally::integrations::xml {

class TallyXmlImporter {
public:
    explicit TallyXmlImporter(database::DatabaseManager& dbManager);

    ImportPreviewResult preview(const std::string& companyId, const std::string& xmlContent);
    ImportExecutionResult execute(const std::string& companyId, const std::string& xmlContent);

private:
    database::DatabaseManager& m_dbManager;
    domain::AccountingEngine m_accountingEngine;
};

} // namespace tally::integrations::xml
