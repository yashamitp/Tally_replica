#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <stdexcept>
#include <ostream>
#include <compare>

namespace tally::domain {

/**
 * @brief Exact decimal arithmetic class for financial accounting.
 * Eliminates IEEE-754 floating point drift using scaled 128-bit integers.
 * Internal scaling factor is 10^4 (4 decimal places).
 * Uses ROUND_HALF_UP rounding for multiplication and division.
 */
class ExactDecimal {
public:
    static constexpr int SCALE = 4;
    static constexpr __int128_t MULTIPLIER = 10000; // 10^4

    ExactDecimal() noexcept : m_scaledValue(0) {}
    explicit ExactDecimal(__int128_t scaledValue) noexcept : m_scaledValue(scaledValue) {}
    explicit ExactDecimal(int64_t integerVal) noexcept : m_scaledValue(static_cast<__int128_t>(integerVal) * MULTIPLIER) {}
    explicit ExactDecimal(int integerVal) noexcept : m_scaledValue(static_cast<__int128_t>(integerVal) * MULTIPLIER) {}

    static ExactDecimal zero() noexcept { return ExactDecimal(static_cast<__int128_t>(0)); }
    static ExactDecimal fromString(std::string_view str);
    static ExactDecimal fromDouble(double value);
    static ExactDecimal fromRawScaled(__int128_t raw) noexcept { return ExactDecimal(raw); }

    // Arithmetic
    ExactDecimal add(const ExactDecimal& other) const noexcept;
    ExactDecimal sub(const ExactDecimal& other) const noexcept;
    ExactDecimal mul(const ExactDecimal& other) const noexcept;
    ExactDecimal div(const ExactDecimal& other) const;
    ExactDecimal abs() const noexcept;
    ExactDecimal neg() const noexcept;

    // Operator overloads
    ExactDecimal operator+(const ExactDecimal& other) const noexcept { return add(other); }
    ExactDecimal operator-(const ExactDecimal& other) const noexcept { return sub(other); }
    ExactDecimal operator*(const ExactDecimal& other) const noexcept { return mul(other); }
    ExactDecimal operator/(const ExactDecimal& other) const { return div(other); }
    ExactDecimal operator-() const noexcept { return neg(); }

    ExactDecimal& operator+=(const ExactDecimal& other) noexcept { *this = add(other); return *this; }
    ExactDecimal& operator-=(const ExactDecimal& other) noexcept { *this = sub(other); return *this; }
    ExactDecimal& operator*=(const ExactDecimal& other) noexcept { *this = mul(other); return *this; }
    ExactDecimal& operator/=(const ExactDecimal& other) { *this = div(other); return *this; }

    // Comparisons
    bool operator==(const ExactDecimal& other) const noexcept { return m_scaledValue == other.m_scaledValue; }
    std::strong_ordering operator<=>(const ExactDecimal& other) const noexcept {
        return m_scaledValue <=> other.m_scaledValue;
    }

    bool equals(const ExactDecimal& other) const noexcept { return *this == other; }
    bool greaterThan(const ExactDecimal& other) const noexcept { return *this > other; }
    bool lessThan(const ExactDecimal& other) const noexcept { return *this < other; }
    bool greaterThanOrEqualTo(const ExactDecimal& other) const noexcept { return *this >= other; }
    bool lessThanOrEqualTo(const ExactDecimal& other) const noexcept { return *this <= other; }

    bool isZero() const noexcept { return m_scaledValue == 0; }
    bool isPositive() const noexcept { return m_scaledValue > 0; }
    bool isNegative() const noexcept { return m_scaledValue < 0; }
    bool isNonNegative() const noexcept { return m_scaledValue >= 0; }

    // Formatting & Representation
    std::string toDbString() const;        // e.g. "150000.0000"
    std::string toCurrencyString() const;  // e.g. "150000.00"
    std::string formatInr(bool includeSymbol = false) const; // e.g. "1,50,000.00" or "₹ 1,50,000.00"
    std::string toString() const { return toDbString(); }
    double toDouble() const noexcept { return static_cast<double>(m_scaledValue) / static_cast<double>(MULTIPLIER); }
    __int128_t rawScaled() const noexcept { return m_scaledValue; }

private:
    __int128_t m_scaledValue; // Scaled by 10^4
};

std::ostream& operator<<(std::ostream& os, const ExactDecimal& d);

} // namespace tally::domain
