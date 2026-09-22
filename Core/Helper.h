#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

/**
 * @name Text Formatting Controls
 * @{
 */
/// @brief Resets all text formatting and color modifications to terminal defaults.
#define COLOR_RESET "\033[0m"

/// @brief Applies bold text weight.
#define COLOR_BOLD "\033[1m"

/// @brief Applies text underline formatting.
#define COLOR_UNDERLINE "\033[4m"
/** @} */

/**
 * @name Standard Foreground Colors (Darker/Normal)
 * @{
 */
/// @brief Standard black foreground text.
#define COLOR_BLACK "\033[30m"

/// @brief Standard red foreground text.
#define COLOR_RED "\033[31m"

/// @brief Standard green foreground text.
#define COLOR_GREEN "\033[32m"

/// @brief Standard yellow foreground text.
#define COLOR_YELLOW "\033[33m"

/// @brief Standard blue foreground text.
#define COLOR_BLUE "\033[34m"

/// @brief Standard magenta foreground text.
#define COLOR_MAGENTA "\033[35m"

/// @brief Standard cyan foreground text.
#define COLOR_CYAN "\033[36m"

/// @brief Standard white foreground text.
#define COLOR_WHITE "\033[37m"
/** @} */


/**
 * @name High-Intensity Foreground Colors (Bright)
 * @{
 */
/// @brief Bright black (dark gray) foreground text.
#define COLOR_BRIGHT_BLACK "\033[90m"

/// @brief High-intensity bright red foreground text.
#define COLOR_BRIGHT_RED "\033[91m"

/// @brief High-intensity bright green foreground text.
#define COLOR_BRIGHT_GREEN "\033[92m"

/// @brief High-intensity bright yellow foreground text.
#define COLOR_BRIGHT_YELLOW "\033[93m"

/// @brief High-intensity bright blue foreground text.
#define COLOR_BRIGHT_BLUE "\033[94m"

/// @brief High-intensity bright magenta foreground text.
#define COLOR_BRIGHT_MAGENTA "\033[95m"

/// @brief High-intensity bright cyan foreground text.
#define COLOR_BRIGHT_CYAN "\033[96m"

/// @brief High-intensity bright white foreground text.
#define COLOR_BRIGHT_WHITE "\033[97m"
/** @} */


/**
 * @name Bold Foreground Colors
 * @{
 */
/// @brief Bold red foreground text.
#define COLOR_BOLD_RED "\033[1;31m"

/// @brief Bold green foreground text.
#define COLOR_BOLD_GREEN "\033[1;32m"

/// @brief Bold yellow foreground text.
#define COLOR_BOLD_YELLOW "\033[1;33m"

/// @brief Bold blue foreground text.
#define COLOR_BOLD_BLUE "\033[1;34m"

/// @brief Bold magenta foreground text.
#define COLOR_BOLD_MAGENTA "\033[1;35m"

/// @brief Bold cyan foreground text.
#define COLOR_BOLD_CYAN "\033[1;36m"

/// @brief Bold white foreground text.
#define COLOR_BOLD_WHITE "\033[1;37m"
/** @} */


/**
 * @name Standard Background Colors
 * @{
 */
/// @brief Standard black background color.
#define COLOR_BG_BLACK "\033[40m"

/// @brief Standard red background color.
#define COLOR_BG_RED "\033[41m"

/// @brief Standard green background color.
#define COLOR_BG_GREEN "\033[42m"

/// @brief Standard yellow background color.
#define COLOR_BG_YELLOW "\033[43m"

/// @brief Standard blue background color.
#define COLOR_BG_BLUE "\033[44m"

/// @brief Standard magenta background color.
#define COLOR_BG_MAGENTA "\033[45m"

/// @brief Standard cyan background color.
#define COLOR_BG_CYAN "\033[46m"

/// @brief Standard white background color.
#define COLOR_BG_WHITE "\033[47m"
/** @} */

namespace winrt::CLauncher::Core
{
	class Helper
	{
	public:
		static inline std::string GET_TIMESTAMP()
		{
			auto now = std::chrono::system_clock::now();
			auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 100;
			auto time = std::chrono::system_clock::to_time_t(now);

			std::tm buffer {};

#ifdef _WIN32
			localtime_s(&buffer, &time);
#else
			localtime_r(&time, &buffer);
#endif
			return std::format(
				"{:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}.{:03d}",
				buffer.tm_year + 1900,
				buffer.tm_mon + 1,
				buffer.tm_mday,
				buffer.tm_hour,
				buffer.tm_min,
				buffer.tm_sec,
				milliseconds.count()
			);
		}

		static inline std::filesystem::path GET_ROOT_DIRECTORY()
		{
			return std::filesystem::current_path();
		}

		static inline std::string TRIM(const std::string& value)
		{
			constexpr std::string_view whitespace = " \t\r\n";
			const auto begin = value.find_first_not_of(whitespace);

			if (begin == std::string::npos)
			{
				return {};
			}

			const auto end = value.find_last_not_of(whitespace);

			return value.substr(begin, end - begin + 1);
		}

		static inline std::string TO_LOWER(std::string value)
		{
			std::transform(
				value.begin(),
				value.end(),
				value.begin(),
				[](unsigned char c)
				{
					return static_cast<char>(std::tolower(c));
				}
			);
		}

		static inline bool STARTS_WITH_CASE_SENSITIVE(const std::string& value, std::string_view prefix)
		{
			if (value.size() < prefix.size())
			{
				return false;
			}

			for (std::size_t i = 0; i < prefix.size(); ++i)
			{
				const auto a = static_cast<unsigned char>(value[i]);
				const auto b = static_cast<unsigned char>(prefix[i]);

				if (std::tolower(a) != std::tolower(b))
				{
					return false;
				}
			}

			return true;
		}

		static inline std::vector<std::string> SPLIT_LINES(const std::string& content)
		{
			std::vector<std::string> lines;
			std::istringstream stream(content);

			std::string line;

			while (std::getline(stream, line))
			{
				if (!line.empty() && line.back() == '\r')
				{
					line.pop_back();
				}

				lines.push_back(line);
			}

			return lines;
		}

		static inline bool TRY_PARSE_INT64(const std::string& value, std::int64_t& result)
		{
			try
			{
				std::size_t consumed = 0;
				const auto parsed = std::stoll(value, &consumed);

				if (consumed != value.size())
				{
					return false;
				}

				result = parsed;
				
				return true;
			}
			catch (...)
			{
				return false;
			}
		}

		static inline bool TRY_PARSE_INT(const std::string& value, int& result)
		{
			try
			{
				std::size_t consumed = 0;
				const auto parsed = std::stoll(value, &consumed);

				if (consumed != value.size())
				{
					return false;
				}

				result = parsed;

				return true;
			}
			catch (...)
			{
				return false;
			}
		}

		static inline bool TRY_PARSE_INT32_HEX(const std::string& value, std::uint32_t& result)
		{
			try
			{
				std::size_t consumed = 0;
				const auto parsed = std::stoull(value, &consumed, 16);

				if (consumed != value.size())
				{
					return false;
				}

				result = static_cast<std::uint32_t>(parsed);

				return true;
			}
			catch (...)
			{
				return false;
			}
		}

		static inline bool TRY_PARSE_FLOAT(const std::string& value, float& result)
		{
			try
			{
				std::size_t consumed = 0;
				const auto parsed = std::stof(value, &consumed);

				if (consumed != value.size())
				{
					return false;
				}

				result = parsed;

				return true;
			}
			catch (...)
			{
				return false;
			}
		}

		static inline std::vector<std::uint8_t> XOR( const std::vector<std::uint8_t>& data, const std::string& key)
		{
			if (data.empty() || key.empty())
			{
				return data;
			}

			const std::vector<std::uint8_t> bytes(key.begin(), key.end());
			std::vector<std::uint8_t> result(data.size());

			for (std::size_t i = 0; i < data.size(); ++i)
			{
				result[i] = static_cast<std::uint8_t>(data[i] ^ bytes[i % bytes.size()]);
			}

			return result;
		}

		static inline std::vector<std::uint8_t> XOR(std::string_view data, const std::string& key)
		{
			return XOR( std::vector<std::uint8_t>(data.begin(), data.end()), key);
		}
	};
}