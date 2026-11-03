#pragma once

#include <string>
#include <vector>
#include <optional>
#include "../domain/voucher/Voucher.hpp"
#include "../domain/decimal/ExactDecimal.hpp"
#include "../database/DatabaseManager.hpp"

namespace tally::services {

struct DayBookFilter {
    std::string companyId;
    std::optional<std::string> fromDate;
    std::optional<std::string> toDate;
    std::optional<std::string> voucherTypeId;
    std::optional<domain::VoucherCategory> category;
    std::optional<std::string> ledgerId;
    std::optional<domain::VoucherStatus> status;
    int limit = 100;
    int offset = 0;
};

struct DayBookItem {
    std::string id;
    std::string date;
    std::string voucherNumber;
    std::string voucherTypeName;
    domain::VoucherCategory voucherCategory = domain::VoucherCategory::Journal;
    std::string particulars;
    std::optional<std::string> narration;
    domain::ExactDecimal totalAmount = domain::ExactDecimal::zero();
    domain::VoucherStatus status = domain::VoucherStatus::Posted;

    struct EntryLine {
        std::string ledgerId;
        std::string ledgerName;
        domain::BalanceType entryType;
        domain::ExactDecimal amount;
        std::optional<std::string> narration;
    };
    std::vector<EntryLine> entries;
};

struct DayBookResult {
    int totalCount = 0;
    std::vector<DayBookItem> items;
    domain::ExactDecimal totalDebitSum = domain::ExactDecimal::zero();
    domain::ExactDecimal totalCreditSum = domain::ExactDecimal::zero();
};

class DayBookService {
public:
    explicit DayBookService(database::DatabaseManager& dbManager);

    DayBookResult getDayBook(const DayBookFilter& filter);

private:
    database::DatabaseManager& m_dbManager;
};

} // namespace tally::services
