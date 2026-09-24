#pragma once

#include "../Globals.h"
#include "../Helper.h"
#include "../Logging/Logs.h"
#include "Payload.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include <curl/curl.h>

namespace winrt::CLauncher::Core::API
{
	enum class DOWNLOAD_STATUS : uint8_t
	{
		STATUS_IDLE,
		STATUS_FETCHING_MANIFEST,
		STATUS_CHECKING_FILES,
		STATUS_DOWNLOADING,
		STATUS_COMPLETED,
		STATUS_FAILED,
		STATUS_CANCELLED
	};

	using PROGRESS_CALLBACK = std::function<void(uint64_t bytesReceived, uint64_t totalBytes, double speedMBs, const std::string& currentFileName)>;
	using STATUS_CALLBACK = std::function<void(const std::string& statusMessage)>;
	using FILE_COMPLETE_CALLBACK = std::function<void(const std::string& fileName, bool success)>;

	class Downloader
	{
	public:
		Downloader();
		explicit Downloader(std::filesystem::path targetDirectory);
		~Downloader();

		void SET_TARGET_DIRECTORY(const std::filesystem::path& directory);
		[[nodiscard]] const std::filesystem::path& GET_TARGET_DIRECTORY() const;

		void SET_BASE_SERVER_URL(const std::string& url);
		[[nodiscard]] std::string GET_BASE_SERVER_URL() const;

		void SET_PROGRESS_CALLBACK(PROGRESS_CALLBACK callback);
		void SET_STATUS_CALLBACK(STATUS_CALLBACK callback);
		void SET_FILE_COMPLETE_CALLBACK(FILE_COMPLETE_CALLBACK callback);

		void CANCEL();
		[[nodiscard]] bool IS_CANCELLED() const;
		[[nodiscard]] DOWNLOAD_STATUS GET_STATUS() const;

		bool FETCH_CLIENT_MANIFEST(const std::string& serverUrl = "");

		bool CHECK_CLIENT_FILES(
			std::vector<CLIENT_MANIFEST_ENTRY>& outFilesToDownload,
			uint64_t& outTotalBytesToDownload);

		bool DOWNLOAD_CLIENT_FILES(
			const std::vector<CLIENT_MANIFEST_ENTRY>& filesToDownload,
			uint64_t totalBytesToDownload);

		bool SYNC_CLIENT_FILES();

		bool DOWNLOAD_FILE(
			const std::string& url,
			const std::filesystem::path& destinationPath,
			const std::string& expectedChecksum = "",
			uint64_t expectedSize = 0);

	private:
		struct CURL_PROGRESS_CONTEXT
		{
			Downloader* pDownloader{ nullptr };
			uint64_t szFileBytesPreviouslyDone{ 0 };
			uint64_t szOverallTotalBytes{ 0 };
		};

		static size_t WriteMemoryCallBack(void* contents, size_t size, size_t nmemb, void* userp);
		static size_t WriteFileCallBack(void* contents, size_t size, size_t nmemb, void* userp);
		static int CurlXferInfoCallBack(
			void* clientp,
			curl_off_t dltotal,
			curl_off_t dlnow,
			curl_off_t ultotal,
			curl_off_t ulnow);

		long PERFORM_GET(
			const std::string& url,
			void* writeData,
			size_t (*writeFunc)(void*, size_t, size_t, void*),
			CURL_PROGRESS_CONTEXT* progressCtx = nullptr);

		void UPDATE_PROGRESS(uint64_t currentFileReceived, uint64_t currentFileTotal, uint64_t overallReceived, uint64_t overallTotal);
		void REPORT_STATUS(const std::string& message);
		static bool EQUALS_IGNORE_CASE(std::string_view a, std::string_view b);

		std::filesystem::path szTargetDirectory;
		std::string szBaseServerUrl;
		std::atomic<bool> szIsCancelled{ false };
		std::atomic<DOWNLOAD_STATUS> szStatus{ DOWNLOAD_STATUS::STATUS_IDLE };
		bool szUseClientSubfolder{ false };

		PROGRESS_CALLBACK szProgressCallback;
		STATUS_CALLBACK szStatusCallback;
		FILE_COMPLETE_CALLBACK szFileCompleteCallback;

		uint64_t szOverallBytesReceived{ 0 };
		uint64_t szOverallTotalBytes{ 0 };
		std::string szCurrentFileName;

		std::chrono::steady_clock::time_point szLastSpeedTime{};
		uint64_t szLastSpeedBytes{ 0 };
		double szCurrentSpeedMBs{ 0.0 };
	};
}

