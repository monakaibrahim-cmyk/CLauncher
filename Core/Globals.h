#pragma once

#include "Helper.h"
#include "Logging/Logs.h"

#include <algorithm>
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

namespace winrt::CLauncher::Core::Globals
{
	inline std::string REALMLISTS_TEXT_PATH = "Data\\enUS";
	inline std::string REALMLISTS_TEXT_FILE = "realmlist.wtf";

	inline std::string CONFIG_WTF_FOLDER_PATH = "WTF";
	inline std::string CONFIG_WTF_FILE = "Config.wtf";

	inline std::string MFIL_FILE = "WoW.mfil";

	inline std::string REMOTE_AUTH_API = "https://127.0.0.1/api/v1";

	inline const std::string PUBLIC_KEY_PEM = R"(
			-----BEGIN PUBLIC KEY-----
			MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA0O0c690RnZ02SPLUL4jq
			Vz8NhcsJ0iG1WpkiierERyKN/uz3lZ4PBSs9cIPZiaQ+8dKsEVEryc3FF+IpxrDy
			bzVTBtE4vDbOZilTFej2FSYLUEC1SN6np8hMko6glejjp2sjXR8zrgO0Dj4fxqh2
			DrwpLtNqO6jrtjatRVdSzw5QIuKk8NYyqeyWvT2r28y8elZfUfE0KFEDg9hlMqek
			DpKO1nj+OqSQNKXSdke5MDmjxTo2zZCfK5Skj+thy2h9UUR+gv3ECfBvCQbVwDXF
			HJxSCrBq90u/BDGZNdDp2EHMTuzZKSwTBZhHg2z4hTGzTkan9T3gG/XHV1qDuPwe
			VQIDAQAB
			-----END PUBLIC KEY-----
		)";

	inline std::string MANIFEST_KEY = "wotlk";
	inline std::string ENCRYPTION_KEY = "WOTLK";

#ifndef NDEBUG
	inline std::string DEBUG_AUTH_TOKEN = "hLFkyoJwkDZgagzfZ8ZmxpgyOz3K4RqLKeaQya6uFt2lhVFOfbyamTj9crYk0Df1";
#endif
}

struct CONFIG
{
	bool szConsole;
	bool szDebug;
};

struct PLAYER_LOGIN
{
	std::uint64_t szVersion;
	std::string szUsername;
	std::string szPassword;
	std::string szRealmName;

	bool alwaysSignedIn;

	static std::filesystem::path DIRECTORY_FULL_PATH()
	{
		return winrt::CLauncher::Core::Helper::GET_ROOT_DIRECTORY();
	}

	static inline bool LOAD_ENCRYPTED_DATA(const unsigned char* key, PLAYER_LOGIN& player)
	{
		std::filesystem::path root = DIRECTORY_FULL_PATH() / "Player.dat";
		std::ifstream file(root, std::ios::binary);

		if (!file.is_open())
		{
			LOG_ERROR("Failed to open file for reading: {}", root.string());

			return false;
		}

		unsigned char iv[winrt::CLauncher::Core::LoginSaveCrypto::IV_SIZE];
		unsigned char tag[winrt::CLauncher::Core::LoginSaveCrypto::TAG_SIZE];
		file.read(reinterpret_cast<char*>(iv), winrt::CLauncher::Core::LoginSaveCrypto::IV_SIZE);
		file.read(reinterpret_cast<char*>(tag), winrt::CLauncher::Core::LoginSaveCrypto::TAG_SIZE);

		uint32_t cipher_length = 0;
		file.read(reinterpret_cast<char*>(&cipher_length), sizeof(cipher_length));

		std::vector<unsigned char> chiper(cipher_length);
		file.read(reinterpret_cast<char*>(chiper.data()), cipher_length);
		file.close();

		std::vector<unsigned char> plain(cipher_length);
		int plain_length = 0;

		if (!winrt::CLauncher::Core::LoginSaveCrypto::DECRYPT_DATA(chiper.data(), static_cast<int>(chiper.size()), key, iv, tag, plain.data(), plain_length))
		{
			LOG_ERROR("Decryption failed! File may be tampered with or the key is incorrect.");

			return false;
		}

		size_t offset = 0;

		auto readString = [&plain, &offset](std::string& str)
		{
			if (offset + sizeof(uint32_t) > plain.size())
			{
				return false;
			}

			uint32_t length = *reinterpret_cast<const uint32_t*>(&plain[offset]);
			offset += sizeof(uint32_t);

			if (offset + length > plain.size())
			{
				return false;
			}

			str.assign(reinterpret_cast<const char*>(&plain[offset]), length);
			offset += length;

			return true;
		};

		if (!readString(player.szUsername) || !readString(player.szPassword) || !readString(player.szRealmName))
		{
			LOG_ERROR("Failed to deserialize player data streams.");

			return false;
		}

		return true;
	}

	static inline bool SAVE_ENCRYPTED_DATA(const PLAYER_LOGIN& player, const unsigned char* key)
	{
		std::filesystem::path root = DIRECTORY_FULL_PATH() / "Player.dat";
		std::vector<unsigned char> plaintext;

		auto appendString = [&plaintext](const std::string& str)
		{
			uint32_t len = static_cast<uint32_t>(str.size());
			const unsigned char* lenBytes = reinterpret_cast<const unsigned char*>(&len);
			plaintext.insert(plaintext.end(), lenBytes, lenBytes + sizeof(len));
			plaintext.insert(plaintext.end(), str.begin(), str.end());
		};

		appendString(player.szUsername);
		appendString(player.szPassword);
		appendString(player.szRealmName);

		unsigned char iv[winrt::CLauncher::Core::LoginSaveCrypto::IV_SIZE];

		if (RAND_bytes(iv, winrt::CLauncher::Core::LoginSaveCrypto::IV_SIZE) != 1)
		{
			LOG_ERROR("Failed to generate random IV.");

			return false;
		}

		std::vector<unsigned char> ciphertext(plaintext.size());
		unsigned char tag[winrt::CLauncher::Core::LoginSaveCrypto::TAG_SIZE];

		if (!winrt::CLauncher::Core::LoginSaveCrypto::ENCRYPT_DATA(plaintext.data(), static_cast<int>(plaintext.size()), key, iv, ciphertext.data(), tag))
		{
			LOG_ERROR("Encryption failed.");

			return false;
		}

		std::ofstream outFile(root, std::ios::binary);

		if (!outFile.is_open())
		{
			LOG_ERROR("Failed to open file {} for writing.", root.string());

			return false;
		}

		outFile.write(reinterpret_cast<char*>(iv), winrt::CLauncher::Core::LoginSaveCrypto::IV_SIZE);
		outFile.write(reinterpret_cast<char*>(tag), winrt::CLauncher::Core::LoginSaveCrypto::TAG_SIZE);

		uint32_t cipherLen = static_cast<uint32_t>(ciphertext.size());

		outFile.write(reinterpret_cast<char*>(&cipherLen), sizeof(cipherLen));
		outFile.write(reinterpret_cast<char*>(ciphertext.data()), cipherLen);

		return true;
	}
};

struct REALM_LISTS
{
	std::string szRemote;

	static std::filesystem::path FILE_FULL_PATH()
	{
		return winrt::CLauncher::Core::Helper::GET_ROOT_DIRECTORY() / winrt::CLauncher::Core::Globals::REALMLISTS_TEXT_PATH / winrt::CLauncher::Core::Globals::REALMLISTS_TEXT_FILE;
	}

	static std::filesystem::path DIRECTORY_FULL_PATH()
	{
		return winrt::CLauncher::Core::Helper::GET_ROOT_DIRECTORY() / winrt::CLauncher::Core::Globals::REALMLISTS_TEXT_PATH;
	}

	static REALM_LISTS Load()
	{
		REALM_LISTS data;
		data.szRemote = "127.0.0.1";
		bool save = true;
		const auto FILE_PATH = FILE_FULL_PATH();

		if (std::filesystem::exists(FILE_PATH))
		{
			std::ifstream file(FILE_PATH);
			std::string line;

			while (std::getline(file, line))
			{
				if (winrt::CLauncher::Core::Helper::STARTS_WITH_CASE_SENSITIVE(line, "set realmlist"))
				{
					std::istringstream stream(line);
					std::string command;
					std::string key;
					std::string remote;

					stream >> command >> key >> remote;

					if (key == "realmlist" && remote == data.szRemote)
					{
						save = false;
						break;
					}
				}
			}
		}

		if (save)
		{
			LOG_INFO("Updating realmlist with predefined remote: {}", data.szRemote);
			data.Save();
		}
		else
		{
			LOG_INFO("Updating realmlist with predefined remote: {}. Skipping save.", data.szRemote);
		}

		return data;
	}

	void Save() const
	{
		const auto DIRECTORY = DIRECTORY_FULL_PATH();

		if (!std::filesystem::exists(DIRECTORY))
		{
			LOG_INFO("Directory not found, creating: {}", DIRECTORY.string());
			
			std::filesystem::create_directories(DIRECTORY);
		}

		const auto FILE_PATH = FILE_FULL_PATH();

		LOG_INFO("Saving realmlist.wtf to: {}", FILE_PATH.string());

		std::ofstream file(FILE_PATH);

		if (!file)
		{
			throw std::runtime_error("Unable to write " + FILE_PATH.string());
		}

		file << "set realmlist " << szRemote << std::endl;
	}
};

struct CONFIG_WTF
{
	std::string szLocale;
	std::string szRealmList;
	bool szHwDetect{};

	bool szGxWindow{};
	bool szGxMaximize{};
	std::string szGxResolution;
	int szGxRefresh{};
	bool szGxTripleBuffer{};
	float szGxMultisampleQuality{};
	int szVideoOptionsVersion{};
	bool szWindowResizeLock{};
	bool szMovie{};
	float szGamma{};

	bool szReadTOS{};
	bool szReadEULA{};
	bool szShowToolsUI{};

	std::string szSoundOutputDriverName;
	float szSoundMusicVolume{};
	float szSoundAmbienceVolume{};

	int szFarclip{};
	bool szSpecular{};
	int szGroundEffectDensity{};
	bool szProjectedTextures{};

	static std::filesystem::path FILE_FULL_PATH()
	{
		return winrt::CLauncher::Core::Helper::GET_ROOT_DIRECTORY() / winrt::CLauncher::Core::Globals::CONFIG_WTF_FOLDER_PATH / winrt::CLauncher::Core::Globals::CONFIG_WTF_FILE;
	}

	static std::filesystem::path DIRECTORY_FULL_PATH()
	{
		return winrt::CLauncher::Core::Helper::GET_ROOT_DIRECTORY() / winrt::CLauncher::Core::Globals::CONFIG_WTF_FOLDER_PATH;
	}

	static CONFIG_WTF Load()
	{
		const auto FILE_PATH = FILE_FULL_PATH();

		if (!std::filesystem::exists(FILE_PATH))
		{
			LOG_WARNING("Config.wtf not found at {}. Generating default configuration.", FILE_PATH.string());

			CONFIG_WTF temp = Default();
			temp.Save();

			return temp;
		}

		LOG_INFO("Loading Config.wtf from {}", FILE_PATH.string());

		CONFIG_WTF config = Default();
		std::ifstream file(FILE_PATH);
		std::string line;

		while (std::getline(file, line))
		{
			const std::string trimmed = winrt::CLauncher::Core::Helper::TRIM(line);

			if (!winrt::CLauncher::Core::Helper::STARTS_WITH_CASE_SENSITIVE(trimmed, "SET "))
			{
				continue;
			}

			std::istringstream stream(trimmed);

			std::string command;
			std::string key;
			std::string value;

			stream >> command >> key;

			std::getline(stream, value);

			value = winrt::CLauncher::Core::Helper::TRIM(value);

			if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
			{
				value = value.substr(1, value.size() - 2);
			}

			key = winrt::CLauncher::Core::Helper::TO_LOWER(key);

			if (key == "locale")
			{
				config.szLocale = value;
			}
			else if (key == "realmlist")
			{
				config.szRealmList = value;
			}
			else if (key == "hwdetect")
			{
				config.szHwDetect = value == "1";
			}
			else if (key == "gxwindow")
			{
				config.szGxWindow = value == "1";
			}
			else if (key == "gxmaximize")
			{
				config.szGxMaximize = value == "1";
			}
			else if (key == "gxresolution")
			{
				config.szGxResolution = value;
			}
			else if (key == "gxrefresh")
			{
				winrt::CLauncher::Core::Helper::TRY_PARSE_INT(value, config.szGxRefresh);
			}
			else if (key == "gxtriplebuffer")
			{
				config.szGxTripleBuffer = value == "1";
			}
			else if (key == "gxmultisamplequality")
			{
				winrt::CLauncher::Core::Helper::TRY_PARSE_FLOAT(value, config.szGxMultisampleQuality);
			}
			else if (key == "videooptionsversion")
			{
				winrt::CLauncher::Core::Helper::TRY_PARSE_INT(value, config.szVideoOptionsVersion);
			}
			else if (key == "windowresizelock")
			{
				config.szWindowResizeLock = value == "1";
			}
			else if (key == "movie")
			{
				config.szMovie = value == "1";
			}
			else if (key == "gamma")
			{
				winrt::CLauncher::Core::Helper::TRY_PARSE_FLOAT(value, config.szGamma);
			}
			else if (key == "readtos")
			{
				config.szReadTOS = value == "1";
			}
			else if (key == "readeula")
			{
				config.szReadEULA = value == "1";
			}
			else if (key == "sound_outputdrivername")
			{
				config.szSoundOutputDriverName = value;
			}
			else if (key == "sound_musicvolume")
			{
				winrt::CLauncher::Core::Helper::TRY_PARSE_FLOAT(value, config.szSoundMusicVolume);
			}
			else if (key == "sound_ambiencevolume")
			{
				winrt::CLauncher::Core::Helper::TRY_PARSE_FLOAT(value, config.szSoundAmbienceVolume);
			}
			else if (key == "farclip")
			{
				winrt::CLauncher::Core::Helper::TRY_PARSE_INT(value, config.szFarclip);
			}
			else if (key == "specular")
			{
				config.szSpecular = value == "1";
			}
			else if (key == "groundeffectdensity")
			{
				winrt::CLauncher::Core::Helper::TRY_PARSE_INT(value, config.szGroundEffectDensity);
			}
			else if (key == "projectedtextures")
			{
				config.szProjectedTextures = value == "1";
			}
		}

		return config;
	}

	static CONFIG_WTF Default()
	{
		CONFIG_WTF config;

		config.szLocale = "enUS";
		config.szRealmList = "127.0.0.1";

		config.szHwDetect = false;

		config.szGxWindow = true;
		config.szGxMaximize = true;
		config.szGxResolution = "1920x1080";
		config.szGxRefresh = 60;
		config.szGxTripleBuffer = true;
		config.szGxMultisampleQuality = 0.000000f;

		config.szVideoOptionsVersion = 3;
		config.szWindowResizeLock = true;
		config.szMovie = false;
		config.szGamma = 1.000000f;

		config.szReadTOS = true;
		config.szReadEULA = true;
		config.szShowToolsUI = true;

		config.szSoundOutputDriverName = "System Default";
		config.szSoundMusicVolume = 0.40000000596046f;
		config.szSoundAmbienceVolume = 0.60000002384186f;

		config.szFarclip = 397;
		config.szSpecular = true;
		config.szGroundEffectDensity = 24;
		config.szProjectedTextures = true;

		return config;
	}

	void Save() const
	{
		const auto directory = DIRECTORY_FULL_PATH();

		if (!std::filesystem::exists(directory))
		{
			LOG_INFO("Directory not found, creating: {}", directory.string());

			std::filesystem::create_directories(directory);
		}

		const auto FILE_PATH = FILE_FULL_PATH();

		LOG_INFO("Saving Config.wtf to: {}.", FILE_PATH.string());

		std::ofstream file(FILE_PATH);

		if (!file)
		{
			throw std::runtime_error("Unable to write " + FILE_PATH.string());
		}

		file << TO_STRING();
	}

	std::string TO_STRING() const
	{
		std::ostringstream sb;

		sb << "SET locale \""
		   << szLocale << "\"\n";

		sb << "SET realmList \""
		   << szRealmList << "\"\n";

		sb << "SET hwDetect \""
		   << (szHwDetect ? 1 : 0) << "\"\n";

		sb << "SET gxWindow \""
		   << (szGxWindow ? 1 : 0) << "\"\n";

		sb << "SET gxMaximize \""
		   << (szGxMaximize ? 1 : 0) << "\"\n";

		sb << "SET gxResolution \""
		   << szGxResolution << "\"\n";

		sb << "SET gxRefresh \""
		   << szGxRefresh << "\"\n";

		sb << "SET gxTripleBuffer \""
		   << (szGxTripleBuffer ? 1 : 0) << "\"\n";

		sb << "SET gxMultisampleQuality \""
		   << std::fixed
		   << std::setprecision(6)
		   << szGxMultisampleQuality
		   << "\"\n";

		sb << "SET videoOptionsVersion \""
		   << szVideoOptionsVersion
		   << "\"\n";

		sb << "SET windowResizeLock \""
		   << (szWindowResizeLock ? 1 : 0)
		   << "\"\n";

		sb << "SET movie \""
		   << (szMovie ? 1 : 0)
		   << "\"\n";

		sb << "SET Gamma \""
		   << std::fixed
		   << std::setprecision(6)
		   << szGamma
		   << "\"\n";

		sb << "SET readTOS \""
		   << (szReadTOS ? 1 : 0)
		   << "\"\n";

		sb << "SET readEULA \""
		   << (szReadEULA ? 1 : 0)
		   << "\"\n";

		sb << "SET showToolsUI \""
		   << (szShowToolsUI ? 1 : 0)
		   << "\"\n";

		sb << "SET Sound_OutputDriverName \""
		   << szSoundOutputDriverName
		   << "\"\n";

		sb << "SET Sound_MusicVolume \""
		   << std::setprecision(std::numeric_limits<float>::max_digits10)
		   << szSoundMusicVolume
		   << "\"\n";

		sb << "SET Sound_AmbienceVolume \""
		   << std::setprecision(std::numeric_limits<float>::max_digits10)
		   << szSoundAmbienceVolume
		   << "\"\n";

		sb << "SET farclip \""
		   << szFarclip
		   << "\"\n";

		sb << "SET specular \""
		   << (szSpecular ? 1 : 0)
		   << "\"\n";

		sb << "SET groundEffectDensity \""
		   << szGroundEffectDensity
		   << "\"\n";

		sb << "SET projectedTextures \""
		   << (szProjectedTextures ? 1 : 0)
		   << "\"\n";

		return sb.str();
	}
};

struct SERVER_ENTRY
{
	std::string szServerName;
	std::string szUrl;
};

struct MFIL
{
	std::string szVersion;
	std::vector<SERVER_ENTRY> szServers;
	std::string szManifestPartial;
	std::string szClientPartial;

	static std::filesystem::path FULL_PATH()
	{
		return winrt::CLauncher::Core::Helper::GET_ROOT_DIRECTORY() / winrt::CLauncher::Core::Globals::MFIL_FILE;
	}

	static std::filesystem::path DIRECTORY_FULL_PATH()
	{
		return winrt::CLauncher::Core::Helper::GET_ROOT_DIRECTORY();
	}

	static MFIL Load()
	{
		MFIL data;
		const std::filesystem::path FILE_PATH = FULL_PATH();

		if (!std::filesystem::exists(FILE_PATH))
		{
			LOG_ERROR("{} not found at {}.", winrt::CLauncher::Core::Globals::MFIL_FILE, FILE_PATH.string());

			std::terminate();
		}

		LOG_INFO("Loading WoW.mfil from: {}", FILE_PATH.string());

		std::ifstream file(FILE_PATH);

		if (!file)
		{
			LOG_ERROR("Failed to open WoW.mfil: {}.", FILE_PATH.string());

			throw std::runtime_error("Failed to open WoW.mfil: " + FILE_PATH.string());
		}

		std::string raw;
		std::string server;

		while (std::getline(file, raw))
		{
			if (!raw.empty() && raw.back() == '\r')
			{
				raw.pop_back();
			}

			const bool indented = !raw.empty() && (raw.front() == '\t' || raw.front() == ' ');
			const std::string line = winrt::CLauncher::Core::Helper::TRIM(raw);

			if (line.empty())
			{
				continue;
			}

			const auto index = line.find('=');

			if (index == std::string::npos)
			{
				continue;
			}

			const auto key = winrt::CLauncher::Core::Helper::TO_LOWER(winrt::CLauncher::Core::Helper::TRIM(line.substr(0, index)));
			const auto value = winrt::CLauncher::Core::Helper::TRIM(line.substr(index + 1));

			if (!indented)
			{
				if (key == "version")
				{
					data.szVersion = value;
				}
				else if (key == "server")
				{
					server = value;

					data.szServers.push_back(SERVER_ENTRY{ value, {}});
				}
				else if (key == "manifest_partial")
				{
					data.szManifestPartial = value;
				}
				else if (key == "client_partial")
				{
					data.szClientPartial = value;
				}
			}
			else
			{
				if (key == "location" && !server.empty() && !data.szServers.empty())
				{
					data.szServers.back().szUrl = value;
				}
			}
		}

		LOG_INFO("mFIL loaded - Version={} | Servers={} | ManifestPartial={} | ClientPartial={}", data.szVersion, data.szServers.back().szServerName, data.szManifestPartial, data.szClientPartial);

		return data;
	}
};

struct MPQ_MANIFEST_ENTRY
{
	std::string szFile;
	std::string szName;
	std::int64_t szSize = 0;
	std::string szChecksum;
	int szFileVersion = 0;
	int szFlags = 0;
	std::string szPath;
};

struct MPQ_MANIFEST
{
	std::string szVersion = "3";
	std::string szServerPath = "base";
	std::vector<MPQ_MANIFEST_ENTRY> szFiles;

	static MPQ_MANIFEST Parse(const std::string& content)
	{
		MPQ_MANIFEST manifest;

		manifest.szVersion = "3";
		manifest.szServerPath = "base";

		if (content.empty())
			return manifest;

		MPQ_MANIFEST_ENTRY Entries;
		bool hasEntry = false;

		for (const auto& rawLine : winrt::CLauncher::Core::Helper::SPLIT_LINES(content))
		{
			const std::string line = winrt::CLauncher::Core::Helper::TRIM(rawLine);

			if (line.empty())
			{
				continue;
			}

			const auto index = line.find('=');

			if (index == std::string::npos)
			{
				continue;
			}

			const std::string key = winrt::CLauncher::Core::Helper::TO_LOWER(winrt::CLauncher::Core::Helper::TRIM(line.substr(0, index)));
			const std::string value = winrt::CLauncher::Core::Helper::TRIM(line.substr(index + 1));

			if (key == "version")
			{
				manifest.szVersion = value;
			}
			else if (key == "serverpath")
			{
				manifest.szServerPath = value;
			}
			else if (key == "file")
			{
				if (hasEntry)
				{
					manifest.szFiles.push_back(Entries);
				}

				Entries = MPQ_MANIFEST_ENTRY{};
				Entries.szFile = value;
				Entries.szName = value;
				Entries.szPath = "base";
				hasEntry = true;
			}
			else if (key == ("name"))
			{
				if (hasEntry)
				{
					Entries.szName = value;
				}
			}
			else if (key == "size")
			{
				if (hasEntry)
				{
					std::int64_t size{};

					if (winrt::CLauncher::Core::Helper::TRY_PARSE_INT64(value, size))
					{
						Entries.szSize = size;
					}
				}
			}
			else if (key == "checksum")
			{
				if (hasEntry)
				{
					Entries.szChecksum = value;
				}
			}
			else if (key == "fileversion")
			{
				if (hasEntry)
				{
					int version{};

					if (winrt::CLauncher::Core::Helper::TRY_PARSE_INT(value, version))
					{
						Entries.szFileVersion = version;
					}
				}
			}
			else if (key == "flags")
			{
				if (hasEntry)
				{
					int flags{};

					if (winrt::CLauncher::Core::Helper::TRY_PARSE_INT(value, flags))
					{
						Entries.szFlags = flags;
					}
				}
			}
			else if (key == "path")
			{
				if (hasEntry)
				{
					Entries.szPath = value;
				}
			}
		}

		if (hasEntry)
		{
			manifest.szFiles.push_back(Entries);
		}

		return manifest;
	}
};

struct CLIENT_MANIFEST_ENTRY
{
	std::string szFile;
	std::string szName;
	std::int64_t szSize = 0;
	std::string szChecksum;
	int szFileVersion = 0;
	int szFlags = 0;
	std::string szPath;
};

struct CLIENT_MANIFEST
{
	std::string szVersion = "3";
	std::string szServerPath = "base";
	std::vector<CLIENT_MANIFEST_ENTRY> szFiles;

	static CLIENT_MANIFEST Parse(const std::string& content)
	{
		CLIENT_MANIFEST manifest;

		manifest.szVersion = "3";
		manifest.szServerPath = "base";

		if (content.empty())
			return manifest;

		CLIENT_MANIFEST_ENTRY Entries;
		bool hasEntry = false;

		for (const auto& rawLine : winrt::CLauncher::Core::Helper::SPLIT_LINES(content))
		{
			const std::string line = winrt::CLauncher::Core::Helper::TRIM(rawLine);

			if (line.empty())
			{
				continue;
			}

			const auto index = line.find('=');

			if (index == std::string::npos)
			{
				continue;
			}

			const std::string key = winrt::CLauncher::Core::Helper::TO_LOWER(winrt::CLauncher::Core::Helper::TRIM(line.substr(0, index)));
			const std::string value = winrt::CLauncher::Core::Helper::TRIM(line.substr(index + 1));

			if (key == "version")
			{
				manifest.szVersion = value;
			}
			else if (key == "serverpath")
			{
				manifest.szServerPath = value;
			}
			else if (key == "file")
			{
				if (hasEntry)
				{
					manifest.szFiles.push_back(Entries);
				}

				Entries = CLIENT_MANIFEST_ENTRY{};
				Entries.szFile = value;
				Entries.szName = value;
				Entries.szPath = "base";
				hasEntry = true;
			}
			else if (key == ("name"))
			{
				if (hasEntry)
				{
					Entries.szName = value;
				}
			}
			else if (key == "size")
			{
				if (hasEntry)
				{
					std::int64_t size{};

					if (winrt::CLauncher::Core::Helper::TRY_PARSE_INT64(value, size))
					{
						Entries.szSize = size;
					}
				}
			}
			else if (key == "checksum")
			{
				if (hasEntry)
				{
					Entries.szChecksum = value;
				}
			}
			else if (key == "fileversion")
			{
				if (hasEntry)
				{
					int version{};

					if (winrt::CLauncher::Core::Helper::TRY_PARSE_INT(value, version))
					{
						Entries.szFileVersion = version;
					}
				}
			}
			else if (key == "flags")
			{
				if (hasEntry)
				{
					int flags{};

					if (winrt::CLauncher::Core::Helper::TRY_PARSE_INT(value, flags))
					{
						Entries.szFlags = flags;
					}
				}
			}
			else if (key == "path")
			{
				if (hasEntry)
				{
					Entries.szPath = value;
				}
			}
		}

		if (hasEntry)
		{
			manifest.szFiles.push_back(Entries);
		}

		return manifest;
	}
};

struct ChecksumHelper
{
	static std::uint32_t CALCULATE_CHECKSUM(const std::filesystem::path& path)
	{
		if (!std::filesystem::exists(path))
		{
			return 0;
		}

		try
		{
			std::uint32_t checksum = 0;
			constexpr std::size_t size = 8192;
			std::vector<std::uint8_t> buffer(size);
			std::ifstream stream(path, std::ios::binary);

			if (!stream)
			{
				return 0;
			}

			while (stream)
			{
				stream.read( reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
				const auto bytes = stream.gcount();

				for (std::streamsize i = 0; i < bytes; ++i)
				{
					const auto b = buffer[static_cast<std::size_t>(i)];
					checksum = (checksum >> 8) ^ b;
					checksum = (checksum * 16777619u) ^ b;
				}
			}

			return checksum;
		}
		catch (const std::exception& ex)
		{
			LOG_WARNING("Failed to calculate checksum for {} : {}", path.string(), ex.what());

			return 0;
		}
	}

	static std::string CALCULATE_MD5(const std::filesystem::path& path)
	{
		return winrt::CLauncher::Core::Helper::CALCULATE_MD5(path);
	}
};

namespace winrt::CLauncher::Core::Globals
{
	inline REALM_LISTS CURRENT_REALM_LIST = REALM_LISTS::Load();
	inline CONFIG_WTF CURRENT_CONFIG_WTF = CONFIG_WTF::Load();
	inline MFIL CURRENT_MFIL = MFIL::Load();

	inline CLIENT_MANIFEST CURRENT_CLIENT_MANIFEST {
		"2",
		{}
	};

	inline MPQ_MANIFEST CURRENT_MPQ_MANIFEST {
		"3",
		"base",
		{}
	};

	inline std::string PATCHES_NOTE_REMOTE_BASE_URL()
	{
		if (!CURRENT_MFIL.szServers.empty())
		{
			return CURRENT_MFIL.szServers.front().szUrl;
		}

		return {};
	}

	inline std::string PATCH_REMOTE_BASE_URL()
	{
		if (!CURRENT_MFIL.szServers.empty())
		{
			return CURRENT_MFIL.szServers.front().szUrl;
		}

		return {};
	}
}
