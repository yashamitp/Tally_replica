#include "VoucherService.hpp"

namespace tally::services {

VoucherService::VoucherService(database::DatabaseManager& dbManager)
    : m_engine(dbManager) {}

domain::Voucher VoucherService::postVoucher(domain::Voucher voucher) {
    return m_engine.postVoucher(std::move(voucher));
}

domain::Voucher VoucherService::reverseVoucher(const domain::ReverseVoucherParams& params) {
    return m_engine.reverseVoucher(params);
}

bool VoucherService::cancelVoucher(const std::string& companyId, const std::string& voucherId, const std::string& reason) {
    return m_engine.cancelVoucher(companyId, voucherId, reason);
}

std::optional<domain::Voucher> VoucherService::getVoucher(const std::string& voucherId) {
    return m_engine.getVoucherWithEntries(voucherId);
}

std::string VoucherService::getNextVoucherNumber(const std::string& companyId, const std::string& voucherTypeId) {
    return m_engine.getNextVoucherNumber(companyId, voucherTypeId);
}

} // namespace tally::services
