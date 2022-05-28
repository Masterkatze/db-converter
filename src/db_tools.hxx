#pragma once

#include "xray_re/xr_types.hxx"

#include <string>
#include <vector>

class DBTools
{
public:
	static void pack(const std::string& source_path, const std::string& destination_path, const xray_re::DBVersion& version, const std::string& xdb_ud, bool is_read_only);
	static void unpack(const std::string& source_path, const std::string& destination_path, const xray_re::DBVersion& version, const std::string& filter, bool is_read_only);

	static void set_debug(bool value);
};
