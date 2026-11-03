#include "ExactDecimal.hpp"
#include <cmath>
#include <charconv>
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace tally::domain {

namespace {

__int128_t roundHalfUpDivision(__int128_t numerator, __int128_t denominator) {
    if (denominator == 0) {
        throw std::invalid_argument("Division by zero in ExactDecimal");
    }
    bool negative = (numerator < 0) ^ (denominator < 0);
    __int128_t absNum = numerator < 0 ? -numerator : numerator;
    __int128_t absDen = denominator < 0 ? -denominator : denominator;

    __int128_t quotient = absNum / absDen;
    __int128_t remainder = absNum % absDen;

    // ROUND_HALF_UP: if remainder * 2 >= denominator, round up
    if (remainder * 2 >= absDen) {
        quotient += 1;
    }
    return negative ? -quotient : quotient;
}

} // namespace

ExactDecimal ExactDecimal::fromString(std::string_view str) {
    if (str.empty()) {
        return zero();
    }

    if (str.find('e') != std::string_view::npos || str.find('E') != std::string_view::npos) {
        try {
            return fromDouble(std::stod(std::string(str)));
        } catch (...) {
            throw std::invalid_argument("Invalid scientific notation in ExactDecimal string: " + std::string(str));
        }
    }

    size_t start = 0;
    while (start < str.size() && (str[start] == ' ' || str[start] == '\t')) {
        start++;
    }
    size_t end = str.size();
    while (end > start && (str[end - 1] == ' ' || str[end - 1] == '\t')) {
        end--;
    }

    if (start >= end) {
        return zero();
    }

    bool negative = false;
    if (str[start] == '-') {
        negative = true;
        start++;
    } else if (str[start] == '+') {
        start++;
    }

    size_t dotPos = str.find('.', start);
    __int128_t intPart = 0;
    __int128_t fracPart = 0;

    std::string_view intView = (dotPos == std::string_view::npos) 
        ? str.substr(start, end - start) 
        : str.substr(start, dotPos - start);

    for (char c : intView) {
        if (c >= '0' && c <= '9') {
            intPart = intPart * 10 + (c - '0');
        } else if (c == ',') {
            continue; // Ignore thousands commas in input
        } else {
            throw std::invalid_argument("Invalid numeric character in ExactDecimal string: " + std::string(str));
        }
    }

    if (dotPos != std::string_view::npos) {
        std::string_view fracView = str.substr(dotPos + 1, end - (dotPos + 1));
        int fracDigits = 0;
        for (char c : fracView) {
            if (c >= '0' && c <= '9') {
                if (fracDigits < SCALE) {
                    fracPart = fracPart * 10 + (c - '0');
                    fracDigits++;
                } else if (fracDigits == SCALE) {
                    // 5th digit used for half-up rounding into 4th decimal place
                    if (c >= '5') {
                        fracPart += 1;
                    }
                    fracDigits++;
                }
            } else {
                throw std::invalid_argument("Invalid fractional character in ExactDecimal: " + std::string(str));
            }
        }
        while (fracDigits < SCALE) {
            fracPart *= 10;
            fracDigits++;
        }
    }

    __int128_t totalScaled = intPart * MULTIPLIER + fracPart;
    return ExactDecimal(negative ? -totalScaled : totalScaled);
}

ExactDecimal ExactDecimal::fromDouble(double value) {
    if (std::isnan(value) || std::isinf(value)) {
        throw std::invalid_argument("Cannot construct ExactDecimal from NaN or Inf");
    }
    // Round to 4 decimal places
    __int128_t scaled = static_cast<__int128_t>(std::round(value * static_cast<double>(MULTIPLIER)));
    return ExactDecimal(scaled);
}

ExactDecimal ExactDecimal::add(const ExactDecimal& other) const noexcept {
    return ExactDecimal(m_scaledValue + other.m_scaledValue);
}

ExactDecimal ExactDecimal::sub(const ExactDecimal& other) const noexcept {
    return ExactDecimal(m_scaledValue - other.m_scaledValue);
}

ExactDecimal ExactDecimal::mul(const ExactDecimal& other) const noexcept {
    __int128_t product = m_scaledValue * other.m_scaledValue;
    return ExactDecimal(roundHalfUpDivision(product, MULTIPLIER));
}

ExactDecimal ExactDecimal::div(const ExactDecimal& other) const {
    if (other.isZero()) {
        throw std::invalid_argument("Division by zero in ExactDecimal");
    }
    __int128_t numerator = m_scaledValue * MULTIPLIER;
    return ExactDecimal(roundHalfUpDivision(numerator, other.m_scaledValue));
}

ExactDecimal ExactDecimal::abs() const noexcept {
    return ExactDecimal(m_scaledValue < 0 ? -m_scaledValue : m_scaledValue);
}

ExactDecimal ExactDecimal::neg() const noexcept {
    return ExactDecimal(-m_scaledValue);
}

std::string ExactDecimal::toDbString() const {
    bool negVal = m_scaledValue < 0;
    __int128_t absVal = negVal ? -m_scaledValue : m_scaledValue;

    __int128_t intPart = absVal / MULTIPLIER;
    __int128_t fracPart = absVal % MULTIPLIER;

    std::string intStr;
    if (intPart == 0) {
        intStr = "0";
    } else {
        __int128_t temp = intPart;
        while (temp > 0) {
            intStr.push_back(static_cast<char>('0' + (temp % 10)));
            temp /= 10;
        }
        std::reverse(intStr.begin(), intStr.end());
    }

    std::ostringstream ss;
    if (negVal) ss << "-";
    ss << intStr << "." << std::setw(SCALE) << std::setfill('0') << static_cast<int64_t>(fracPart);
    return ss.str();
}

std::string ExactDecimal::toCurrencyString() const {
    // Round to 2 decimal places using round-half-up
    bool negVal = m_scaledValue < 0;
    __int128_t absVal = negVal ? -m_scaledValue : m_scaledValue;

    // Multiply by 100/10000 -> divide by 100 with half up
    __int128_t rounded2dp = roundHalfUpDivision(absVal, 100);
    __int128_t intPart = rounded2dp / 100;
    __int128_t fracPart = rounded2dp % 100;

    std::string intStr;
    if (intPart == 0) {
        intStr = "0";
    } else {
        __int128_t temp = intPart;
        while (temp > 0) {
            intStr.push_back(static_cast<char>('0' + (temp % 10)));
            temp /= 10;
        }
        std::reverse(intStr.begin(), intStr.end());
    }

    std::ostringstream ss;
    if (negVal) ss << "-";
    ss << intStr << "." << std::setw(2) << std::setfill('0') << static_cast<int64_t>(fracPart);
    return ss.str();
}

std::string ExactDecimal::formatInr(bool includeSymbol) const {
    bool negVal = m_scaledValue < 0;
    __int128_t absVal = negVal ? -m_scaledValue : m_scaledValue;

    __int128_t rounded2dp = roundHalfUpDivision(absVal, 100);
    __int128_t intPart = rounded2dp / 100;
    __int128_t fracPart = rounded2dp % 100;

    std::string s;
    if (intPart == 0) {
        s = "0";
    } else {
        __int128_t temp = intPart;
        while (temp > 0) {
            s.push_back(static_cast<char>('0' + (temp % 10)));
            temp /= 10;
        }
        std::reverse(s.begin(), s.end());
    }

    // Format integer part with Indian grouping:
    // Last 3 digits, then groups of 2 digits
    std::string formattedInt;
    if (s.length() <= 3) {
        formattedInt = s;
    } else {
        std::string last3 = s.substr(s.length() - 3);
        std::string remaining = s.substr(0, s.length() - 3);

        std::string grouped;
        int count = 0;
        for (int i = static_cast<int>(remaining.length()) - 1; i >= 0; --i) {
            if (count == 2) {
                grouped.push_back(',');
                count = 0;
            }
            grouped.push_back(remaining[i]);
            count++;
        }
        std::reverse(grouped.begin(), grouped.end());
        formattedInt = grouped + "," + last3;
    }

    std::ostringstream ss;
    if (includeSymbol) {
        ss << "₹ ";
    }
    if (negVal) {
        ss << "-";
    }
    ss << formattedInt << "." << std::setw(2) << std::setfill('0') << static_cast<int64_t>(fracPart);
    return ss.str();
}

std::ostream& operator<<(std::ostream& os, const ExactDecimal& d) {
    return os << d.toDbString();
}

} // namespace tally::domain
