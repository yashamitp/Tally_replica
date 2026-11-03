#pragma once

#include <string>
#include <chrono>

namespace tally::utilities {

class DateUtils {
public:
    static std::string currentIsoDateTime();
    static std::string currentIsoDate();
    static std::string formatTallyDisplayDate(const std::string& isoDate); // e.g. "2026-04-01" -> "1-Apr-26"
    static std::string getDayOfWeek(const std::string& isoDate);          // e.g. "Wednesday"
    static std::string addFinancialYear(const std::string& startDate);     // e.g. "2026-04-01" -> "2027-03-31"
};

} // namespace tally::utilities
