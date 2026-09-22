#include "pch.h"
#include "Logs.h"

HANDLE hConsole = INVALID_HANDLE_VALUE;

namespace winrt::CLauncher::Core::Logging
{
	void Logs::AllocateConsole()
	{
		if (AllocConsole() || AttachConsole(ATTACH_PARENT_PROCESS))
		{
			hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
			SetConsoleOutputCP(CP_UTF8);

			if (hConsole != INVALID_HANDLE_VALUE)
			{
				DWORD dwMode = 0;

				if (GetConsoleMode(hConsole, &dwMode))
				{
					dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
					SetConsoleMode(hConsole, dwMode);
				}
			}

			FILE* stream = nullptr;

			freopen_s(&stream, "CONOUT$", "w", stdout);
			freopen_s(&stream, "CONOUT$", "w", stderr);
			freopen_s(&stream, "CONIN$", "r", stdin);

			std::cout.clear();
			std::cerr.clear();
			std::cin.clear();

			std::ios::sync_with_stdio(true);
		}
	}

	void Logs::write(SeverityLevel level, const char* file, uint32_t line, const char* function, const std::string_view message)
	{
		if ((static_cast<uint8_t>(ACTIVE_LOG_LEVEL) & static_cast<uint8_t>(level)) == 0)
		{
			return;
		}

		std::string filename = filesystem::path(file).filename().string();
		std::string timestamp = winrt::CLauncher::Core::Helper::GET_TIMESTAMP();
		std::string_view LEVEL_STRING = LEVEL_TO_STRING(level);
		std::string content;

		if (OUTPUT_TO_CONSOLE)
		{
			std::string_view LEVEL_COLOR = LEVEL_TO_COLOR(level);
			std::string BACKGROUND;

			switch (level)
			{
			case SeverityLevel::Warning:
			case SeverityLevel::Error:
			case SeverityLevel::Fatal:
				switch (level)
				{
				case SeverityLevel::Warning:
					BACKGROUND = COLOR_BG_YELLOW;
					break;
				case SeverityLevel::Error:
					BACKGROUND = COLOR_BG_RED;
					break;
				case SeverityLevel::Fatal:
					BACKGROUND = COLOR_BG_MAGENTA;
					break;
				}

				content = std::format(
					"{} [{}{}{}] [{}{}{}:{}{}{}] [{}{}(){}] : {}{}{}{}{}",
					timestamp,
					LEVEL_COLOR,
					LEVEL_STRING,
					COLOR_RESET,
					COLOR_BRIGHT_YELLOW,
					filename,
					COLOR_RESET,
					COLOR_BRIGHT_YELLOW,
					line,
					COLOR_RESET,
					COLOR_BRIGHT_YELLOW,
					function,
					COLOR_RESET,
					BACKGROUND,
					COLOR_BRIGHT_WHITE,
					message,
					COLOR_RESET,
					COLOR_RESET
				);
				break;
			case SeverityLevel::Verbose:
			case SeverityLevel::Trace:
			case SeverityLevel::Debug:
			case SeverityLevel::Info:
			case SeverityLevel::Http:
			default:
				content = std::format(
					"{} [{}{}{}] [{}{}{}:{}{}{}] [{}{}(){}] : {}{}{}",
					timestamp,
					LEVEL_COLOR,
					LEVEL_STRING,
					COLOR_RESET,
					COLOR_BRIGHT_YELLOW,
					filename,
					COLOR_RESET,
					COLOR_BRIGHT_YELLOW,
					line,
					COLOR_RESET,
					COLOR_BRIGHT_YELLOW,
					function,
					COLOR_RESET,
					COLOR_BRIGHT_WHITE,
					message,
					COLOR_RESET
				);
				break;
			}

			content += "\n";

			DWORD written;
			WriteConsoleA(hConsole, content.c_str(), (DWORD)content.length(), &written, NULL);
		}
		
		if (OUTPUT_TO_FILE)
		{
			content = std::format(
				"{} [{}] [{}:{}] [{}()] : {}",
				timestamp,
				LEVEL_STRING,
				filename,
				line,
				function,
				message
			);

			std::lock_guard<std::mutex> lock(MUTEX);

			try
			{
				if (!filesystem::exists(LOG_FOLDER))
				{
					filesystem::create_directories(LOG_FOLDER);
				}

				std::ofstream out(LOG_FOLDER + "\\" + LOG_FILENAME, std::ios::out | std::ios::app);

				if (out.is_open())
				{
					out << content << std::endl;
				}
			}
			catch (...)
			{
				// ignore exceptions
			}
		}
	}

	std::string_view Logs::LEVEL_TO_STRING(SeverityLevel level)
	{
		switch (level)
		{
		case SeverityLevel::Verbose:
			return "VERBOSE";
		case SeverityLevel::Trace:
			return "TRACE";
		case SeverityLevel::Debug:
			return "DEBUG";
		case SeverityLevel::Info:
			return "INFO";
		case SeverityLevel::Http:
			return "HTTP";
		case SeverityLevel::Warning:
			return "WARNING";
		case SeverityLevel::Error:
			return "ERROR";
		case SeverityLevel::Fatal:
			return "FATAL";
		default:
			return "UNKNOWN";
		}
	}

	std::string_view Logs::LEVEL_TO_COLOR(SeverityLevel level)
	{
		switch (level)
		{
		case SeverityLevel::Verbose:
			return COLOR_BRIGHT_BLACK;
		case SeverityLevel::Trace:
			return COLOR_WHITE;
		case SeverityLevel::Debug:
			return COLOR_CYAN;
		case SeverityLevel::Info:
			return COLOR_GREEN;
		case SeverityLevel::Http:
			return COLOR_CYAN;
		case SeverityLevel::Warning:
			return COLOR_YELLOW;
		case SeverityLevel::Error:
			return COLOR_RED;
		case SeverityLevel::Fatal:
			return COLOR_MAGENTA;
		default:
			return COLOR_RESET;
		}
	}
}
