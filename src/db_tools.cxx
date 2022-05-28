#include "db_tools.hxx"
#include "packer.hxx"
#include "unpacker.hxx"

#include <spdlog/spdlog.h>

using namespace xray_re;

bool m_debug = false;

void DBTools::pack(const std::string& source_path, const std::string& destination_path, const xray_re::DBVersion& version, const std::string& xdb_ud, bool is_read_only)
{
	Packer packer;
	packer.process(source_path, destination_path, version, xdb_ud, is_read_only);
}

void DBTools::unpack(const std::string& source_path, const std::string& destination_path, const xray_re::DBVersion& version, const std::string& filter, bool is_read_only)
{
	Unpacker unpacker;
	unpacker.process(source_path, destination_path, version, filter, is_read_only);
}

void DBTools::set_debug(bool value)
{
	m_debug = value;
	if(m_debug)
	{
		spdlog::set_level(spdlog::level::debug);
	}
}
