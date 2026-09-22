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

struct CONFIG
{
	bool szConsole;
	bool szDebug;
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
			LOG_INFO("Directory not found, creating: ", directory.string());

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
			}
			else
			{
				if (key == "location" && !server.empty() && !data.szServers.empty())
				{
					data.szServers.back().szUrl = value;
				}
			}
		}

		LOG_INFO("mFIL loaded - Version={} | Servers={} | Manifest={}", data.szVersion, std::to_string(data.szServers.size()), data.szManifestPartial);

		return data;
	}
};

struct CLIENT_CHUNK_ENTRY
{
	std::string szHash;
	std::int64_t szOffset = 0;
	std::int64_t szSize = 0;
	std::uint32_t szCheckSum = 0;
};

struct CLIENT_MANIFEST_ENTRY
{
	std::string szFile;
	std::string szName;
	std::int64_t szSize = 0;
	std::uint32_t szCheckSum = 0;

	std::vector<CLIENT_CHUNK_ENTRY> szChunks;
};

struct CLIENT_MANIFEST
{
	std::string szVersion = "2";
	std::vector<CLIENT_MANIFEST_ENTRY> szFiles;

	static CLIENT_MANIFEST Parse(const std::vector<std::uint8_t>& input)
	{
		if (input.empty())
		{
			return CLIENT_MANIFEST { "2", {} };
		}

		std::vector<std::uint8_t> raw = input;

		const std::size_t length = std::min<std::size_t>(8, raw.size());

		const std::string prefix(raw.begin(), raw.begin() + length);

		if (!winrt::CLauncher::Core::Helper::STARTS_WITH_CASE_SENSITIVE(prefix, "version="))
		{
			raw = winrt::CLauncher::Core::Helper::XOR(raw, winrt::CLauncher::Core::Globals::MANIFEST_KEY);
		}

		const std::string content(raw.begin(), raw.end());

		return Parse(content);
	}

	static CLIENT_MANIFEST Parse(const std::string& content)
	{
		CLIENT_MANIFEST manifest;

		manifest.szVersion = "2";

		if (content.empty())
		{
			return manifest;
		}

		std::string current = content;

		if (!winrt::CLauncher::Core::Helper::STARTS_WITH_CASE_SENSITIVE(current, "version="))
		{
			const auto decrypted = winrt::CLauncher::Core::Helper::XOR(current, winrt::CLauncher::Core::Globals::MANIFEST_KEY);
			const std::string decryptedStr(decrypted.begin(), decrypted.end());

			if (winrt::CLauncher::Core::Helper::STARTS_WITH_CASE_SENSITIVE(decryptedStr, "version="))
			{
				current = decryptedStr;
			}
		}

		CLIENT_MANIFEST_ENTRY Entries;
		bool hasEntry = false;

		for (const auto& raw : winrt::CLauncher::Core::Helper::SPLIT_LINES(current))
		{
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

			const std::string key = winrt::CLauncher::Core::Helper::TO_LOWER(winrt::CLauncher::Core::Helper::TRIM(line.substr(0,index)));
			const std::string value = winrt::CLauncher::Core::Helper::TRIM(line.substr(index + 1));

			if (key == "version")
			{
				manifest.szVersion = value;
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
				hasEntry = true;
			}
			else if (key == "name")
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
					std::uint32_t checksum{};

					if (winrt::CLauncher::Core::Helper::TRY_PARSE_INT32_HEX(value, checksum))
					{
						Entries.szCheckSum = checksum;
					}
				}
			}
			else if (key == "chunks")
			{
				//
			}
			else if (key == "chunk")
			{
				if (!hasEntry)
				{
					continue;
				}

				std::vector<std::string> parts;
				std::stringstream ss(value);
				std::string part;

				while (std::getline(ss, part, ','))
				{
					parts.push_back(winrt::CLauncher::Core::Helper::TRIM(part));
				}

				if (parts.size() >= 3)
				{
					CLIENT_CHUNK_ENTRY chunk;
					chunk.szHash = parts[0];
					std::int64_t offset{};

					if (winrt::CLauncher::Core::Helper::TRY_PARSE_INT64(parts[1], offset))
					{
						chunk.szOffset = offset;
					}

					std::int64_t size{};

					if (winrt::CLauncher::Core::Helper::TRY_PARSE_INT64(parts[2], size))
					{
						chunk.szSize = size;
					}

					if (parts.size() >= 4)
					{
						std::uint32_t checksum{};

						if (winrt::CLauncher::Core::Helper::TRY_PARSE_INT32_HEX(parts[3], checksum))
						{
							chunk.szCheckSum = checksum;
						}
					}

					Entries.szChunks.push_back(chunk);
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

struct MPQ_MANIFEST_ENTRY
{
	std::string szFile;
	std::string szName;
	std::int64_t szSize = 0;
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
			else if (key == "name")
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
};

namespace winrt::CLauncher::Core
{
	class Globals
	{
	public:
		static inline std::string REALMLISTS_TEXT_PATH = "Data\\enUS";
		static inline std::string REALMLISTS_TEXT_FILE = "realmlist.wtf";

		static inline std::string CONFIG_WTF_FOLDER_PATH = "WTF";
		static inline std::string CONFIG_WTF_FILE = "Config.wtf";

		static inline std::string MFIL_FILE = "WoW.mfil";

		static inline std::string REMOTE_AUTH_TOKEN = "hLFkyoJwkDZgagzfZ8ZmxpgyOz3K4RqLKeaQya6uFt2lhVFOfbyamTj9crYk0Df1";

		static inline std::string MANIFEST_KEY = "wotlk";

		static inline REALM_LISTS CURRENT_REALM_LIST;
		static inline CONFIG_WTF CURRENT_CONFIG_WTF;
		static inline MFIL CURRENT_MFIL;

		static inline CLIENT_MANIFEST CURRENT_CLIENT_MANIFEST {
			"2",
			{}
		};

		static inline MPQ_MANIFEST CURRENT_MPQ_MANIFEST {
			"3",
			"base",
			{}
		};


		static std::string PATCHES_NOTE_REMOTE_BASE_URL()
		{
			if (!CURRENT_MFIL.szServers.empty())
			{
				return CURRENT_MFIL.szServers.front().szUrl;
			}

			return {};
		}

		static std::string PATCH_REMOTE_BASE_URL()
		{
			if (!CURRENT_MFIL.szServers.empty())
			{
				return CURRENT_MFIL.szServers.front().szUrl;
			}

			return {};
		}
	};

	inline REALM_LISTS Globals::CURRENT_REALM_LIST = REALM_LISTS::Load();
	inline CONFIG_WTF Globals::CURRENT_CONFIG_WTF = CONFIG_WTF::Load();
	inline MFIL Globals::CURRENT_MFIL = MFIL::Load();
}
