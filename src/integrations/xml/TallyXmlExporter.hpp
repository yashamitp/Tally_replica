#pragma once

#include <string>
#include <optional>
#include <vector>
#include "../../database/DatabaseManager.hpp"

namespace tally::integrations::xml {

struct ExportVouchersFilter {
    std::string companyId;
    std::optional<std::string> fromDate;
    std::optional<std::string> toDate;
    std::optional<std::string> voucherTypeId;
};

class TallyXmlExporter {
public:
    explicit TallyXmlExporter(database::DatabaseManager& dbManager);

    std::string exportMasters(const std::string& companyId, int* outRecordCount = nullptr);
    std::string exportVouchers(const ExportVouchersFilter& filter, int* outRecordCount = nullptr);

private:
    database::DatabaseManager& m_dbManager;
};

} // namespace tally::integrations::xml
