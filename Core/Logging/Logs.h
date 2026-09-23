#pragma once

#include "../Helper.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <format>
#include <fstream>
#include <mutex>
#include <print>
#include <string>
#include <string_view>
#include <windows.h>

namespace filesystem = std::filesystem;

enum class SeverityLevel : uint8_t
{
	None = 0,
	Verbose = 1 << 0, // 0x01 (1)
	Trace = 1 << 1,	  // 0x02 (2)
	Debug = 1 << 2,	  // 0x04 (4)
	Info = 1 << 3,	  // 0x08 (8)
	Http = 1 << 4,	  // 0x10 (16)
	Warning = 1 << 5, // 0x20 (32)
	Error = 1 << 6,	  // 0x40 (64)
	Fatal = 1 << 7,	  // 0x80 (128)

	Development = Verbose | Trace | Debug | Info | Http | Warning | Error | Fatal,
	Production = Info | Warning | Error | Fatal,
};

extern HANDLE hConsole;

inline constexpr std::size_t SEVERITY_LEVEL_MAX_SIZE = 8;

constexpr SeverityLevel operator|(SeverityLevel a, SeverityLevel b)
{
	return static_cast<SeverityLevel>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

constexpr SeverityLevel operator&(SeverityLevel a, SeverityLevel b)
{
	return static_cast<SeverityLevel>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

constexpr SeverityLevel operator~(SeverityLevel flag)
{
	return static_cast<SeverityLevel>(static_cast<uint8_t>(flag));
}

namespace winrt::CLauncher::Core::Logging
{
	class Logs
	{
	public:
#ifndef NDEBUG
		static inline SeverityLevel ACTIVE_LOG_LEVEL = SeverityLevel::Development;
#else
		static inline SeverityLevel ACTIVE_LOG_LEVEL = SeverityLevel::Production;
#endif
		static inline bool OUTPUT_TO_CONSOLE = true;
		static inline bool OUTPUT_TO_FILE = true;

		static void AllocateConsole();
		static void write(SeverityLevel level, const char* file, uint32_t line, const char* function, const std::string_view message);

	private:
		static inline const std::string LOG_FOLDER = "Logs";
		static inline const std::string LOG_FILENAME = "CLauncher.log";
		static inline std::mutex MUTEX;

		static std::string_view LEVEL_TO_STRING(SeverityLevel level);
		static std::string_view LEVEL_TO_COLOR(SeverityLevel level);
	}; // class Logs
} // namespace winrt::CLauncher::Core::Logging

#define LOG_VERBOSE(...) \
	winrt::CLauncher::Core::Logging::Logs::write(SeverityLevel::Verbose, __FILE__, __LINE__, __FUNCTION__, std::format(__VA_ARGS__))
#define LOG_TRACE(...) \
	winrt::CLauncher::Core::Logging::Logs::write(SeverityLevel::Trace, __FILE__, __LINE__, __FUNCTION__, std::format(__VA_ARGS__))
#define LOG_DEBUG(...) \
	winrt::CLauncher::Core::Logging::Logs::write(SeverityLevel::Debug, __FILE__, __LINE__, __FUNCTION__, std::format(__VA_ARGS__))
#define LOG_INFO(...) \
	winrt::CLauncher::Core::Logging::Logs::write(SeverityLevel::Info, __FILE__, __LINE__, __FUNCTION__, std::format(__VA_ARGS__))
#define LOG_HTTP(...) \
	winrt::CLauncher::Core::Logging::Logs::write(SeverityLevel::Http, __FILE__, __LINE__, __FUNCTION__, std::format(__VA_ARGS__))
#define LOG_WARNING(...) \
	winrt::CLauncher::Core::Logging::Logs::write(SeverityLevel::Warning, __FILE__, __LINE__, __FUNCTION__, std::format(__VA_ARGS__))
#define LOG_ERROR(...) \
	winrt::CLauncher::Core::Logging::Logs::write(SeverityLevel::Error, __FILE__, __LINE__, __FUNCTION__, std::format(__VA_ARGS__))
#define LOG_FATAL(...) \
	winrt::CLauncher::Core::Logging::Logs::write(SeverityLevel::Fatal, __FILE__, __LINE__, __FUNCTION__, std::format(__VA_ARGS__))
