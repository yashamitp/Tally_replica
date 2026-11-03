#pragma once

#include <string>
#include <vector>
#include <map>
#include "../../domain/groups/AccountingGroup.hpp"
#include "../../domain/ledger/Ledger.hpp"
#include "../../domain/voucher/Voucher.hpp"

namespace tally::integrations::xml {

class TallyXmlWriter {
public:
    static std::string groupToXml(const domain::AccountingGroup& group, const std::string& parentName = "");
    static std::string ledgerToXml(const domain::Ledger& ledger, const std::string& parentGroupName = "Current Assets");
    static std::string voucherToXml(const domain::Voucher& voucher, const std::string& voucherTypeName, const std::map<std::string, std::string>& ledgerIdToName);

    static std::string wrapEnvelope(const std::string& requestType,
                                    const std::string& reportId,
                                    const std::string& companyName,
                                    const std::string& innerDataXml);
};

} // namespace tally::integrations::xml
