#pragma once

#include <string>
#include <cstring>
#include <cctype>

namespace xray_re
{
	static inline void xr_strlwr(std::string& s)
	{
		for (std::string::iterator it = s.begin(), end = s.end(); it != end; ++it)
			*it = char(std::tolower(*it));
	}

	static inline void xr_strlwr(char* s, size_t n)
	{
    #if defined(_MSC_VER) && _MSC_VER >= 1400
		_strlwr_s(s, n);
    #else
		for (int c; (c = *s) != 0;)
			*s++ = std::tolower(c);
    #endif
	}

	static inline int xr_stricmp(const char* s1, const char* s2)
	{
		return strcasecmp(s1, s2); // _stricmp
	}

    #if defined(_MSC_VER) && _MSC_VER >= 1400
    #define xr_snprintf	sprintf_s
    #else
	// FIXME: snprintf has different semantics really!!!
    #define xr_snprintf	snprintf
    #endif
}
