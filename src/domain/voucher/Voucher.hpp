#pragma once

#include <string>
#include <vector>
#include <optional>
#include "../decimal/ExactDecimal.hpp"
#include "../ledger/Ledger.hpp"

namespace tally::domain {

enum class VoucherCategory {
    Journal,
    Payment,
    Receipt,
    Contra,
    Sales,
    Purchase,
    CreditNote,
    DebitNote
};

inline std::string voucherCategoryToString(VoucherCategory cat) {
    switch (cat) {
        case VoucherCategory::Journal: return "JOURNAL";
        case VoucherCategory::Payment: return "PAYMENT";
        case VoucherCategory::Receipt: return "RECEIPT";
        case VoucherCategory::Contra: return "CONTRA";
        case VoucherCategory::Sales: return "SALES";
        case VoucherCategory::Purchase: return "PURCHASE";
        case VoucherCategory::CreditNote: return "CREDIT_NOTE";
        case VoucherCategory::DebitNote: return "DEBIT_NOTE";
    }
    return "JOURNAL";
}

inline VoucherCategory voucherCategoryFromString(const std::string& str) {
    if (str == "JOURNAL") return VoucherCategory::Journal;
    if (str == "PAYMENT") return VoucherCategory::Payment;
    if (str == "RECEIPT") return VoucherCategory::Receipt;
    if (str == "CONTRA") return VoucherCategory::Contra;
    if (str == "SALES") return VoucherCategory::Sales;
    if (str == "PURCHASE") return VoucherCategory::Purchase;
    if (str == "CREDIT_NOTE") return VoucherCategory::CreditNote;
    if (str == "DEBIT_NOTE") return VoucherCategory::DebitNote;
    return VoucherCategory::Journal;
}

enum class VoucherStatus {
    Draft,
    Validated,
    Posted,
    Cancelled,
    Reversed
};

inline std::string voucherStatusToString(VoucherStatus status) {
    switch (status) {
        case VoucherStatus::Draft: return "DRAFT";
        case VoucherStatus::Validated: return "VALIDATED";
        case VoucherStatus::Posted: return "POSTED";
        case VoucherStatus::Cancelled: return "CANCELLED";
        case VoucherStatus::Reversed: return "REVERSED";
    }
    return "POSTED";
}

inline VoucherStatus voucherStatusFromString(const std::string& str) {
    if (str == "DRAFT") return VoucherStatus::Draft;
    if (str == "VALIDATED") return VoucherStatus::Validated;
    if (str == "POSTED") return VoucherStatus::Posted;
    if (str == "CANCELLED") return VoucherStatus::Cancelled;
    if (str == "REVERSED") return VoucherStatus::Reversed;
    return VoucherStatus::Posted;
}

struct VoucherType {
    std::string id;
    std::string companyId;
    std::string name;
    VoucherCategory category = VoucherCategory::Journal;
    std::string numberingMethod = "AUTOMATIC";
    std::optional<std::string> prefix;
    std::optional<std::string> suffix;
    bool isSystem = false;
    std::string createdAt;
};

struct VoucherEntry {
    std::string id;
    std::string voucherId;
    std::string ledgerId;
    BalanceType entryType = BalanceType::Dr;
    ExactDecimal amount = ExactDecimal::zero();
    std::optional<std::string> narration;
    int lineIndex = 1;
    std::string createdAt;
};

struct Voucher {
    std::string id;
    std::string companyId;
    std::string voucherTypeId;
    std::string voucherNumber;
    std::string date; // ISO YYYY-MM-DD
    std::optional<std::string> referenceNumber;
    std::optional<std::string> referenceDate;
    std::optional<std::string> narration;
    ExactDecimal totalAmount = ExactDecimal::zero();
    VoucherStatus status = VoucherStatus::Posted;
    bool isCancelled = false;
    bool isReversed = false;
    std::optional<std::string> reversalVoucherId;
    std::string unmappedFields = "{}";
    std::string createdAt;
    std::string updatedAt;

    std::vector<VoucherEntry> entries;
};

} // namespace tally::domain
