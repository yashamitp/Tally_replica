#pragma once

#include <string>
#include <vector>
#include <optional>
#include "../domain/voucher/Voucher.hpp"
#include "../domain/accounting/AccountingEngine.hpp"
#include "../database/DatabaseManager.hpp"

namespace tally::services {

class VoucherService {
public:
    explicit VoucherService(database::DatabaseManager& dbManager);

    domain::Voucher postVoucher(domain::Voucher voucher);
    domain::Voucher reverseVoucher(const domain::ReverseVoucherParams& params);
    bool cancelVoucher(const std::string& companyId, const std::string& voucherId, const std::string& reason);
    std::optional<domain::Voucher> getVoucher(const std::string& voucherId);
    std::string getNextVoucherNumber(const std::string& companyId, const std::string& voucherTypeId);

private:
    domain::AccountingEngine m_engine;
};

} // namespace tally::services
