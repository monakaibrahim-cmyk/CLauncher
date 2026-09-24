#pragma once

#include <windows.h>
#include <wininet.h>
#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#pragma comment(lib, "wininet.lib")

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
			auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
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

			return value;
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

		static inline std::string GET_MACHINE_GUID()
		{
			HKEY hKey;
			char value[64];
			DWORD length = sizeof(value);

			if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Cryptography", 0, KEY_READ | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS)
			{
				if (RegQueryValueExA(hKey, "MachineGuid", NULL, NULL, (LPBYTE)value, &length) == ERROR_SUCCESS)
				{
					RegCloseKey(hKey);

					return std::string(value);
				}

				RegCloseKey(hKey);
			}

			return "UNKNOWN-GUID";
		}

		static inline std::string FETCH_PUBLIC_USER_IP()
		{
			std::string hAddress = "127.0.0.1";
			HINTERNET hInternet = InternetOpenA("CLauncher", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);

			if (hInternet)
			{
				HINTERNET hConnect = InternetOpenUrlA(hInternet, "https://ipify.org", NULL, 0, INTERNET_FLAG_RELOAD, 0);
				
				if (hConnect)
				{
					char buffer[64];
					DWORD bytes = 0;

					if (InternetReadFile(hConnect, buffer, sizeof(buffer) - 1, &bytes) && bytes > 0)
					{
						buffer[bytes] = '\0';
						hAddress = std::string(buffer);
					}

					InternetCloseHandle(hConnect);
				}

				InternetCloseHandle(hInternet);
			}

			return hAddress;
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
				const auto parsed = std::stoi(value, &consumed);

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

		static inline bool READ_FILE(const char* path, std::vector<char>& content)
		{
			std::ifstream file(path, std::ios::binary);
			
			if (!file)
			{
				return false;
			}

			content.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());

			return true;
		}

		static inline bool WRITE_FILE(const char* path, const std::vector<char>& content)
		{
			std::ofstream file(path, std::ios::binary | std::ios::trunc);

			if (!file)
			{
				return false;
			}

			file.write(content.data(), content.size());

			return true;
		}

		static inline unsigned VIRTUAL_ADDRESS_2_RAW_OFFSET(char* image, unsigned address)
		{
			auto* dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(image);
			auto* ntHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(&image[dosHeader->e_lfanew]);
			auto* header = IMAGE_FIRST_SECTION(ntHeaders);

			for (size_t i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++)
			{
				DWORD begin = static_cast<DWORD>(ntHeaders->OptionalHeader.ImageBase) + header->VirtualAddress;
				DWORD end = begin + header->Misc.VirtualSize;

				if (address >= begin && address < end)
				{
					return address - begin + header->PointerToRawData;
				}
					
				header++;
			}

			return 0;
		}

		static inline bool CONVERT_HEX_STRING_2_BYTE_ARRAY(const char* hex, std::vector<char>& result)
		{
			size_t hexLen = strlen(hex);
			if (hexLen % 2 != 0)
			{
				return false;
			}

			result.clear();
			result.reserve(hexLen / 2);

			for (size_t i = 0; i < hexLen; i += 2)
			{
				int v = 0;

				if (std::from_chars(&hex[i], &hex[i + 2], v, 0x10).ec == std::errc{})
				{
					result.push_back(static_cast<char>(v & 0xFF));
				}
				else
				{
					return false;
				}
			}

			return result.size() == (hexLen / 2);
		}

		static inline bool APPLY_LAA(std::vector<char>& image)
		{
			if (image.size() < sizeof(IMAGE_DOS_HEADER))
			{
				return false;
			}
				
			auto* dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(image.data());

			if (dosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS) > image.size())
			{
				return false;
			}
				
			auto* ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(&image[dosHeader->e_lfanew]);
			ntHeaders->FileHeader.Characteristics |= IMAGE_FILE_LARGE_ADDRESS_AWARE;

			return true;
		}

		static inline std::string CALCULATE_MD5(const std::filesystem::path& filePath)
		{
			if (!std::filesystem::exists(filePath))
			{
				return {};
			}

			std::ifstream file(filePath, std::ios::binary);
			if (!file.is_open())
			{
				return {};
			}

			EVP_MD_CTX* context = EVP_MD_CTX_new();
			if (!context)
			{
				return {};
			}

			if (EVP_DigestInit_ex(context, EVP_md5(), nullptr) != 1)
			{
				EVP_MD_CTX_free(context);
				return {};
			}

			constexpr std::size_t BUFFER_SIZE = 1024ull * 1024ull;
			std::vector<unsigned char> buffer(BUFFER_SIZE);

			while (true)
			{
				file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
				const std::streamsize bytesRead = file.gcount();

				if (bytesRead > 0)
				{
					if (EVP_DigestUpdate(context, buffer.data(), static_cast<std::size_t>(bytesRead)) != 1)
					{
						EVP_MD_CTX_free(context);
						return {};
					}
				}

				if (file.eof())
				{
					break;
				}

				if (file.fail())
				{
					EVP_MD_CTX_free(context);
					return {};
				}
			}

			unsigned char digest[EVP_MAX_MD_SIZE];
			unsigned int digestLength = 0;

			if (EVP_DigestFinal_ex(context, digest, &digestLength) != 1)
			{
				EVP_MD_CTX_free(context);
				return {};
			}

			EVP_MD_CTX_free(context);

			std::ostringstream result;
			for (unsigned int i = 0; i < digestLength; ++i)
			{
				result << std::hex << std::nouppercase << std::setw(2) << std::setfill('0') << static_cast<int>(digest[i]);
			}

			return result.str();
		}

		static inline std::string ENCODE_URL_PATH(const std::string& path)
		{
			std::string result;
			result.reserve(path.size() + 16);

			for (char c : path)
			{
				if (c == ' ')
				{
					result += "%20";
				}
				else
				{
					result += c;
				}
			}

			return result;
		}

		static inline std::string JOIN_URL(const std::string& base, const std::string& path)
		{
			if (base.empty())
			{
				return ENCODE_URL_PATH(path);
			}
			if (path.empty())
			{
				return base;
			}

			std::string encodedPath = ENCODE_URL_PATH(path);
			std::string result = base;
			bool baseHasSlash = (!result.empty() && (result.back() == '/' || result.back() == '\\'));
			bool pathHasSlash = (!encodedPath.empty() && (encodedPath.front() == '/' || encodedPath.front() == '\\'));

			if (baseHasSlash && pathHasSlash)
			{
				result.pop_back();
				result += encodedPath;
			}
			else if (!baseHasSlash && !pathHasSlash)
			{
				result += '/';
				result += encodedPath;
			}
			else
			{
				result += encodedPath;
			}

			std::replace(result.begin(), result.end(), '\\', '/');
			return result;
		}
	};

	class LoginSaveCrypto
	{
	public:
		static inline constexpr int KEY_SIZE = 32;
		static inline constexpr int IV_SIZE = 12;
		static inline constexpr int TAG_SIZE = 16;
		static inline unsigned char KEY[32] = "0123456789012345678901234567890";

		static bool DECRYPT_DATA(
			const unsigned char* chiper,
			int cipher_length,
			const unsigned char* key,
			const unsigned char* iv,
			const unsigned char* tag,
			unsigned char* plain,
			int& plain_length)
		{
			EVP_CIPHER_CTX* context = EVP_CIPHER_CTX_new();
			if (!context)
				return false;

			int length = 0;
			bool success = false;
			do
			{
				if (EVP_DecryptInit_ex(context, EVP_aes_256_gcm(), NULL, NULL, NULL) != 1)
				{
					break;
				}
					
				if (EVP_CIPHER_CTX_ctrl(context, EVP_CTRL_GCM_SET_IVLEN, IV_SIZE, NULL) != 1)
				{
					break;
				}
				
				if (EVP_DecryptInit_ex(context, NULL, NULL, key, iv) != 1)
				{
					break;
				}
				
				if (EVP_DecryptUpdate(context, plain, &length, chiper, cipher_length) != 1)
				{
					break;
				}
				
				plain_length = length;

				if (EVP_CIPHER_CTX_ctrl(context, EVP_CTRL_GCM_SET_TAG, TAG_SIZE, (void*)tag) != 1)
				{
					break;
				}
				

				if (EVP_DecryptFinal_ex(context, plain + length, &length) > 0)
				{
					plain_length += length;
					success = true;
				}
			} while (0);

			EVP_CIPHER_CTX_free(context);

			return success;
		}

		static inline bool ENCRYPT_DATA(
			const unsigned char* plain,
			int plain_length,
			const unsigned char* key,
			const unsigned char* iv,
			unsigned char* chiper,
			unsigned char* tag
		)
		{
			EVP_CIPHER_CTX* context = EVP_CIPHER_CTX_new();
			if (!context)
			{
				return false;
			}

			int length = 0;
			int chiper_length = 0;
			bool success = false;

			do
			{
				if (EVP_EncryptInit_ex(context, EVP_aes_256_gcm(), NULL, NULL, NULL) != 1)
				{
					break;
				}

				if (EVP_CIPHER_CTX_ctrl(context, EVP_CTRL_GCM_SET_IVLEN, IV_SIZE, NULL) != 1)
				{
					break;
				}

				if (EVP_EncryptInit_ex(context, NULL, NULL, key, iv) != 1)
				{
					break;
				}

				if (EVP_EncryptUpdate(context, chiper, &length, plain, plain_length) != 1)
				{
					break;
				}

				chiper_length = length;

				if (EVP_EncryptFinal_ex(context, chiper + length, &length) != 1)
				{
					break;
				}

				chiper_length += length;

				if (EVP_CIPHER_CTX_ctrl(context, EVP_CTRL_GCM_GET_TAG, TAG_SIZE, tag) != 1)
				{
					break;
				}

				success = true;
			}
			while (0);

			EVP_CIPHER_CTX_free(context);

			return success;
		}
	};
}