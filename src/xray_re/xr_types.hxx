#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>

#include <string>

namespace xray_re
{
	constexpr auto PA_FS_ROOT = "$fs_root$";

	enum
	{
		DB_CHUNK_DATA     = 0,
		DB_CHUNK_HEADER   = 1,
		DB_CHUNK_USERDATA = 0x29a
	};

	enum class DBVersion
	{
		DB_VERSION_AUTO   = 0,
		DB_VERSION_1114   = 0x01,
		DB_VERSION_2215   = 0x02,
		DB_VERSION_2945   = 0x04,
		DB_VERSION_2947RU = 0x08,
		DB_VERSION_2947WW = 0x10,
		DB_VERSION_XDB    = 0x20
	};

	struct db_file
	{
		bool operator<(const db_file& file) const;

		std::string path;
		std::size_t offset;
		std::size_t size_real;
		std::size_t size_compressed;
		unsigned int crc;
	};
} // namespace xray_re
