#pragma once

#include <string>
#include <optional>
#include "../voucher/Voucher.hpp"
#include "../groups/AccountingGroup.hpp"
#include "../../database/DatabaseManager.hpp"
#include "DoubleEntryValidator.hpp"

namespace tally::domain {

struct ReverseVoucherParams {
    std::string companyId;
    std::string voucherId;
    std::string reversalDate; // ISO YYYY-MM-DD
    std::string reason;
};

class AccountingEngine {
public:
    explicit AccountingEngine(database::DatabaseManager& dbManager);

    Voucher postVoucher(Voucher voucher);
    Voucher reverseVoucher(const ReverseVoucherParams& params);
    bool cancelVoucher(const std::string& companyId, const std::string& voucherId, const std::string& reason);
    std::optional<Voucher> getVoucherWithEntries(const std::string& voucherId);
    std::string getNextVoucherNumber(const std::string& companyId, const std::string& voucherTypeId);

private:
    database::DatabaseManager& m_dbManager;
};

} // namespace tally::domain
