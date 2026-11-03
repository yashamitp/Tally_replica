#pragma once

#include <string>
#include <optional>
#include <stdexcept>

namespace tally::domain {

enum class GroupNature {
    Asset,
    Liability,
    Equity,
    Income,
    Expense
};

inline std::string groupNatureToString(GroupNature nature) {
    switch (nature) {
        case GroupNature::Asset: return "ASSET";
        case GroupNature::Liability: return "LIABILITY";
        case GroupNature::Equity: return "EQUITY";
        case GroupNature::Income: return "INCOME";
        case GroupNature::Expense: return "EXPENSE";
    }
    return "ASSET";
}

inline GroupNature groupNatureFromString(const std::string& str) {
    if (str == "ASSET") return GroupNature::Asset;
    if (str == "LIABILITY") return GroupNature::Liability;
    if (str == "EQUITY") return GroupNature::Equity;
    if (str == "INCOME") return GroupNature::Income;
    if (str == "EXPENSE") return GroupNature::Expense;
    throw std::invalid_argument("Unknown GroupNature: " + str);
}

struct AccountingGroup {
    std::string id;
    std::string companyId;
    std::optional<std::string> parentId;
    std::string name;
    GroupNature nature = GroupNature::Asset;
    bool affectsGrossProfit = false;
    bool isPrimary = false;
    bool isSystem = false;
    std::string createdAt;
    std::string updatedAt;
};

} // namespace tally::domain
