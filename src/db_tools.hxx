#pragma once

#include "xray_re/xr_types.hxx"

#include <string>
#include <vector>

namespace xray_re
{
	class xr_reader;
	class xr_writer;
};

class db_tools
{
public:
	virtual ~db_tools() = default;

	static bool is_db(const std::string& extension);
	static bool is_xdb(const std::string& extension);
	static bool is_xrp(const std::string& extension);
	static bool is_xp(const std::string& extension);
	static bool is_known(const std::string& extension);

	static void set_debug(const bool value);

	enum
	{
		DB_CHUNK_DATA     = 0,
		DB_CHUNK_HEADER   = 1,
		DB_CHUNK_USERDATA = 0x29a,
	};

	enum db_version
	{
		DB_VERSION_AUTO   = 0,
		DB_VERSION_1114   = 0x01,
		DB_VERSION_2215   = 0x02,
		DB_VERSION_2945   = 0x04,
		DB_VERSION_2947RU = 0x08,
		DB_VERSION_2947WW = 0x10,
		DB_VERSION_XDB    = 0x20,
	};

	enum source_format
	{
		TOOLS_AUTO      = 0x00,
		TOOLS_DB_UNPACK = 0x01,
		TOOLS_DB_PACK   = 0x02
	};

	struct db_file
	{
		bool operator<(const db_file& file) const;

		std::string path;
		size_t offset;
		size_t size_real;
		size_t size_compressed;
		unsigned int crc;
	};

	static bool m_debug;
};

class db_unpacker: public db_tools
{
public:
	~db_unpacker() = default;

	void process(const std::string& source_path, const std::string& destination_path, const db_version& version, const std::string& filter);

protected:
	static void extract_1114(const std::string& prefix, const std::string& mask, xray_re::xr_reader *reader, const uint8_t *data);
	static void extract_2215(const std::string& prefix, const std::string& mask, xray_re::xr_reader *reader, const uint8_t *data);
	static void extract_2945(const std::string& prefix, const std::string& mask, xray_re::xr_reader *reader, const uint8_t *data);
	static void extract_2947(const std::string& prefix, const std::string& mask, xray_re::xr_reader *reader, const uint8_t *data);
};

class db_packer: public db_tools
{
public:
	~db_packer();

	void process(const std::string& source_path, const std::string& destination_path, const db_version& version, const std::string& xdb_ud);

protected:
	void process_folder(const std::string& path = "");
	void process_file(const std::string& path);
	void add_folder(const std::string& path);

protected:
	xray_re::xr_writer *m_archive;
	std::string m_root;
	std::vector<std::string> m_folders;
	std::vector<db_file*> m_files;
};
