#pragma once

#include <string>
#include <optional>
#include "../decimal/ExactDecimal.hpp"

namespace tally::domain {

enum class BalanceType {
    Dr,
    Cr
};

inline std::string balanceTypeToString(BalanceType type) {
    return type == BalanceType::Dr ? "DR" : "CR";
}

inline BalanceType balanceTypeFromString(const std::string& str) {
    return (str == "CR" || str == "cr") ? BalanceType::Cr : BalanceType::Dr;
}

struct Ledger {
    std::string id;
    std::string companyId;
    std::string groupId;
    std::string name;
    std::optional<std::string> alias;
    ExactDecimal openingBalance = ExactDecimal::zero();
    BalanceType openingBalanceType = BalanceType::Dr;
    ExactDecimal currentBalance = ExactDecimal::zero();
    bool isSystem = false;
    bool isActive = true;
    std::optional<std::string> gstin;
    std::optional<std::string> pan;
    std::optional<std::string> address;
    std::optional<std::string> state;
    std::optional<std::string> pincode;
    std::string unmappedFields = "{}";
    std::string createdAt;
    std::string updatedAt;
};

} // namespace tally::domain
