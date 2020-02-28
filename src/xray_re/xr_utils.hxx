#pragma once

#include <string>

namespace xray_re
{
	enum
	{
		CHUNK_COMPRESSED = 0x80000000,
	};

	template<typename T> static inline
	typename T::value_type find_by_name(T& container, const char* name)
	{
		for (typename T::iterator it = container.begin(), end = container.end(); it != end; ++it)
		{
			if ((*it)->name() == name)
				return *it;
		}
		return container.end();
	}

	template<typename T> static inline
	typename T::value_type find_by_name(T& container, const std::string& name)
	{
		//return find_by_name<T>(container, name.c_str());
		for (typename T::iterator it = container.begin(), end = container.end(); it != end; ++it)
		{
			if ((*it)->name() == name)
				return *it;
		}
		return container.end();
	}

	template<typename T> void delete_elements(T& container)
	{
		for (typename T::iterator it = container.begin(), end = container.end(); it != end; ++it)
			delete *it;
	}
}
