#pragma once

#include "../Globals.h"
#include "../Logging/Logs.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>

#include <curl/curl.h>
#include <jwt-cpp/traits/nlohmann-json/defaults.h>
#include <nlohmann/json.hpp>

#pragma comment(lib, "crypt32.lib")

namespace winrt::CLauncher::Core::API
{
	class PayLoad
	{
	public:
		static size_t WriteCallBack(void* contents, size_t size, size_t nmemb, std::string* output);
		static std::string REQUEST_TOKEN();
		[[nodiscard]] static bool SEND_PAYLOAD(const std::string& username, const std::string& password);
	};
}
