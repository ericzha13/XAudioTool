#pragma once

#include <cstdio>
#include <string>
#include <chrono>
#include <thread>
#include <iostream>
#include <regex>
#include <sstream>



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
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

		if (duration < duration_time) {
			std::this_thread::sleep_for(std::chrono::milliseconds(duration_time - duration));
		}
	}


private:
	std::chrono::time_point<std::chrono::high_resolution_clock> start;
	long long duration_time = 0;
};

namespace ast::utils
{
	


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

	inline std::string get_format_time(bool is_name)
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
			"%04d%02d%02d%02d%02d%02d%03d", curtime.wYear, curtime.wMonth, curtime.wDay, curtime.wHour,
			curtime.wMinute, curtime.wSecond, curtime.wMilliseconds);

#else  // ! _WIN32
		struct timeval tv = {};
		gettimeofday(&tv, nullptr);
		time_t nowtime = tv.tv_sec;
		struct tm* tm_info = localtime(&nowtime);
		auto offset = strftime(buff, sizeof(buff), "%Y%m%d%H%M%S", tm_info);
		sprintf(buff + offset, ".%03ld", static_cast<long int>(tv.tv_usec / 1000));
#endif // END _WIN32
		return buff;
	}



	// 将时分秒字符串转换成时间变量、待优化判断
	inline long parse_time(std::string timeStr, int& hours, int& minutes, int& seconds, int& milliseconds) {
		int mcount = count(timeStr.begin(), timeStr.end(), ':') + count(timeStr.begin(), timeStr.end(), '.');
		if (mcount != 3) {
			std::cerr << "Invalid time format: " << timeStr << std::endl;
			return -1;
		}

		std::stringstream ss(timeStr);
		char delimiter;
		ss >> hours >> delimiter >> minutes >> delimiter >> seconds >> delimiter >> milliseconds;

		if (hours < 0 || hours >= 24 || minutes < 0 || minutes >= 60 || seconds < 0 || seconds >= 60 || milliseconds < 0 || milliseconds >= 1000) {
			std::cerr << "Invalid time format: " << timeStr << std::endl;
			return -1;
		}
		return hours * 3600000 + minutes * 60000 + seconds * 1000 + milliseconds;
	}



	inline std::string time_format(long ms) {
		if (ms > LONG_MAX || ms < 0) {
			printf("毫秒数异常");
			return "";
		}
		int hour = ms / (60 * 60 * 1000);
		int minute = (ms - hour * 60 * 60 * 1000) / (60 * 1000);
		int second = (ms - hour * 60 * 60 * 1000 - minute * 60 * 1000) / 1000;
		int mms = ms % 1000;

		return std::to_string(hour) + ":" + std::to_string(minute) + ":" + std::to_string(second) + "." + std::to_string(mms);

	}





	// 将时分秒字符串转换成毫秒时间，必须是hh:mm:ss.xxx格式的时间
	inline long parse_time(std::string timeStr) {
		int mcount = count(timeStr.begin(), timeStr.end(), ':') + count(timeStr.begin(), timeStr.end(), '.');
		if (mcount != 3) {
			std::cerr << "Invalid time format: " << timeStr << std::endl;
			return -1;
		}

		int hours = 0, minutes = 0, seconds = 0, milliseconds = 0;
		std::stringstream ss(timeStr);
		char delimiter;
		ss >> hours >> delimiter >> minutes >> delimiter >> seconds >> delimiter >> milliseconds;

		if (hours < 0 || hours >= 24 || minutes < 0 || minutes >= 60 || seconds < 0 || seconds >= 60 || milliseconds < 0 || milliseconds >= 1000) {
			std::cerr << "Invalid time format: " << timeStr << std::endl;
			return -1;
		}
		return hours * 3600000 + minutes * 60000 + seconds * 1000 + milliseconds;
	}


	inline bool check_time_format(const std::string& time_str) {
		std::regex time_regex(R"(\d{2}:\d{2}:\d{2}\.\d{3})");  // 正则表达式匹配 hh:mm:ss.xxx 格式
		return std::regex_match(time_str, time_regex);
	}

}
