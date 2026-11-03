#pragma once

#include <string>
#include <vector>
#include <optional>
#include "../domain/decimal/ExactDecimal.hpp"
#include "../domain/groups/AccountingGroup.hpp"
#include "../domain/ledger/Ledger.hpp"
#include "../database/DatabaseManager.hpp"

namespace tally::services {

struct ReportPeriod {
    std::string startDate;
    std::string endDate;
    std::optional<std::string> financialYearId;
};

// 1. Trial Balance
struct TrialBalanceLedgerRow {
    std::string ledgerId;
    std::string ledgerName;
    std::string groupId;
    std::string groupName;
    domain::GroupNature groupNature = domain::GroupNature::Asset;
    domain::ExactDecimal openingDebit = domain::ExactDecimal::zero();
    domain::ExactDecimal openingCredit = domain::ExactDecimal::zero();
    domain::ExactDecimal periodDebit = domain::ExactDecimal::zero();
    domain::ExactDecimal periodCredit = domain::ExactDecimal::zero();
    domain::ExactDecimal closingDebit = domain::ExactDecimal::zero();
    domain::ExactDecimal closingCredit = domain::ExactDecimal::zero();
    domain::ExactDecimal closingBalance = domain::ExactDecimal::zero();
    domain::BalanceType closingSide = domain::BalanceType::Dr;
};

struct TrialBalanceGroupNode {
    std::string groupId;
    std::string groupName;
    domain::GroupNature groupNature = domain::GroupNature::Asset;
    std::optional<std::string> parentId;
    domain::ExactDecimal openingDebit = domain::ExactDecimal::zero();
    domain::ExactDecimal openingCredit = domain::ExactDecimal::zero();
    domain::ExactDecimal periodDebit = domain::ExactDecimal::zero();
    domain::ExactDecimal periodCredit = domain::ExactDecimal::zero();
    domain::ExactDecimal closingDebit = domain::ExactDecimal::zero();
    domain::ExactDecimal closingCredit = domain::ExactDecimal::zero();
    domain::ExactDecimal netClosingBalance = domain::ExactDecimal::zero();
    domain::BalanceType closingSide = domain::BalanceType::Dr;
    std::vector<TrialBalanceLedgerRow> ledgers;
    std::vector<TrialBalanceGroupNode> subGroups;
};

struct TrialBalanceReport {
    std::string companyName;
    ReportPeriod period;
    std::vector<TrialBalanceGroupNode> groups;
    domain::ExactDecimal totalOpeningDebit = domain::ExactDecimal::zero();
    domain::ExactDecimal totalOpeningCredit = domain::ExactDecimal::zero();
    domain::ExactDecimal totalPeriodDebit = domain::ExactDecimal::zero();
    domain::ExactDecimal totalPeriodCredit = domain::ExactDecimal::zero();
    domain::ExactDecimal totalClosingDebit = domain::ExactDecimal::zero();
    domain::ExactDecimal totalClosingCredit = domain::ExactDecimal::zero();
    bool isReconciled = false;
    domain::ExactDecimal discrepancy = domain::ExactDecimal::zero();
};

// 2. Profit & Loss
struct PnLLedgerDetail {
    std::string ledgerId;
    std::string ledgerName;
    domain::ExactDecimal amount = domain::ExactDecimal::zero();
};

struct PnLGroupSection {
    std::string groupId;
    std::string groupName;
    domain::ExactDecimal totalAmount = domain::ExactDecimal::zero();
    std::vector<PnLLedgerDetail> ledgers;
    std::vector<PnLGroupSection> subGroups;
};

struct ProfitAndLossReport {
    std::string companyName;
    ReportPeriod period;

    struct TradingAccount {
        domain::ExactDecimal directIncomeTotal = domain::ExactDecimal::zero();
        std::vector<PnLGroupSection> directIncomeSections;
        domain::ExactDecimal directExpensesTotal = domain::ExactDecimal::zero();
        std::vector<PnLGroupSection> directExpenseSections;
        domain::ExactDecimal grossProfit = domain::ExactDecimal::zero();
        domain::ExactDecimal grossLoss = domain::ExactDecimal::zero();
        bool isGrossProfit = true;
    } tradingAccount;

    struct OperatingAccount {
        domain::ExactDecimal indirectIncomeTotal = domain::ExactDecimal::zero();
        std::vector<PnLGroupSection> indirectIncomeSections;
        domain::ExactDecimal indirectExpensesTotal = domain::ExactDecimal::zero();
        std::vector<PnLGroupSection> indirectExpenseSections;
        domain::ExactDecimal netProfit = domain::ExactDecimal::zero();
        domain::ExactDecimal netLoss = domain::ExactDecimal::zero();
        bool isNetProfit = true;
    } operatingAccount;
};

// 3. Balance Sheet
struct BalanceSheetLedgerDetail {
    std::string ledgerId;
    std::string ledgerName;
    domain::ExactDecimal amount = domain::ExactDecimal::zero();
    domain::BalanceType side = domain::BalanceType::Dr;
};

struct BalanceSheetGroupSection {
    std::string groupId;
    std::string groupName;
    domain::ExactDecimal totalAmount = domain::ExactDecimal::zero();
    std::vector<BalanceSheetLedgerDetail> ledgers;
    std::vector<BalanceSheetGroupSection> subGroups;
};

struct BalanceSheetReport {
    std::string companyName;
    std::string asOfDate;
    ReportPeriod period;

    struct AssetsSection {
        domain::ExactDecimal total = domain::ExactDecimal::zero();
        std::vector<BalanceSheetGroupSection> sections;
    } assets;

    struct LiabilitiesSection {
        domain::ExactDecimal total = domain::ExactDecimal::zero();
        std::vector<BalanceSheetGroupSection> sections;
    } liabilities;

    struct EquitySection {
        domain::ExactDecimal totalCapital = domain::ExactDecimal::zero();
        domain::ExactDecimal currentPeriodProfitOrLoss = domain::ExactDecimal::zero();
        bool isCurrentPeriodProfit = true;
        domain::ExactDecimal totalEquity = domain::ExactDecimal::zero();
        std::vector<BalanceSheetGroupSection> sections;
    } equity;

    domain::ExactDecimal totalAssets = domain::ExactDecimal::zero();
    domain::ExactDecimal totalLiabilitiesAndEquity = domain::ExactDecimal::zero();
    bool isReconciled = false;
    domain::ExactDecimal discrepancy = domain::ExactDecimal::zero();
};

class ReportService {
public:
    explicit ReportService(database::DatabaseManager& dbManager);

    TrialBalanceReport generateTrialBalance(
        const std::string& companyId,
        const std::optional<std::string>& fromDate = std::nullopt,
        const std::optional<std::string>& toDate = std::nullopt,
        bool includeZeroBalances = false
    );

    ProfitAndLossReport generateProfitAndLoss(
        const std::string& companyId,
        const std::optional<std::string>& fromDate = std::nullopt,
        const std::optional<std::string>& toDate = std::nullopt
    );

    BalanceSheetReport generateBalanceSheet(
        const std::string& companyId,
        const std::optional<std::string>& asOfDate = std::nullopt
    );

private:
    database::DatabaseManager& m_dbManager;

    ReportPeriod resolvePeriod(
        const std::string& companyId,
        const std::optional<std::string>& fromDate,
        const std::optional<std::string>& toDate
    );
};

} // namespace tally::services
