#pragma once

#include <string>
#include <vector>
#include <optional>
#include "../domain/company/Company.hpp"
#include "../domain/groups/AccountingGroup.hpp"
#include "../domain/voucher/Voucher.hpp"
#include "../database/DatabaseManager.hpp"

namespace tally::services {

struct CreateCompanyParams {
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
    std::string financialYearStart = "2026-04-01";
    std::string booksBeginningFrom = "2026-04-01";
};

class CompanyService {
public:
    explicit CompanyService(database::DatabaseManager& dbManager);

    domain::Company createCompany(const CreateCompanyParams& params);
    std::vector<domain::Company> listCompanies();
    std::optional<domain::Company> getCompany(const std::string& id);
    bool alterCompany(const domain::Company& company);
    bool deleteCompany(const std::string& id);
    std::vector<domain::AccountingGroup> listGroups(const std::string& companyId);
    std::vector<domain::VoucherType> listVoucherTypes(const std::string& companyId);

private:
    database::DatabaseManager& m_dbManager;
};

} // namespace tally::services
