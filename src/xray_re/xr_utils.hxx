#pragma once

#include <string>

namespace xray_re
{
	enum
	{
		CHUNK_COMPRESSED = 0x80000000,
	};

	template<typename T> void delete_elements(T& container)
	{
		for(typename T::iterator it = container.begin(), end = container.end(); it != end; ++it)
		{
			delete *it;
		}
	}

	bool is_db(const std::string& extension);
	bool is_xdb(const std::string& extension);
	bool is_xrp(const std::string& extension);
	bool is_xp(const std::string& extension);
	bool is_known(const std::string& extension);
}
