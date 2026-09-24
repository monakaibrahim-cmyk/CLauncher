#include "pch.h"
#include "Payload.h"

namespace winrt::CLauncher::Core::API
{
	size_t PayLoad::WriteCallBack(void* contents, size_t size, size_t nmemb, std::string* output)
	{
		size_t total_size = size * nmemb;
		output->append((char*)contents, total_size);
		return total_size;
	}

	std::string PayLoad::REQUEST_TOKEN()
	{
		CURL* curl = curl_easy_init();
		std::string response;

		if (curl)
		{
			try
			{
				nlohmann::json payload = {
					{"Application", "CLauncher"},
					{"Version", "1.0.0"},
					{"IpAddress", winrt::CLauncher::Core::Helper::FETCH_PUBLIC_USER_IP()},
					{"GUID", winrt::CLauncher::Core::Helper::GET_MACHINE_GUID()}
				};

				std::string dump = payload.dump();

				struct curl_slist* headers = NULL;
				headers = curl_slist_append(headers, "Content-Type: application/json");

				curl_easy_setopt(curl, CURLOPT_URL, (winrt::CLauncher::Core::Globals::REMOTE_AUTH_API + "/CLauncher").c_str());
				curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
				curl_easy_setopt(curl, CURLOPT_POSTFIELDS, dump.c_str());
				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallBack);
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

				CURLcode code = curl_easy_perform(curl);
				curl_easy_cleanup(curl);
				curl_slist_free_all(headers);

				if (code == CURLE_OK)
				{
					auto response_json = nlohmann::json::parse(response);

					if (!response_json.is_discarded() && response_json.contains("token"))
					{
						std::string token = response_json["token"].get<std::string>();
						LOG_DEBUG("Token: {}", token);

						return response_json["token"].get<std::string>();
					}
				}
			}
			catch (const std::exception& e)
			{
				LOG_HTTP("REQUEST_TOKEN Exception: {}", e.what());
			}
		}

#ifndef NDEBUG
		if (!winrt::CLauncher::Core::Globals::DEBUG_AUTH_TOKEN.empty())
		{
			LOG_DEBUG("Using DEBUG_AUTH_TOKEN for testing: {}", winrt::CLauncher::Core::Globals::DEBUG_AUTH_TOKEN);
			return winrt::CLauncher::Core::Globals::DEBUG_AUTH_TOKEN;
		}
#endif

		return {};
	}

	std::string PayLoad::GET_OR_REQUEST_TOKEN(bool forceRefresh)
	{
		std::lock_guard<std::mutex> lock(szTokenMutex);
		if (!forceRefresh && !szCachedToken.empty())
		{
			return szCachedToken;
		}

#ifndef NDEBUG
		if (!winrt::CLauncher::Core::Globals::DEBUG_AUTH_TOKEN.empty())
		{
			szCachedToken = winrt::CLauncher::Core::Globals::DEBUG_AUTH_TOKEN;
			return szCachedToken;
		}
#endif

		szCachedToken = REQUEST_TOKEN();
		return szCachedToken;
	}

	void PayLoad::CLEAR_TOKEN()
	{
		std::lock_guard<std::mutex> lock(szTokenMutex);
		szCachedToken.clear();
	}

	bool PayLoad::SEND_PAYLOAD(const std::string& username, const std::string& password)
	{
		std::string token = GET_OR_REQUEST_TOKEN();
		
		if (token.empty())
		{
			LOG_HTTP("Token is empty!");

			return false;
		}

		try
		{
			auto decoded = jwt::decode(token);
#ifndef NDEBUG
			auto verifier = jwt::verify().allow_algorithm(jwt::algorithm::rs256(winrt::CLauncher::Core::Globals::PUBLIC_KEY_PEM, "", "", "")).with_issuer("auth.yourdomain.com");
#else
			auto verifier = jwt::verify().allow_algorithm(jwt::algorithm::rs256(winrt::CLauncher::Core::Globals::PUBLIC_KEY_PEM, "", "", ""));
#endif
			verifier.verify(decoded);

			CURL* curl = curl_easy_init();
			std::string response;

			if (curl)
			{
				nlohmann::json body = {
					{"username", username},
					{"password", password}
				};
				std::string dump = body.dump();

				struct curl_slist* headers = NULL;
				std::string authorization = "Authorization: Bearer " + token;
				headers = curl_slist_append(headers, authorization.c_str());

				curl_easy_setopt(curl, CURLOPT_URL, (winrt::CLauncher::Core::Globals::REMOTE_AUTH_API + "/Login").c_str());
				curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
				curl_easy_setopt(curl, CURLOPT_POSTFIELDS, dump.c_str());
				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallBack);
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

				CURLcode code = curl_easy_perform(curl);
				curl_easy_cleanup(curl);
				curl_slist_free_all(headers);

				if (code == CURLE_OK)
				{
					auto response_json = nlohmann::json::parse(response);

					if (response_json.contains("success"))
					{
						if (response_json["success"].is_boolean())
						{
							return response_json["success"].get<bool>();
						}
						else if (response_json["success"].is_string())
						{
							return response_json["success"].get<std::string>() == "true";
						}
					}
				}
			}

			return false;
		}
		catch (const std::exception& e)
		{
			LOG_HTTP("Verification Failed {}", e.what());

			return false;
		}
	}
}
