#pragma once

#include <string>
#include <vector>
#include <optional>
#include "../domain/ledger/Ledger.hpp"
#include "../domain/decimal/ExactDecimal.hpp"
#include "../domain/groups/AccountingGroup.hpp"
#include "../database/DatabaseManager.hpp"

namespace tally::services {

struct CreateLedgerParams {
    std::string companyId;
    std::string groupId;
    std::string name;
    std::optional<std::string> alias;
    domain::ExactDecimal openingBalance = domain::ExactDecimal::zero();
    domain::BalanceType openingBalanceType = domain::BalanceType::Dr;
    std::optional<std::string> gstin;
    std::optional<std::string> pan;
    std::optional<std::string> address;
    std::optional<std::string> state;
    std::optional<std::string> pincode;
    bool isSystem = false;
    std::string unmappedFields = "{}";
};

struct LedgerStatementLine {
    std::string date;
    std::string voucherId;
    std::string voucherNumber;
    std::string voucherTypeName;
    std::string particulars;
    std::optional<std::string> narration;
    std::optional<domain::ExactDecimal> debitAmount;
    std::optional<domain::ExactDecimal> creditAmount;
    domain::ExactDecimal runningBalance = domain::ExactDecimal::zero();
    domain::BalanceType runningBalanceType = domain::BalanceType::Dr;
};

struct LedgerStatement {
    std::string companyId;
    std::string ledgerId;
    std::string ledgerName;
    std::string groupName;
    domain::GroupNature groupNature = domain::GroupNature::Asset;
    std::string fromDate;
    std::string toDate;
    domain::ExactDecimal openingBalance = domain::ExactDecimal::zero();
    domain::BalanceType openingBalanceType = domain::BalanceType::Dr;
    domain::ExactDecimal totalDebits = domain::ExactDecimal::zero();
    domain::ExactDecimal totalCredits = domain::ExactDecimal::zero();
    domain::ExactDecimal closingBalance = domain::ExactDecimal::zero();
    domain::BalanceType closingBalanceType = domain::BalanceType::Dr;
    std::vector<LedgerStatementLine> transactions;
};

class LedgerService {
public:
    explicit LedgerService(database::DatabaseManager& dbManager);

    domain::Ledger createLedger(const CreateLedgerParams& params);
    std::optional<domain::Ledger> getLedger(const std::string& id);
    std::optional<domain::Ledger> getLedgerByName(const std::string& companyId, const std::string& name);
    std::vector<domain::Ledger> listLedgers(const std::string& companyId);
    bool alterLedger(const domain::Ledger& ledger);

    LedgerStatement getLedgerStatement(
        const std::string& companyId,
        const std::string& ledgerId,
        const std::optional<std::string>& fromDate = std::nullopt,
        const std::optional<std::string>& toDate = std::nullopt
    );

private:
    database::DatabaseManager& m_dbManager;
};

} // namespace tally::services
