#include "DateUtils.hpp"
#include <ctime>
#include <iomanip>
#include <sstream>

namespace tally::utilities {

std::string DateUtils::currentIsoDateTime() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

std::string DateUtils::currentIsoDate() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d");
    return ss.str();
}

std::string DateUtils::formatTallyDisplayDate(const std::string& isoDate) {
    if (isoDate.length() < 10) return isoDate;
    int year = std::stoi(isoDate.substr(0, 4));
    int month = std::stoi(isoDate.substr(5, 2));
    int day = std::stoi(isoDate.substr(8, 2));

    const char* months[] = {
        "", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };

    std::string monStr = (month >= 1 && month <= 12) ? months[month] : "Jan";
    int shortYear = year % 100;

    std::ostringstream ss;
    ss << day << "-" << monStr << "-" << std::setw(2) << std::setfill('0') << shortYear;
    return ss.str();
}

std::string DateUtils::getDayOfWeek(const std::string& isoDate) {
    if (isoDate.length() < 10) return "";
    int year = std::stoi(isoDate.substr(0, 4));
    int month = std::stoi(isoDate.substr(5, 2));
    int day = std::stoi(isoDate.substr(8, 2));

    std::tm tm{};
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    std::mktime(&tm);

    const char* days[] = {
        "Sunday", "Monday", "Tuesday", "Wednesday",
        "Thursday", "Friday", "Saturday"
    };

    return (tm.tm_wday >= 0 && tm.tm_wday <= 6) ? days[tm.tm_wday] : "";
}

std::string DateUtils::addFinancialYear(const std::string& startDate) {
    if (startDate.length() < 4) return "2027-03-31";
    int year = std::stoi(startDate.substr(0, 4));
    int nextYear = year + 1;
    return std::to_string(nextYear) + "-03-31";
}

} // namespace tally::utilities
