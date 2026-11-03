#pragma once

#include <vector>
#include <string>
#include "../decimal/ExactDecimal.hpp"
#include "../voucher/Voucher.hpp"

namespace tally::domain {

struct DoubleEntryValidationResult {
    bool isBalanced = false;
    ExactDecimal totalDebit = ExactDecimal::zero();
    ExactDecimal totalCredit = ExactDecimal::zero();
    ExactDecimal difference = ExactDecimal::zero();
    std::string errorMessage;
};

class DoubleEntryValidator {
public:
    static DoubleEntryValidationResult validate(const std::vector<VoucherEntry>& entries) {
        DoubleEntryValidationResult result;

        if (entries.size() < 2) {
            result.isBalanced = false;
            result.errorMessage = "A valid double-entry voucher requires at least two line items (at least one Debit and one Credit).";
            return result;
        }

        bool hasDebit = false;
        bool hasCredit = false;

        for (const auto& entry : entries) {
            if (!entry.amount.isPositive()) {
                result.isBalanced = false;
                result.errorMessage = "Invalid entry amount: " + entry.amount.toDbString() + ". All entries must have strictly positive amounts.";
                return result;
            }

            if (entry.entryType == BalanceType::Dr) {
                result.totalDebit = result.totalDebit.add(entry.amount);
                hasDebit = true;
            } else {
                result.totalCredit = result.totalCredit.add(entry.amount);
                hasCredit = true;
            }
        }

        if (!hasDebit || !hasCredit) {
            result.isBalanced = false;
            result.difference = (result.totalDebit.sub(result.totalCredit)).abs();
            result.errorMessage = "Voucher must contain at least one Debit (DR) and at least one Credit (CR) entry.";
            return result;
        }

        result.difference = (result.totalDebit.sub(result.totalCredit)).abs();
        result.isBalanced = result.totalDebit.equals(result.totalCredit);

        if (!result.isBalanced) {
            result.errorMessage = "Unbalanced transaction: Total Debits (" + result.totalDebit.toDbString() + 
                                  ") != Total Credits (" + result.totalCredit.toDbString() + 
                                  "). Discrepancy: " + result.difference.toDbString() + ".";
        }

        return result;
    }
};

} // namespace tally::domain
