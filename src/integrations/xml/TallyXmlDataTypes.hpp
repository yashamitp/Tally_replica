#pragma once

#include <string>
#include <vector>
#include <optional>
#include <map>
#include "../../domain/accounting/DoubleEntryValidator.hpp"

namespace tally::integrations::xml {

struct RawTallyGroup {
    std::string name;
    std::string parentName;
};

struct RawTallyLedger {
    std::string name;
    std::string parentGroupName;
    std::string openingBalance; // Raw string
    std::string isDeemedPositive; // "Yes" or "No"
    std::string gstin;
    std::string pan;
    std::string address;
    std::string state;
    std::string pincode;
};

struct RawTallyVoucherLine {
    std::string ledgerName;
    domain::BalanceType entryType = domain::BalanceType::Dr;
    domain::ExactDecimal amount = domain::ExactDecimal::zero();
    std::string narration;
};

struct RawTallyVoucher {
    std::string voucherTypeName = "Journal";
    std::string voucherNumber;
    std::string date; // YYYY-MM-DD
    std::string referenceNumber;
    std::string narration;
    std::string guid;
    std::vector<RawTallyVoucherLine> entries;
    bool isBalanced = false;
    domain::ExactDecimal totalDebit = domain::ExactDecimal::zero();
    domain::ExactDecimal totalCredit = domain::ExactDecimal::zero();
};

enum class PreviewAction {
    Create,
    Skip,
    Conflict,
    Reject
};

struct ImportPreviewItem {
    std::string type; // "GROUP", "LEDGER", "VOUCHER"
    std::string identifier;
    PreviewAction action;
    std::string reason;
};

struct ImportPreviewResult {
    std::string sourceFormat = "TALLY_XML";
    std::vector<std::string> companiesDetected;
    int groupsCount = 0;
    int ledgersCount = 0;
    int vouchersCount = 0;
    int acceptedCount = 0;
    int rejectedCount = 0;
    int skippedCount = 0;
    std::vector<ImportPreviewItem> items;
    domain::ExactDecimal expectedDebitsSum = domain::ExactDecimal::zero();
    domain::ExactDecimal expectedCreditsSum = domain::ExactDecimal::zero();
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
    bool canCommit = false;
};

struct ImportExecutionResult {
    bool success = false;
    int createdGroups = 0;
    int createdLedgers = 0;
    int createdVouchers = 0;
    int skippedCount = 0;
    int failedCount = 0;
    std::vector<std::string> errors;
};

} // namespace tally::integrations::xml
