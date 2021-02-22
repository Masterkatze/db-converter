#include "db_tools.hxx"
#include "unpacker.hxx"
#include "packer.hxx"

#include <spdlog/spdlog.h>

using namespace xray_re;

bool m_debug = false;

void DBTools::unpack(const std::string& source_path, const std::string& destination_path, const xray_re::DBVersion& version, const std::string& filter)
{
	Unpacker unpacker;
	unpacker.process(source_path, destination_path, version, filter);
}


void DBTools::pack(const std::string& source_path, const std::string& destination_path, const xray_re::DBVersion& version, const std::string& xdb_ud)
{
	Packer packer;
	packer.process(source_path, destination_path, version, xdb_ud);
}


bool DBTools::is_xrp(const std::string& extension)
{
	return extension == ".xrp";
}

bool DBTools::is_xp(const std::string& extension)
{
	return (extension.size() == 3 && extension == ".xp") ||
	       (extension.size() == 4 && extension.compare(0, 3, ".xp") == 0 && std::isalnum(extension[3]));
}

bool DBTools::is_xdb(const std::string& extension)
{
	return (extension.size() == 3 && extension == ".xdb") ||
	       (extension.size() == 4 && extension.compare(0, 3, ".xdb") == 0 && std::isalnum(extension[4]));
}

bool DBTools::is_db(const std::string& extension)
{
	return (extension.size() == 3 && extension == ".db") ||
	       (extension.size() == 4 && extension.compare(0, 3, ".db") == 0 && std::isalnum(extension[3]));
}

bool DBTools::is_known(const std::string& extension)
{
	return is_db(extension) || is_xdb(extension) || is_xrp(extension) || is_xp(extension);
}

void DBTools::set_debug(const bool value)
{
	m_debug = value;
	if(m_debug)
	{
		spdlog::set_level(spdlog::level::debug);
	}
}
