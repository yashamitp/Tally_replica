#include <cassert>
#include <iostream>
#include "../../src/domain/decimal/ExactDecimal.hpp"

using tally::domain::ExactDecimal;

void testFloatingPointDrift() {
    // In IEEE-754: 0.1 + 0.2 == 0.30000000000000004
    ExactDecimal a = ExactDecimal::fromString("0.1");
    ExactDecimal b = ExactDecimal::fromString("0.2");
    ExactDecimal sum = a.add(b);

    assert(sum.toDbString() == "0.3000");
    assert(sum.toCurrencyString() == "0.30");
    assert(sum.equals(ExactDecimal::fromString("0.3")));
    std::cout << "[PASS] testFloatingPointDrift" << std::endl;
}

void testHighPrecisionArithmetic() {
    ExactDecimal d1 = ExactDecimal::fromString("1250000.5555");
    ExactDecimal d2 = ExactDecimal::fromString("750000.4445");
    ExactDecimal total = d1.add(d2);

    assert(total.toDbString() == "2000001.0000");
    assert(total.sub(d2).equals(d1));
    std::cout << "[PASS] testHighPrecisionArithmetic" << std::endl;
}

void testMulDivAndRounding() {
    ExactDecimal qty = ExactDecimal::fromString("3.0000");
    ExactDecimal rate = ExactDecimal::fromString("33.3333");
    ExactDecimal amount = qty.mul(rate);
    assert(amount.toDbString() == "99.9999");

    ExactDecimal divided = ExactDecimal::fromString("100.0000").div(ExactDecimal::fromString("3.0000"));
    assert(divided.toCurrencyString() == "33.33");
    std::cout << "[PASS] testMulDivAndRounding" << std::endl;
}

void testDivisionByZero() {
    ExactDecimal val = ExactDecimal::fromString("1000");
    bool threw = false;
    try {
        val.div(ExactDecimal::zero());
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    assert(threw);
    std::cout << "[PASS] testDivisionByZero" << std::endl;
}

void testIndianRupeeFormatting() {
    ExactDecimal d1 = ExactDecimal::fromString("150000.00");
    assert(d1.formatInr(false) == "1,50,000.00");
    assert(d1.formatInr(true) == "₹ 1,50,000.00");

    ExactDecimal d2 = ExactDecimal::fromString("12345678.90");
    assert(d2.formatInr(false) == "1,23,45,678.90");
    std::cout << "[PASS] testIndianRupeeFormatting" << std::endl;
}

int main() {
    std::cout << "--- Running ExactDecimal Unit Tests ---" << std::endl;
    testFloatingPointDrift();
    testHighPrecisionArithmetic();
    testMulDivAndRounding();
    testDivisionByZero();
    testIndianRupeeFormatting();
    std::cout << "All ExactDecimal unit tests passed!" << std::endl;
    return 0;
}
