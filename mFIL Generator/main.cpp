// mFIL Generator.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <openssl/evp.h>

#pragma comment(lib, "libcrypto.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Crypt32.lib")

namespace filesystem = std::filesystem;

class MfilServer
{
public:
	std::string name;
	std::string location;

	MfilServer(std::string name_, std::string location_) : name(std::move(name_)), location(std::move(location_))
	{
		//
	}
};

class MfilGenerator
{
public:
	int version = 1;
	std::vector<MfilServer> servers;
	std::string manifestPartial;
	std::string clientPartial;

	void AddServer(const std::string& name, const std::string& location)
	{
		servers.emplace_back(name, location);
	}

	void SetClientPartial(const std::string& buildNumber, const std::string& md5Hash)
	{
		std::string upperHash = md5Hash;

		std::transform(
			upperHash.begin(),
			upperHash.end(),
			upperHash.begin(),
			[](unsigned char c)
			{
				return static_cast<char>(std::toupper(c));
			});

		clientPartial = "wow-client-" + buildNumber + "-" + upperHash + ".mfil";
	}

	void SetManifestPartialFromHash(const std::string& buildNumber, const std::string& md5Hash)
	{
		std::string upperHash = md5Hash;

		std::transform(
			upperHash.begin(),
			upperHash.end(),
			upperHash.begin(),
			[](unsigned char c)
			{
				return static_cast<char>(std::toupper(c));
			}
		);

		manifestPartial = "wow-" + buildNumber + "-" + upperHash + ".mfil";
	}

	[[nodiscard]]
	std::string Generate() const
	{
		std::ostringstream output;

		output << "version=" << version << std::endl;

		for (const auto& server : servers)
		{
			output << "server=" << server.name << std::endl;
			output << "\tlocation=" << server.location << std::endl;
		}

		if (!manifestPartial.empty())
		{
			output << "manifest_partial=" << manifestPartial << std::endl;
		}

		if (!clientPartial.empty())
		{
			output << "client_partial=" << clientPartial << std::endl;
		}

		return output.str();
	}

	void SaveToFile(const filesystem::path& filePath) const
	{
		std::ofstream file(filePath, std::ios::binary);

		if (!file)
		{
			throw std::runtime_error("Unable to open output file: " + filePath.string());
		}

		const std::string content = Generate();

		file.write(content.data(), static_cast<std::streamsize>(content.size()));

		if (!file)
		{
			throw std::runtime_error("Failed to write output file: " + filePath.string());
		}
	}
};

std::string ToLower(std::string value)
{
	std::transform(
		value.begin(),
		value.end(),
		value.begin(),
		[](unsigned char c)
		{
			return static_cast<char>(
				std::tolower(c));
		}
	);

	return value;
}

std::string ToUpper(std::string value)
{
	std::transform(
		value.begin(),
		value.end(),
		value.begin(),
		[](unsigned char c)
		{
			return static_cast<char>(
				std::toupper(c));
		}
	);

	return value;
}

std::string TrimQuotesAndSpaces(std::string input)
{
	auto isTrimCharacter = [](unsigned char c)
	{
		return c == '"' || c == '\'' || std::isspace(c);
	};

	while (!input.empty() && isTrimCharacter(static_cast<unsigned char>(input.front())))
	{
		input.erase(input.begin());
	}

	while (!input.empty() && isTrimCharacter(static_cast<unsigned char>(input.back())))
	{
		input.pop_back();
	}

	return input;
}

std::string PromptRequired(const std::string& message)
{
	while (true)
	{
		std::cout << message;

		std::string input;

		if (!std::getline(std::cin, input))
		{
			throw std::runtime_error("Input stream closed.");
		}

		input = TrimQuotesAndSpaces(input);

		if (!input.empty())
		{
			return input;
		}

		std::cout << "Input cannot be empty. Please try again." << std::endl;
	}
}

std::string RemoveProtocol(std::string url)
{
	const std::string lower = ToLower(url);

	if (lower.starts_with("http://"))
	{
		return url.substr(7);
	}

	if (lower.starts_with("https://"))
	{
		return url.substr(8);
	}

	return url;
}

std::string CalculateMD5(const std::string& data)
{
	EVP_MD_CTX* context = EVP_MD_CTX_new();

	if (!context)
	{
		throw std::runtime_error("Failed to create MD5 context.");
	}

	if (EVP_DigestInit_ex(context, EVP_md5(), nullptr) != 1)
	{
		EVP_MD_CTX_free(context);

		throw std::runtime_error("Failed to initialize MD5.");
	}

	if (EVP_DigestUpdate(context, data.data(), data.size()) != 1)
	{
		EVP_MD_CTX_free(context);

		throw std::runtime_error("Failed to calculate MD5.");
	}

	unsigned char digest[EVP_MAX_MD_SIZE];
	unsigned int digestLength = 0;

	if (EVP_DigestFinal_ex(context, digest, &digestLength) != 1)
	{
		EVP_MD_CTX_free(context);

		throw std::runtime_error("Failed to finalize MD5.");
	}

	EVP_MD_CTX_free(context);

	std::ostringstream result;

	for (unsigned int i = 0; i < digestLength; ++i)
	{
		result << std::hex << std::nouppercase << std::setw(2) << std::setfill('0') << static_cast<int>(digest[i]);
	}

	return result.str();
}

std::string CalculateFileMD5(const filesystem::path& filePath)
{
	std::ifstream file(filePath, std::ios::binary);

	if (!file.is_open())
	{
		throw std::runtime_error("Unable to open file: " + filePath.string());
	}

	EVP_MD_CTX* context = EVP_MD_CTX_new();

	if (!context)
	{
		throw std::runtime_error("Failed to create MD5 context.");
	}

	if (EVP_DigestInit_ex(context, EVP_md5(), nullptr) != 1)
	{
		EVP_MD_CTX_free(context);

		throw std::runtime_error("Failed to initialize MD5.");
	}

	constexpr std::size_t BUFFER_SIZE = 4ull * 1024ull * 1024ull;

	std::vector<unsigned char> buffer(BUFFER_SIZE);

	std::uint64_t totalRead = 0;

	while (true)
	{
		file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));

		const std::streamsize bytesRead = file.gcount();

		if (bytesRead > 0)
		{
			if (EVP_DigestUpdate(context, buffer.data(), static_cast<std::size_t>(bytesRead)) != 1)
			{
				EVP_MD_CTX_free(context);

				throw std::runtime_error("EVP_DigestUpdate failed for: " + filePath.string());
			}

			totalRead += static_cast<std::uint64_t>(bytesRead);
		}

		if (file.eof())
		{
			break;
		}

		if (file.fail())
		{
			EVP_MD_CTX_free(context);

			throw std::runtime_error("Failed while reading: " + filePath.string());
		}
	}

	unsigned char digest[EVP_MAX_MD_SIZE];
	unsigned int digestLength = 0;

	if (EVP_DigestFinal_ex(context, digest, &digestLength) != 1)
	{
		EVP_MD_CTX_free(context);

		throw std::runtime_error("Failed to finalize MD5: " + filePath.string());
	}

	EVP_MD_CTX_free(context);

	std::ostringstream result;

	for (unsigned int i = 0; i < digestLength; ++i)
	{
		result << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(digest[i]);
	}

	return result.str();
}



int GetFlags(const std::string& relativePath)
{
	const std::string fileName = ToLower(filesystem::path(relativePath).filename().string());
	const std::string path = ToLower(relativePath);

	if (fileName.starts_with("wow-update-"))
	{
		return 4;
	}

	if (fileName == "base-osx.mpq" || fileName == "base-win.mpq")
	{
		return 2;
	}

	const std::size_t slashCount = std::count(path.begin(), path.end(), '/');

	if (path.starts_with("data/") && slashCount > 1)
	{
		return 1;
	}

	return 0;
}

std::vector<filesystem::path> FindMPQFiles(const filesystem::path& wowFolder)
{
	const std::vector<std::string> targetDirs =
	{
		"Data",
		"Updates"
	};

	std::vector<filesystem::path> mpqFiles;

	for (const auto& dir : targetDirs)
	{
		const filesystem::path fullPath = wowFolder / dir;

		if (!filesystem::exists(fullPath) || !filesystem::is_directory(fullPath))
		{
			continue;
		}

		for ( const auto& entry : filesystem::recursive_directory_iterator(fullPath, filesystem::directory_options::skip_permission_denied))
		{
			if (!entry.is_regular_file())
			{
				continue;
			}

			if (ToLower(entry.path().extension().string()) == ".mpq")
			{
				mpqFiles.push_back(entry.path());
			}
		}
	}

	std::sort(mpqFiles.begin(), mpqFiles.end());

	return mpqFiles;
}

std::string GeneratePartialManifest(const filesystem::path& wowFolder, int fileVersion)
{
	std::ostringstream mfil;

	mfil << "version=3" << std::endl;
	mfil << "serverpath=base" << std::endl;
	mfil << "\tpath=" << std::endl;

	const auto mpqFiles = FindMPQFiles(wowFolder);

	std::cout << "Found " << mpqFiles.size() << " MPQ file(s)." << std::endl;

	for (const auto& filePath : mpqFiles)
	{
		const std::uintmax_t fileSize = filesystem::file_size(filePath);
		const std::string relativePath = filesystem::relative(filePath, wowFolder).generic_string();
		const std::string checksum = CalculateFileMD5(filePath);
		const int flags = GetFlags(relativePath);

		mfil << "file=" << relativePath << std::endl;
		mfil << "\tname=" << relativePath << std::endl;
		mfil << "\tsize=" << fileSize << std::endl;
		mfil << "\tchecksum=" << checksum << std::endl;
		mfil << "\tfileversion=" << fileVersion << std::endl;
		mfil << "\tflags=" << flags << std::endl;
		mfil << "\tpath=base" << std::endl;
	}

	return mfil.str();
}

bool IsClientFile(const filesystem::path& filePath)
{
	const std::string extension = ToLower(filePath.extension().string());

	if (extension == ".mpq")
	{
		return false;
	}

	if (extension == ".mfil")
	{
		return false;
	}

	return true;
}

std::vector<filesystem::path> FindClientFiles(const filesystem::path& wowFolder)
{
	std::vector<filesystem::path> clientFiles;

	for (const auto& entry : filesystem::recursive_directory_iterator(wowFolder, filesystem::directory_options::skip_permission_denied))
	{
		if (!entry.is_regular_file())
		{
			continue;
		}

		const filesystem::path filePath = entry.path();

		if (!IsClientFile(filePath))
		{
			continue;
		}

		clientFiles.push_back(filePath);
	}

	std::sort(clientFiles.begin(), clientFiles.end());

	return clientFiles;
}

std::string GenerateClientManifest(const filesystem::path& wowFolder, int fileVersion)
{
	std::ostringstream mfil;

	mfil << "version=3" << std::endl;
	mfil << "serverpath=base" << std::endl;
	mfil << "\tpath=" << std::endl;

	const auto clientFiles = FindClientFiles(wowFolder);

	std::cout << "Found " << clientFiles.size() << " client file(s)." << std::endl;

	for (const auto& filePath : clientFiles)
	{
		const std::uintmax_t fileSize = filesystem::file_size(filePath);
		const std::string relativePath = filesystem::relative(filePath, wowFolder) .generic_string();
		const std::string checksum = CalculateFileMD5(filePath);

		mfil << "file=" << relativePath << std::endl;
		mfil << "\tname=" << relativePath << std::endl;
		mfil << "\tsize=" << fileSize << std::endl;
		mfil << "\tchecksum=" << checksum << std::endl;
		mfil << "\tfileversion=" << fileVersion << std::endl;
		mfil << "\tflags=0" << std::endl;
		mfil << "\tpath=base" << std::endl;
	}

	return mfil.str();
}


int main(int argc, char* argv[])
{
	try
	{
		filesystem::path wowFolder;

		if (argc > 1)
		{
			wowFolder = filesystem::path(argv[1]);
		}
		else
		{
			wowFolder = filesystem::current_path();
		}

		wowFolder = filesystem::absolute(wowFolder);

		constexpr int fileVersion = 12340;

		std::cout
			<< "========================================\n"
			<< "     WoW 3.3.5a MFIL Generator\n"
			<< "========================================\n\n";

		const filesystem::path dataFolder = wowFolder / "Data";

		if (!filesystem::exists(dataFolder) || !filesystem::is_directory(dataFolder))
		{
			std::cout << "Please run this tool in your World of Warcraft directory (where the Data folder is located) or pass the path as an argument." << std::endl;

			return 1;
		}

		std::cout << "WoW directory:" << std::endl << wowFolder.string() << std::endl << std::endl;

		std::cout << "[1/3] Generating partial manifest..." << std::endl;

		const std::string partialContent = GeneratePartialManifest(wowFolder, fileVersion);
		const std::string partialHash = CalculateMD5(partialContent);
		const std::string partialFileName = "wow-" + std::to_string(fileVersion) + "-" + partialHash + ".mfil";
		const filesystem::path partialOutput = wowFolder / partialFileName;

		{
			std::ofstream file(partialOutput, std::ios::binary);

			if (!file)
			{
				throw std::runtime_error("Unable to create partial manifest: " + partialOutput.string());
			}

			file.write(partialContent.data(), static_cast<std::streamsize>(partialContent.size()));

			if (!file)
			{
				throw std::runtime_error("Failed to write partial manifest.");
			}
		}

		std::cout << "Partial manifest generated:" << std::endl << partialOutput.string() << std::endl;
		std::cout << "MD5: " << partialHash << std::endl << std::endl;
		std::cout << "[2/3] Generating client manifest..." << std::endl << std::endl;

		const std::string clientContent = GenerateClientManifest(wowFolder, fileVersion);
		const std::string clientHash = CalculateMD5(clientContent);
		const std::string clientFileName = "wow-client-" + std::to_string(fileVersion) + "-" + clientHash + ".mfil";

		const filesystem::path clientOutput = wowFolder / clientFileName;

		{
			std::ofstream file(clientOutput, std::ios::binary);

			if (!file)
			{
				throw std::runtime_error("Unable to create client manifest: " + clientOutput.string());
			}

			file.write(clientContent.data(), static_cast<std::streamsize>(clientContent.size()));

			if (!file)
			{
				throw std::runtime_error("Failed to write client manifest.");
			}
		}

		std::cout << "Client manifest generated:" << std::endl << clientOutput.string() << std::endl;
		std::cout << "MD5: " << clientHash << std::endl << std::endl;
		std::cout << "[3/3] Configuring WoW.mfil..." << std::endl << std::endl;

		MfilGenerator generator;

		const std::string build = std::to_string(fileVersion);

		generator.SetClientPartial(build, clientHash);
		generator.SetManifestPartialFromHash(build, partialHash);

		int serverCount = 0;

		while (true)
		{
			std::cout << "How many servers do you " "want to add? ";

			std::string input;

			if (!std::getline(std::cin, input))
			{
				throw std::runtime_error("Input stream closed.");
			}

			try
			{
				std::size_t position = 0;

				const int count = std::stoi(input, &position);

				if (position == input.size() && count > 0)
				{
					serverCount = count;
					break;
				}
			}
			catch (...)
			{
			}

			std::cout << "Please enter a valid positive number." << std::endl;
		}

		for (int i = 1; i <= serverCount; ++i)
		{
			const std::string serverName = PromptRequired("Enter name for Server #" + std::to_string(i) + " (e.g., akamai): ");
			std::string protocol;

			while (true)
			{
				std::cout << "Use http or https for Server #" << i << "? [http/https]: ";

				std::string input;

				if (!std::getline(std::cin, input))
				{
					throw std::runtime_error("Input stream closed.");
				}

				protocol = ToLower(TrimQuotesAndSpaces(input));

				if (protocol == "http" || protocol == "https")
				{
					break;
				}

				std::cout << "Please enter either 'http' or 'https'." << std::endl;
			}

			const std::string urlPart = PromptRequired(
					"Enter the rest of the URL "
					"for Server #" +
					std::to_string(i) +
					" (e.g., "
					"dist.blizzard.com.edgesuite.net/"
					"wow-pod-retail/EU/" +
					build +
					".direct/): "
				);

			const std::string cleanUrl = RemoveProtocol(urlPart);
			const std::string serverUrl = protocol + "://" + cleanUrl;

			generator.AddServer(serverName, serverUrl);
		}

		const filesystem::path finalOutput = wowFolder / "WoW.mfil";

		generator.SaveToFile(finalOutput);

		std::cout
			<< "\n========================================\n"
			<< "Generation complete\n"
			<< "========================================\n\n";

		std::cout << "Partial MFIL:" << std::endl << partialOutput.string() << std::endl << std::endl;
		std::cout << "Main MFIL:" << std::endl << finalOutput.string() << std::endl << std::endl;
		std::cout << "--- WoW.mfil ---" << std::endl << generator.Generate() << "----------------" << std::endl << std::endl;
		std::cout << "Press Enter to exit...";

		std::string dummy;
		std::getline(std::cin, dummy);

		return 0;
	}
	catch (const filesystem::filesystem_error& ex)
	{
		std::cerr << "Filesystem error: " << ex.what() << std::endl;

		return 1;
	}
	catch (const std::exception& ex)
	{
		std::cerr << "Error: " << ex.what() << std::endl;

		return 1;
	}
}
