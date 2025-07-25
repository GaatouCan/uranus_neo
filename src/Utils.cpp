#include "Utils.h"

#include <cassert>
#include <random>
#include <ranges>


namespace utils {
    void TraverseFolder(const std::string &folder, const std::function<void(const std::filesystem::directory_entry &)> &func) {
        for (const auto &entry: std::filesystem::directory_iterator(folder)) {
            if (entry.is_directory()) {
                TraverseFolder(entry.path().string(), func);
            } else {
                std::invoke(func, entry);
            }
        }
    }

    std::string StringReplace(std::string source, const char toReplace, const char replacement) {
        size_t pos = 0;
        while ((pos = source.find(toReplace, pos)) != std::string::npos) {
            source.replace(pos, 1, 1, replacement);
            pos += 1;
        }
        return source;
    }

    int64_t ThreadIDToInt(const AThreadID tid) {
        std::stringstream ss;
        ss << tid;
        return std::stoll(ss.str());
    }

    std::string PascalToUnderline(const std::string &src) {
        if (src.empty()) return "";

        std::string result;
        result.reserve(src.size());

        for (auto idx = 0; idx < src.size(); ++idx) {
            if (idx > 0 && isupper(src[idx])) {
                result.push_back('_');
            }
            result.push_back(tolower(src[idx]));
        }

        return result;
    }

    std::vector<uint8_t> HexToBytes(const std::string &hex) {
        std::vector<uint8_t> result;
        result.reserve(hex.size() / 2);
        for (size_t i = 0; i < hex.size(); i += 2) {
            result.push_back(static_cast<uint8_t>(std::stoi(hex.substr(i, 2), nullptr, 16)));
        }
        return result;
    }

    long long UnixTime() {
        const auto now = NowTimePoint();
        const auto durationSinceEpoch = now.time_since_epoch();
        const auto secondsSinceEpoch = std::chrono::duration_cast<std::chrono::seconds>(durationSinceEpoch);
        return secondsSinceEpoch.count();
    }

    long long ToUnixTime(const ASystemTimePoint point) {
        const auto durationSinceEpoch = point.time_since_epoch();
        const auto secondsSinceEpoch = std::chrono::duration_cast<std::chrono::seconds>(durationSinceEpoch);
        return secondsSinceEpoch.count();
    }

    int64_t SetBit(const int64_t number, const int32_t n) {
        assert(n >= 0 && n < sizeof(int64_t) * 8);
        return number | static_cast<int64_t>(1) << n;
    }

    int64_t ClearBit(const int64_t number, const int32_t n) {
        assert(n >= 0 && n < sizeof(int64_t) * 8);
        return number & ~(static_cast<int64_t>(1) << n);
    }

    int64_t ToggleBit(const int64_t number, const int32_t n) {
        assert(n >= 0 && n < sizeof(int64_t) * 8);
        return number ^ static_cast<int64_t>(1) << n;
    }

    bool CheckBit(const int64_t number, const int32_t n) {
        assert(n >= 0 && n < sizeof(int64_t) * 8);
        return number & static_cast<int64_t>(1) << n;
    }

    std::vector<std::string> SplitString(const std::string &src, const char delimiter) {
        std::vector<std::string> result;
        std::stringstream ss(src);
        std::string item;

        while (std::getline(ss, item, delimiter)) {
            result.push_back(item);
        }

        return result;
    }


    std::vector<int> SplitStringToInt(const std::string &src, const char delimiter) {
        std::vector<int> result;
        std::stringstream ss(src);
        std::string item;

        while (std::getline(ss, item, delimiter)) {
            result.push_back(std::stoi(item));
        }

        return result;
    }

    unsigned GetDayOfWeek(const ASystemTimePoint point) {
        const auto localTime = std::chrono::current_zone()->to_local(point);
        const auto weekDay = std::chrono::weekday{std::chrono::floor<std::chrono::days>(localTime)};

        return weekDay.c_encoding();
    }

    unsigned GetDayOfMonth(const ASystemTimePoint point) {
        const auto localTime = std::chrono::current_zone()->to_local(point);
        const std::chrono::year_month_day ymd{std::chrono::floor<std::chrono::days>(localTime)};
        return static_cast<unsigned>(ymd.day());
    }

    int GetDayOfYear(const ASystemTimePoint point) {
        const auto localTime = std::chrono::current_zone()->to_local(point);

        const auto today = std::chrono::floor<std::chrono::days>(localTime);

        const auto thisYear = std::chrono::floor<std::chrono::years>(localTime);
        const auto firstDayOfYear = std::chrono::floor<std::chrono::days>(thisYear);

        return today.time_since_epoch().count() - firstDayOfYear.time_since_epoch().count() + 1;
    }

    int GetDaysGone(const ASystemTimePoint former, const ASystemTimePoint latter) {
        if (latter <= former)
            return 0;

        const auto formerLocalTime = std::chrono::current_zone()->to_local(former);
        const auto latterLocalTime = std::chrono::current_zone()->to_local(latter);

        const auto formerDays = std::chrono::floor<std::chrono::days>(formerLocalTime);
        const auto latterDays = std::chrono::floor<std::chrono::days>(latterLocalTime);

        return latterDays.time_since_epoch().count() - formerDays.time_since_epoch().count();
    }

    ASystemTimePoint GetDayZeroTime(const ASystemTimePoint point) {
        const auto zeroLocalTime = std::chrono::floor<std::chrono::days>(std::chrono::current_zone()->to_local(point));
        return std::chrono::current_zone()->to_sys(zeroLocalTime);
    }

    bool IsSameWeek(const ASystemTimePoint former, const ASystemTimePoint latter) {
        if (latter <= former)
            return false;

        const auto formerLocalTime = std::chrono::current_zone()->to_local(former);
        const auto latterLocalTime = std::chrono::current_zone()->to_local(latter);

        const std::chrono::year_month_day formerYmd{std::chrono::floor<std::chrono::days>(formerLocalTime)};
        const std::chrono::year_month_day latterYmd{std::chrono::floor<std::chrono::days>(latterLocalTime)};

        const auto formerWeekDay = std::chrono::weekday{std::chrono::floor<std::chrono::days>(formerLocalTime)};
        const auto latterWeekDay = std::chrono::weekday{std::chrono::floor<std::chrono::days>(latterLocalTime)};

        const auto formerWeekStart = std::chrono::sys_days{formerYmd} - (formerWeekDay - std::chrono::Monday);
        const auto latterWeekStart = std::chrono::sys_days{latterYmd} - (latterWeekDay - std::chrono::Monday);

        return formerWeekStart == latterWeekStart;
    }

    bool IsSameMonth(const ASystemTimePoint former, const ASystemTimePoint latter) {
        if (latter <= former)
            return false;

        const auto formerLocalTime = std::chrono::current_zone()->to_local(former);
        const auto latterLocalTime = std::chrono::current_zone()->to_local(latter);

        const std::chrono::year_month_day formerYmd{std::chrono::floor<std::chrono::days>(formerLocalTime)};
        const std::chrono::year_month_day latterYmd{std::chrono::floor<std::chrono::days>(latterLocalTime)};

        return formerYmd.year() == latterYmd.year() && formerYmd.month() == latterYmd.month();
    }

    int RandomDraw(const std::map<int, int> &pool) {
        std::random_device randomDevice;
        std::mt19937 generator(randomDevice());

        int weight = 0, hit = 0;

        for (const auto &val: pool | std::views::values) {
            weight += val;
        }

        if (weight == 0)
            return -1;

        std::uniform_int_distribution distribution(0, weight);

        hit = distribution(generator);

        for (const auto &[k, v]: pool) {
            hit -= v;
            if (hit < 0)
                return k;
        }

        return -1;
    }
}
