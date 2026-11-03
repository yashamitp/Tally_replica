#pragma once

#include <string>

namespace tally::domain {

struct AuditEvent {
    std::string id;
    std::string companyId;
    std::string entityType; // COMPANY, GROUP, LEDGER, VOUCHER
    std::string entityId;
    std::string action;     // CREATE, UPDATE, POST, CANCEL, REVERSE, IMPORT
    std::string performedBy = "SYSTEM";
    std::string changesJson = "{}";
    std::string performedAt;
};

} // namespace tally::domain
