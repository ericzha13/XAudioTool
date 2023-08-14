#pragma once

#include <cstdio>
#include <string>
#include <chrono>
#include <thread>



#ifdef _WIN32
#include "Windows.h"
#else
#include <ctime>
#include <fcntl.h>
#include <sys/time.h>
#endif

#define scope_delay_ms(ti)  ScopeDelay ST1(ti)

class ScopeDelay
{
public:
    ScopeDelay() = delete;
    ScopeDelay(ScopeDelay&&) = delete;
    ScopeDelay(const ScopeDelay&) = delete;
    ScopeDelay(long long milsecond) {
        start = std::chrono::high_resolution_clock::now();
        duration_time = milsecond;
    }
    ~ScopeDelay() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();

        if (duration < duration_time) {
                std::this_thread::sleep_for(std::chrono::milliseconds(duration_time-duration));
        }
    }


private:
        std::chrono::time_point<std::chrono::high_resolution_clock> start;
        long long duration_time = 0;
};



inline std::string get_format_time()
{
    char buff[128] = { 0 };
#ifdef _WIN32
    SYSTEMTIME curtime;
    GetLocalTime(&curtime);
#ifdef _MSC_VER
    sprintf_s(buff, sizeof(buff),
#else  // ! _MSC_VER
    sprintf(buff,
#endif // END _MSC_VER
        "%04d-%02d-%02d %02d:%02d:%02d.%03d", curtime.wYear, curtime.wMonth, curtime.wDay, curtime.wHour,
        curtime.wMinute, curtime.wSecond, curtime.wMilliseconds);

#else  // ! _WIN32
    struct timeval tv = {};
    gettimeofday(&tv, nullptr);
    time_t nowtime = tv.tv_sec;
    struct tm* tm_info = localtime(&nowtime);
    auto offset = strftime(buff, sizeof(buff), "%Y-%m-%d %H:%M:%S", tm_info);
    sprintf(buff + offset, ".%03ld", static_cast<long int>(tv.tv_usec / 1000));
#endif // END _WIN32
    return buff;
}


