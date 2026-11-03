#pragma once

#include <string>
#include <optional>

namespace tally::domain {

struct Company {
    std::string id;
    std::string name;
    std::optional<std::string> formalName;
    std::optional<std::string> addressLine1;
    std::optional<std::string> addressLine2;
    std::optional<std::string> city;
    std::optional<std::string> state;
    std::string country = "India";
    std::optional<std::string> pincode;
    std::optional<std::string> gstin;
    std::optional<std::string> pan;
    std::string currencySymbol = "₹";
    std::string currencyFormalName = "INR";
    int decimalPlaces = 2;
    bool isActive = true;
    std::string createdAt;
    std::string updatedAt;
};

struct FinancialYear {
    std::string id;
    std::string companyId;
    std::string startDate;          // ISO YYYY-MM-DD
    std::string endDate;            // ISO YYYY-MM-DD
    std::string booksBeginningFrom; // ISO YYYY-MM-DD
    bool isClosed = false;
    std::string createdAt;
};

} // namespace tally::domain
