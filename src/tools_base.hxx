#pragma once

#include <string>
//#include "xray_re/xr_cl_parser.hxx"
#include <boost/program_options.hpp>

class tools_base
{
public:
	virtual ~tools_base();

	enum source_format
	{
		TOOLS_AUTO   = 0x000,
		TOOLS_OGF    = 0x001,
		TOOLS_OMF    = 0x002,
		TOOLS_DM     = 0x004,
		TOOLS_LEVEL  = 0x008,
		TOOLS_OGG    = 0x010,
		TOOLS_DDS    = 0x020,
		TOOLS_DB     = 0x040,
		TOOLS_FOLDER = 0x080,
		TOOLS_FANCY	 = 0x100,
		TOOLS_XRDEMO = 0x200,
	};

	virtual void process(const boost::program_options::variables_map& vm) = 0;
};

inline tools_base::~tools_base() {}
