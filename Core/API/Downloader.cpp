#include "pch.h"
#include "Downloader.h"

namespace winrt::CLauncher::Core::API
{
	Downloader::Downloader()
		: szTargetDirectory(winrt::CLauncher::Core::Helper::GET_ROOT_DIRECTORY())
		, szBaseServerUrl(winrt::CLauncher::Core::Globals::PATCH_REMOTE_BASE_URL())
	{
	}

	Downloader::Downloader(std::filesystem::path targetDirectory)
		: szTargetDirectory(std::move(targetDirectory))
		, szBaseServerUrl(winrt::CLauncher::Core::Globals::PATCH_REMOTE_BASE_URL())
	{
	}

	Downloader::~Downloader()
	{
		CANCEL();
	}

	void Downloader::SET_TARGET_DIRECTORY(const std::filesystem::path& directory)
	{
		szTargetDirectory = directory;
	}

	const std::filesystem::path& Downloader::GET_TARGET_DIRECTORY() const
	{
		return szTargetDirectory;
	}

	void Downloader::SET_BASE_SERVER_URL(const std::string& url)
	{
		szBaseServerUrl = url;
	}

	std::string Downloader::GET_BASE_SERVER_URL() const
	{
		if (!szBaseServerUrl.empty())
		{
			return szBaseServerUrl;
		}

		return winrt::CLauncher::Core::Globals::PATCH_REMOTE_BASE_URL();
	}

	void Downloader::SET_PROGRESS_CALLBACK(PROGRESS_CALLBACK callback)
	{
		szProgressCallback = std::move(callback);
	}

	void Downloader::SET_STATUS_CALLBACK(STATUS_CALLBACK callback)
	{
		szStatusCallback = std::move(callback);
	}

	void Downloader::SET_FILE_COMPLETE_CALLBACK(FILE_COMPLETE_CALLBACK callback)
	{
		szFileCompleteCallback = std::move(callback);
	}

	void Downloader::CANCEL()
	{
		szIsCancelled = true;
		szStatus = DOWNLOAD_STATUS::STATUS_CANCELLED;
	}

	bool Downloader::IS_CANCELLED() const
	{
		return szIsCancelled.load();
	}

	DOWNLOAD_STATUS Downloader::GET_STATUS() const
	{
		return szStatus.load();
	}

	size_t Downloader::WriteMemoryCallBack(void* contents, size_t size, size_t nmemb, void* userp)
	{
		const size_t total_size = size * nmemb;
		auto* output = static_cast<std::string*>(userp);
		output->append(static_cast<const char*>(contents), total_size);
		return total_size;
	}

	size_t Downloader::WriteFileCallBack(void* contents, size_t size, size_t nmemb, void* userp)
	{
		const size_t total_size = size * nmemb;
		auto* stream = static_cast<std::ofstream*>(userp);
		stream->write(static_cast<const char*>(contents), static_cast<std::streamsize>(total_size));
		return total_size;
	}

	int Downloader::CurlXferInfoCallBack(
		void* clientp,
		curl_off_t dltotal,
		curl_off_t dlnow,
		curl_off_t /*ultotal*/,
		curl_off_t /*ulnow*/)
	{
		auto* ctx = static_cast<CURL_PROGRESS_CONTEXT*>(clientp);
		if (!ctx || !ctx->pDownloader)
		{
			return 0;
		}

		if (ctx->pDownloader->IS_CANCELLED())
		{
			return 1;
		}

		const uint64_t currentFileReceived = static_cast<uint64_t>(dlnow > 0 ? dlnow : 0);
		const uint64_t currentFileTotal = static_cast<uint64_t>(dltotal > 0 ? dltotal : 0);
		const uint64_t overallReceived = ctx->szFileBytesPreviouslyDone + currentFileReceived;
		const uint64_t overallTotal = ctx->szOverallTotalBytes > 0 ? ctx->szOverallTotalBytes : currentFileTotal;

		ctx->pDownloader->UPDATE_PROGRESS(currentFileReceived, currentFileTotal, overallReceived, overallTotal);
		return 0;
	}

	void Downloader::UPDATE_PROGRESS(
		uint64_t /*currentFileReceived*/,
		uint64_t /*currentFileTotal*/,
		uint64_t overallReceived,
		uint64_t overallTotal)
	{
		const auto now = std::chrono::steady_clock::now();
		const double elapsed = std::chrono::duration<double>(now - szLastSpeedTime).count();

		if (elapsed >= 0.5)
		{
			if (overallReceived >= szLastSpeedBytes)
			{
				const double bytesDelta = static_cast<double>(overallReceived - szLastSpeedBytes);
				const double bytesPerSec = bytesDelta / elapsed;
				szCurrentSpeedMBs = bytesPerSec / (1024.0 * 1024.0);
			}

			szLastSpeedBytes = overallReceived;
			szLastSpeedTime = now;
		}

		if (szProgressCallback)
		{
			szProgressCallback(overallReceived, overallTotal, szCurrentSpeedMBs, szCurrentFileName);
		}
	}

	void Downloader::REPORT_STATUS(const std::string& message)
	{
		LOG_INFO("Downloader: {}", message);
		if (szStatusCallback)
		{
			szStatusCallback(message);
		}
	}

	bool Downloader::EQUALS_IGNORE_CASE(std::string_view a, std::string_view b)
	{
		if (a.size() != b.size())
		{
			return false;
		}

		for (size_t i = 0; i < a.size(); ++i)
		{
			if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i])))
			{
				return false;
			}
		}

		return true;
	}

	long Downloader::PERFORM_GET(
		const std::string& url,
		void* writeData,
		size_t (*writeFunc)(void*, size_t, size_t, void*),
		CURL_PROGRESS_CONTEXT* progressCtx)
	{
		int attempts = 0;
		long httpCode = 0;

		while (attempts < 2)
		{
			attempts++;
			CURL* curl = curl_easy_init();
			if (!curl)
			{
				LOG_ERROR("Failed to initialize curl handle.");
				return -1;
			}

			std::string token = PayLoad::GET_OR_REQUEST_TOKEN();
			struct curl_slist* headers = NULL;

			if (!token.empty())
			{
				std::string authorization = "Authorization: Bearer " + token;
				headers = curl_slist_append(headers, authorization.c_str());
			}
			else
			{
				LOG_WARNING("Performing GET without authorization token for: {}", url);
			}

			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunc);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, writeData);
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
			curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);

			if (progressCtx)
			{
				curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
				curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, CurlXferInfoCallBack);
				curl_easy_setopt(curl, CURLOPT_XFERINFODATA, progressCtx);
			}
			else
			{
				curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
			}

			CURLcode code = curl_easy_perform(curl);
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

			curl_easy_cleanup(curl);
			if (headers)
			{
				curl_slist_free_all(headers);
			}

			if (code == CURLE_ABORTED_BY_CALLBACK)
			{
				LOG_INFO("Curl request aborted by callback for: {}", url);
				return 0;
			}

			if (code != CURLE_OK)
			{
				LOG_ERROR("Curl perform error ({}) for URL: {}", static_cast<int>(code), url);
				return -1;
			}

			if (httpCode == 401 && attempts == 1)
			{
				LOG_WARNING("HTTP 401 Unauthorized for URL: {}. Refreshing token and retrying...", url);
				PayLoad::GET_OR_REQUEST_TOKEN(true);
				continue;
			}

			return httpCode;
		}

		return httpCode;
	}

	bool Downloader::FETCH_CLIENT_MANIFEST(const std::string& serverUrl)
	{
		szStatus = DOWNLOAD_STATUS::STATUS_FETCHING_MANIFEST;
		szIsCancelled = false;

		std::string baseUrl = serverUrl.empty() ? GET_BASE_SERVER_URL() : serverUrl;
		if (baseUrl.empty())
		{
			REPORT_STATUS("No server URL configured for client manifest.");
			szStatus = DOWNLOAD_STATUS::STATUS_FAILED;
			return false;
		}

		if (winrt::CLauncher::Core::Globals::CURRENT_MFIL.szClientPartial.empty())
		{
			REPORT_STATUS("szClientPartial is empty in CURRENT_MFIL.");
			szStatus = DOWNLOAD_STATUS::STATUS_FAILED;
			return false;
		}

		const std::string manifestUrl = Helper::JOIN_URL(baseUrl, winrt::CLauncher::Core::Globals::CURRENT_MFIL.szClientPartial);
		REPORT_STATUS(std::format("Fetching client manifest: {}", manifestUrl));

		std::string manifestContent;
		long httpCode = PERFORM_GET(manifestUrl, &manifestContent, WriteMemoryCallBack, nullptr);

		if (httpCode != 200 || manifestContent.empty())
		{
			REPORT_STATUS(std::format("Failed to download client manifest. HTTP Code: {}", httpCode));
			szStatus = DOWNLOAD_STATUS::STATUS_FAILED;
			return false;
		}

		try
		{
			winrt::CLauncher::Core::Globals::CURRENT_CLIENT_MANIFEST = CLIENT_MANIFEST::Parse(manifestContent);
			REPORT_STATUS(std::format(
				"Client manifest parsed successfully: {} file(s) recorded.",
				winrt::CLauncher::Core::Globals::CURRENT_CLIENT_MANIFEST.szFiles.size()));
			szStatus = DOWNLOAD_STATUS::STATUS_IDLE;
			return true;
		}
		catch (const std::exception& e)
		{
			REPORT_STATUS(std::format("Failed to parse client manifest: {}", e.what()));
			szStatus = DOWNLOAD_STATUS::STATUS_FAILED;
			return false;
		}
	}

	bool Downloader::CHECK_CLIENT_FILES(
		std::vector<CLIENT_MANIFEST_ENTRY>& outFilesToDownload,
		uint64_t& outTotalBytesToDownload)
	{
		szStatus = DOWNLOAD_STATUS::STATUS_CHECKING_FILES;
		outFilesToDownload.clear();
		outTotalBytesToDownload = 0;

		const auto& manifest = winrt::CLauncher::Core::Globals::CURRENT_CLIENT_MANIFEST;
		if (manifest.szFiles.empty())
		{
			REPORT_STATUS("Client manifest contains no files to verify.");
			szStatus = DOWNLOAD_STATUS::STATUS_IDLE;
			return true;
		}

		REPORT_STATUS(std::format("Scanning {} client file(s)...", manifest.szFiles.size()));

		for (const auto& entry : manifest.szFiles)
		{
			if (szIsCancelled)
			{
				szStatus = DOWNLOAD_STATUS::STATUS_CANCELLED;
				return false;
			}

			const std::filesystem::path localPath = szTargetDirectory / entry.szFile;

			if (!std::filesystem::exists(localPath))
			{
				LOG_DEBUG("Missing client file: {}", entry.szFile);
				outFilesToDownload.push_back(entry);
				outTotalBytesToDownload += (entry.szSize > 0 ? static_cast<uint64_t>(entry.szSize) : 0);
				continue;
			}

			std::error_code ec;
			const auto localSize = std::filesystem::file_size(localPath, ec);
			if (ec || static_cast<std::int64_t>(localSize) != entry.szSize)
			{
				LOG_DEBUG("Size mismatch for {}: local {} vs remote {}", entry.szFile, localSize, entry.szSize);
				outFilesToDownload.push_back(entry);
				outTotalBytesToDownload += (entry.szSize > 0 ? static_cast<uint64_t>(entry.szSize) : 0);
				continue;
			}

			if (!entry.szChecksum.empty())
			{
				const std::string localMd5 = Helper::CALCULATE_MD5(localPath);
				if (!EQUALS_IGNORE_CASE(localMd5, entry.szChecksum))
				{
					LOG_DEBUG("Checksum mismatch for {}: local {} vs remote {}", entry.szFile, localMd5, entry.szChecksum);
					outFilesToDownload.push_back(entry);
					outTotalBytesToDownload += (entry.szSize > 0 ? static_cast<uint64_t>(entry.szSize) : 0);
					continue;
				}
			}
		}

		REPORT_STATUS(std::format(
			"File scan complete. {} of {} file(s) require download ({:.2f} MB).",
			outFilesToDownload.size(),
			manifest.szFiles.size(),
			static_cast<double>(outTotalBytesToDownload) / (1024.0 * 1024.0)));

		szStatus = DOWNLOAD_STATUS::STATUS_IDLE;
		return true;
	}

	bool Downloader::DOWNLOAD_CLIENT_FILES(
		const std::vector<CLIENT_MANIFEST_ENTRY>& filesToDownload,
		uint64_t totalBytesToDownload)
	{
		if (filesToDownload.empty())
		{
			REPORT_STATUS("No files to download.");
			szStatus = DOWNLOAD_STATUS::STATUS_COMPLETED;
			return true;
		}

		szStatus = DOWNLOAD_STATUS::STATUS_DOWNLOADING;
		szIsCancelled = false;

		szOverallTotalBytes = totalBytesToDownload;
		szOverallBytesReceived = 0;
		szLastSpeedBytes = 0;
		szLastSpeedTime = std::chrono::steady_clock::now();
		szCurrentSpeedMBs = 0.0;

		const std::string baseUrl = GET_BASE_SERVER_URL();
		if (baseUrl.empty())
		{
			REPORT_STATUS("No base server URL available for file downloads.");
			szStatus = DOWNLOAD_STATUS::STATUS_FAILED;
			return false;
		}

		const auto& manifest = winrt::CLauncher::Core::Globals::CURRENT_CLIENT_MANIFEST;

		for (const auto& entry : filesToDownload)
		{
			if (szIsCancelled)
			{
				szStatus = DOWNLOAD_STATUS::STATUS_CANCELLED;
				REPORT_STATUS("Download cancelled by user.");
				return false;
			}

			szCurrentFileName = entry.szFile;
			REPORT_STATUS(std::format("Downloading: {}", entry.szFile));

			std::string subpath = entry.szPath.empty() ? manifest.szServerPath : entry.szPath;
			if (szUseClientSubfolder && subpath.find("Client") == std::string::npos && subpath.find("client") == std::string::npos)
			{
				subpath = Helper::JOIN_URL(subpath, "Client");
			}

			std::string fileUrl = Helper::JOIN_URL(baseUrl, Helper::JOIN_URL(subpath, entry.szFile));
			const std::filesystem::path localPath = szTargetDirectory / entry.szFile;
			const std::filesystem::path tempPath = localPath.string() + ".tmp";

			std::error_code ec;
			std::filesystem::create_directories(localPath.parent_path(), ec);

			std::ofstream fileStream(tempPath, std::ios::binary | std::ios::trunc);
			if (!fileStream.is_open())
			{
				REPORT_STATUS(std::format("Failed to open destination file for writing: {}", tempPath.string()));
				szStatus = DOWNLOAD_STATUS::STATUS_FAILED;
				if (szFileCompleteCallback)
				{
					szFileCompleteCallback(entry.szFile, false);
				}
				return false;
			}

			CURL_PROGRESS_CONTEXT progressCtx{ this, szOverallBytesReceived, szOverallTotalBytes };
			long httpCode = PERFORM_GET(fileUrl, &fileStream, WriteFileCallBack, &progressCtx);
			fileStream.close();

			// If 404, retry with "Client" subfolder if not already present
			if (httpCode == 404 && !szUseClientSubfolder && subpath.find("Client") == std::string::npos && subpath.find("client") == std::string::npos)
			{
				const std::string altSubpath = Helper::JOIN_URL(subpath, "Client");
				const std::string altFileUrl = Helper::JOIN_URL(baseUrl, Helper::JOIN_URL(altSubpath, entry.szFile));

				fileStream.open(tempPath, std::ios::binary | std::ios::trunc);
				if (fileStream.is_open())
				{
					LOG_INFO("HTTP 404 on {}, retrying with client subfolder: {}", fileUrl, altFileUrl);
					httpCode = PERFORM_GET(altFileUrl, &fileStream, WriteFileCallBack, &progressCtx);
					fileStream.close();

					if (httpCode == 200)
					{
						szUseClientSubfolder = true;
						fileUrl = altFileUrl;
					}
				}
			}

			if (szIsCancelled)
			{
				std::filesystem::remove(tempPath, ec);
				szStatus = DOWNLOAD_STATUS::STATUS_CANCELLED;
				REPORT_STATUS("Download cancelled.");
				return false;
			}

			if (httpCode != 200)
			{
				std::filesystem::remove(tempPath, ec);
				REPORT_STATUS(std::format("Failed to download {} (HTTP {})", entry.szFile, httpCode));
				szStatus = DOWNLOAD_STATUS::STATUS_FAILED;
				if (szFileCompleteCallback)
				{
					szFileCompleteCallback(entry.szFile, false);
				}
				return false;
			}

			if (!entry.szChecksum.empty())
			{
				const std::string downloadedMd5 = Helper::CALCULATE_MD5(tempPath);
				if (!EQUALS_IGNORE_CASE(downloadedMd5, entry.szChecksum))
				{
					std::filesystem::remove(tempPath, ec);
					REPORT_STATUS(std::format(
						"Checksum verification failed for {}: expected {} got {}",
						entry.szFile,
						entry.szChecksum,
						downloadedMd5));
					szStatus = DOWNLOAD_STATUS::STATUS_FAILED;
					if (szFileCompleteCallback)
					{
						szFileCompleteCallback(entry.szFile, false);
					}
					return false;
				}
			}

			std::filesystem::rename(tempPath, localPath, ec);
			if (ec)
			{
				std::filesystem::copy_file(tempPath, localPath, std::filesystem::copy_options::overwrite_existing, ec);
				std::filesystem::remove(tempPath, ec);
			}

			szOverallBytesReceived += (entry.szSize > 0 ? static_cast<uint64_t>(entry.szSize) : 0);

			if (szFileCompleteCallback)
			{
				szFileCompleteCallback(entry.szFile, true);
			}
		}

		szStatus = DOWNLOAD_STATUS::STATUS_COMPLETED;
		REPORT_STATUS("All client files downloaded and verified successfully.");
		return true;
	}

	bool Downloader::SYNC_CLIENT_FILES()
	{
		if (!FETCH_CLIENT_MANIFEST())
		{
			return false;
		}

		std::vector<CLIENT_MANIFEST_ENTRY> filesToDownload;
		uint64_t totalBytes = 0;

		if (!CHECK_CLIENT_FILES(filesToDownload, totalBytes))
		{
			return false;
		}

		if (filesToDownload.empty())
		{
			REPORT_STATUS("All client files are already up to date.");
			szStatus = DOWNLOAD_STATUS::STATUS_COMPLETED;
			return true;
		}

		return DOWNLOAD_CLIENT_FILES(filesToDownload, totalBytes);
	}

	bool Downloader::DOWNLOAD_FILE(
		const std::string& url,
		const std::filesystem::path& destinationPath,
		const std::string& expectedChecksum,
		uint64_t expectedSize)
	{
		szIsCancelled = false;
		szCurrentFileName = destinationPath.filename().string();
		REPORT_STATUS(std::format("Downloading single file: {}", destinationPath.filename().string()));

		const std::filesystem::path tempPath = destinationPath.string() + ".tmp";
		std::error_code ec;
		std::filesystem::create_directories(destinationPath.parent_path(), ec);

		std::ofstream fileStream(tempPath, std::ios::binary | std::ios::trunc);
		if (!fileStream.is_open())
		{
			REPORT_STATUS(std::format("Failed to open file: {}", tempPath.string()));
			return false;
		}

		CURL_PROGRESS_CONTEXT progressCtx{ this, 0, expectedSize };
		long httpCode = PERFORM_GET(url, &fileStream, WriteFileCallBack, expectedSize > 0 ? &progressCtx : nullptr);
		fileStream.close();

		if (szIsCancelled)
		{
			std::filesystem::remove(tempPath, ec);
			return false;
		}

		if (httpCode != 200)
		{
			std::filesystem::remove(tempPath, ec);
			REPORT_STATUS(std::format("Failed to download file from {} (HTTP {})", url, httpCode));
			return false;
		}

		if (!expectedChecksum.empty())
		{
			const std::string calculated = Helper::CALCULATE_MD5(tempPath);
			if (!EQUALS_IGNORE_CASE(calculated, expectedChecksum))
			{
				std::filesystem::remove(tempPath, ec);
				REPORT_STATUS(std::format(
					"Checksum mismatch for {}: expected {} got {}",
					destinationPath.filename().string(),
					expectedChecksum,
					calculated));
				return false;
			}
		}

		std::filesystem::rename(tempPath, destinationPath, ec);
		if (ec)
		{
			std::filesystem::copy_file(tempPath, destinationPath, std::filesystem::copy_options::overwrite_existing, ec);
			std::filesystem::remove(tempPath, ec);
		}

		return true;
	}
}

