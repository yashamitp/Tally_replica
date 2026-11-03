#include <cassert>
#include <iostream>
#include "../../src/domain/accounting/DoubleEntryValidator.hpp"

using tally::domain::DoubleEntryValidator;
using tally::domain::VoucherEntry;
using tally::domain::BalanceType;
using tally::domain::ExactDecimal;

void testLessThanTwoEntries() {
    std::vector<VoucherEntry> entries;
    VoucherEntry e1;
    e1.ledgerId = "ledger-1";
    e1.entryType = BalanceType::Dr;
    e1.amount = ExactDecimal::fromString("100.00");
    entries.push_back(e1);

    auto result = DoubleEntryValidator::validate(entries);
    assert(!result.isBalanced);
    std::cout << "[PASS] testLessThanTwoEntries" << std::endl;
}

void testMissingCreditOrDebit() {
    std::vector<VoucherEntry> entries;
    VoucherEntry e1, e2;
    e1.ledgerId = "ledger-1";
    e1.entryType = BalanceType::Dr;
    e1.amount = ExactDecimal::fromString("50.00");

    e2.ledgerId = "ledger-2";
    e2.entryType = BalanceType::Dr;
    e2.amount = ExactDecimal::fromString("50.00");

    entries.push_back(e1);
    entries.push_back(e2);

    auto result = DoubleEntryValidator::validate(entries);
    assert(!result.isBalanced);
    std::cout << "[PASS] testMissingCreditOrDebit" << std::endl;
}

void testUnbalancedEntries() {
    std::vector<VoucherEntry> entries;
    VoucherEntry e1, e2;
    e1.ledgerId = "ledger-1";
    e1.entryType = BalanceType::Dr;
    e1.amount = ExactDecimal::fromString("100.00");

    e2.ledgerId = "ledger-2";
    e2.entryType = BalanceType::Cr;
    e2.amount = ExactDecimal::fromString("95.00");

    entries.push_back(e1);
    entries.push_back(e2);

    auto result = DoubleEntryValidator::validate(entries);
    assert(!result.isBalanced);
    assert(result.difference == ExactDecimal::fromString("5.0000"));
    std::cout << "[PASS] testUnbalancedEntries" << std::endl;
}

void testBalancedEntries() {
    std::vector<VoucherEntry> entries;
    VoucherEntry e1, e2, e3;
    e1.ledgerId = "bank";
    e1.entryType = BalanceType::Dr;
    e1.amount = ExactDecimal::fromString("1000.00");

    e2.ledgerId = "sales";
    e2.entryType = BalanceType::Cr;
    e2.amount = ExactDecimal::fromString("900.00");

    e3.ledgerId = "gst";
    e3.entryType = BalanceType::Cr;
    e3.amount = ExactDecimal::fromString("100.00");

    entries.push_back(e1);
    entries.push_back(e2);
    entries.push_back(e3);

    auto result = DoubleEntryValidator::validate(entries);
    assert(result.isBalanced);
    assert(result.totalDebit == ExactDecimal::fromString("1000.0000"));
    assert(result.totalCredit == ExactDecimal::fromString("1000.0000"));
    assert(result.difference.isZero());
    std::cout << "[PASS] testBalancedEntries" << std::endl;
}

int main() {
    std::cout << "--- Running Double-Entry Validation Tests ---" << std::endl;
    testLessThanTwoEntries();
    testMissingCreditOrDebit();
    testUnbalancedEntries();
    testBalancedEntries();
    std::cout << "All Double-Entry validation tests passed!" << std::endl;
    return 0;
}
