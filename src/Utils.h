#pragma once

#include "Common.h"
#include "Base/Types.h"

#include <filesystem>
#include <functional>
#include <thread>
#include <map>


#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
using AModuleHandle = HMODULE;
#else
#include <dlfcn.h>
using AModuleHandle = void *;
#endif


using AThreadID = std::thread::id;

const auto NowTimePoint = std::chrono::system_clock::now;

namespace utils {
    BASE_API void TraverseFolder(const std::string &folder, const std::function<void(const std::filesystem::directory_entry &)> &func);

    BASE_API std::string StringReplace(std::string source, char toReplace, char replacement);

    BASE_API int64_t ThreadIDToInt(AThreadID tid);

    BASE_API std::string PascalToUnderline(const std::string &src);

    BASE_API std::vector<uint8_t> HexToBytes(const std::string &hex);

    BASE_API long long UnixTime();
    BASE_API long long ToUnixTime(ASystemTimePoint point);

    BASE_API int64_t SetBit(int64_t, int32_t);
    BASE_API int64_t ClearBit(int64_t, int32_t);
    BASE_API int64_t ToggleBit(int64_t, int32_t);
    BASE_API bool CheckBit(int64_t, int32_t);

    BASE_API std::vector<std::string> SplitString(const std::string &src, char delimiter);
    BASE_API std::vector<int> SplitStringToInt(const std::string &src, char delimiter);

    /**
     * Get Day Of The Week
     * @param point Time Point, Default Now
     * @return From 0 To 6, Means Sunday(0) To StaterDay(6)
     */
    BASE_API unsigned GetDayOfWeek(ASystemTimePoint point = NowTimePoint());
    BASE_API unsigned GetDayOfMonth(ASystemTimePoint point = NowTimePoint());
    BASE_API int GetDayOfYear(ASystemTimePoint point = NowTimePoint());

    /**
     * 往日不再
     * @param former 较前的时间点
     * @param latter 较后的时间点 默认当前时间点
     * @return 经过的天数 同一天为0
     */
    BASE_API int GetDaysGone(ASystemTimePoint former, ASystemTimePoint latter = NowTimePoint());
    BASE_API ASystemTimePoint GetDayZeroTime(ASystemTimePoint point = NowTimePoint());

    BASE_API bool IsSameWeek(ASystemTimePoint former, ASystemTimePoint latter = NowTimePoint());
    BASE_API bool IsSameMonth(ASystemTimePoint former, ASystemTimePoint latter = NowTimePoint());

    BASE_API int RandomDraw(const std::map<int, int> &pool);

    // template<class T>
    // void CleanUpWeakPointerSet(std::unordered_set<std::weak_ptr<T>, FWeakPointerHash<T>, FWeakPointerEqual<T>> &set) {
    //     for (auto it = set.begin(); it != set.end();) {
    //         if (it->expired()) {
    //             it = set.erase(it);
    //         } else {
    //             ++it;
    //         }
    //     }
    // }
}
