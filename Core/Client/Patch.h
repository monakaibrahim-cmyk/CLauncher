#pragma once

#include "../Logging/Logs.h"
#include "../Helper.h"

#include <filesystem>
#include <format>
#include <string>
#include <windows.h>

enum CLIENT_PATCH_FAILURES
{
	PATCH_SUCCESS,
	PATCH_FAILED
};

namespace winrt::CLauncher::Core::Client
{
	struct PATCH_DETAILS
	{
		unsigned szVirtualAddress;
		const char* szHexBytes;
	};

	class Patch
	{
		static inline PATCH_DETAILS m_Patches[] =
		{
			{
				0x004DCCF0,						// lua_ScanDllStart
				"B8"
				"00000000"						// mov eax, 1
				"C3"							// ret
			},
			{
				0x004E5CB0,						// ScanDllStart
				"B8"
				"01000000"						// mov eax, 1
				"A3"
				"74B4B600"						// mov s_isScanDllFinished, eax
				"68"
				"E05C4E00"						// push CLauncher.dll
				"E8"
				"1C683800"						// call _loadddll
				"83C4"
				"04"							// add esp, 4
				"55"							// push ebp
				"8BEC"							// mov ebp, esp
				"E8"
				"A110F2FF"						// call 0x00406D70
				"E9"
				"045BF2FF"						// jmp 0x0040B7D8
				"CCCCCCCCCCCCCCCCCCCCCCCC"		// int3 (12 times)
				"434C61756E636865722E646C6C00"	// CLauncher.dll
			},
			{
				0x0040B7D0,						// StartAddress
				"E9"
				"DBA40D00"						// jmp 0x004E5CB0
				"909090"						// nop (3 times)
			}
		};

		static inline std::string FIND_GAME_EXECUTABLE()
		{
			static const char* FILENAME = "wow.exe";

			if (std::filesystem::exists(FILENAME))
			{
				return FILENAME;
			}

			return {};
		}

		static inline bool APPLY_CLIENT_PATCH_MODIFICATION(const char* path)
		{
			std::vector<char> image;

			if (!winrt::CLauncher::Core::Helper::READ_FILE(path, image))
			{
				return false;
			}

			if (!winrt::CLauncher::Core::Helper::APPLY_LAA(image))
			{
				SetLastError(ERROR_INVALID_DATA);

				return false;
			}

			for (auto& [szVirtualAddress, szHexBytes] : m_Patches)
			{
				unsigned offset = winrt::CLauncher::Core::Helper::VIRTUAL_ADDRESS_2_RAW_OFFSET(image.data(), szVirtualAddress);

				if (!offset)
				{
					SetLastError(ERROR_INVALID_ADDRESS);

					return false;
				}

				std::vector<char> data;

				if (!winrt::CLauncher::Core::Helper::CONVERT_HEX_STRING_2_BYTE_ARRAY(szHexBytes, data) || data.empty())
				{
					SetLastError(ERROR_BAD_ARGUMENTS);

					return false;
				}

				memcpy(&image[offset], data.data(), data.size());
			}

			return winrt::CLauncher::Core::Helper::WRITE_FILE(path, image);
		}

		[[nodiscard]] static inline CLIENT_PATCH_FAILURES MainTask()
		{
			std::filesystem::path APP = winrt::CLauncher::Core::Helper::GET_ROOT_DIRECTORY() / FIND_GAME_EXECUTABLE();

			if (APP.empty())
			{
				LOG_FATAL("World of Warcraft executable not found!");

				return PATCH_FAILED;
			}

			if (!APPLY_CLIENT_PATCH_MODIFICATION(APP.string().c_str()))
			{
				DWORD LAST_ERROR = GetLastError();
				std::string message;

				FormatMessageA(
					FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					nullptr,
					LAST_ERROR,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					reinterpret_cast<LPSTR>(&message),
					0,
					nullptr
				);

				LOG_FATAL("Can`t apply patch to {} ", FIND_GAME_EXECUTABLE());
				LOG_FATAL("{} {}", message, LAST_ERROR);

				return PATCH_FAILED;
			}

			return PATCH_SUCCESS;
		}
	};
}
