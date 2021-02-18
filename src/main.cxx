#include "db_tools.hxx"
#include "xray_re/xr_file_system.hxx"

#include <boost/program_options.hpp>
#include <spdlog/spdlog.h>

using namespace xray_re;
using namespace boost::program_options;

enum class ToolsType
{
	AUTO   = 0x00,
	UNPACK = 0x01,
	PACK   = 0x02
};

bool IsConflictingOptionsExist(const variables_map& vm, const std::vector<std::string>& options)
{
	std::string found_option;
	for(const auto& option : options)
	{
		if(vm.count(option))
		{
			if(found_option.empty())
			{
				found_option = option;
			}
			else
			{
				spdlog::error("Conflicting options \"{}\" and \"{}\" specified", found_option, option);
				return true;
			}
		}
	}

	return false;
}

int main(int argc, char *argv[])
{
	try
	{
		unsigned int line_lenght = 82;
		options_description common_options("Common options", line_lenght);
		common_options.add_options()
		    ("help", "produce help message")
		    ("debug", "enable debug output")
		    ("ro", "perform all the steps but do not write anything on disk")
		    ("out", value<std::string>()->value_name("<PATH>"), "output file or folder name")
		    ("11xx", "assume 1114/1154 archive format (unpack only)")
		    ("2215", "assume 2215 archive format (unpack only)")
		    ("2945", "assume 2945/2939 archive format (unpack only)")
		    ("2947ru", "assume release version format")
		    ("2947ww", "assume worldwide release version and 3120 format")
		    ("xdb", "assume .xdb or .db archive format");

		options_description unpack_options("Unpack options");
		unpack_options.add_options()
		    ("unpack", value<std::string>()->value_name("<FILE>"), "unpack game archive")
		    ("flt", value<std::string>()->value_name("<MASK>"), "extract files filtered by mask");

		options_description pack_options("Pack options");
		pack_options.add_options()
		    ("pack", value<std::string>()->value_name("<DIR>"), "pack directory content into game archive")
		    ("xdb_ud", value<std::string>()->value_name("<FILE>"), "attach user data file");

		options_description all_options;
		all_options.add(common_options).add(unpack_options).add(pack_options);

		variables_map vm;
		store(parse_command_line(argc, argv, all_options), vm);
		notify(vm);

		if(vm.count("debug"))
		{
			DBTools::set_debug(true);
			spdlog::set_level(spdlog::level::debug);
		}

		if(vm.count("help"))
		{
			spdlog::info("Usage examples:");
			spdlog::info("  db_converter --unpack resources.db0 --xdb --dir ~/extracted");
			spdlog::info("  db_converter --pack ~/dir_to_pack/ --out ~/packed.db --xdb");
			std::stringstream all_options_string;
			all_options_string << all_options;
			spdlog::info(all_options_string.str());

			return 1;
		}

		if(IsConflictingOptionsExist(vm, {"pack", "unpack"}))
		{
			return 1;
		}

		if(IsConflictingOptionsExist(vm, {"11xx", "2215", "2945", "2947ru", "2947ww", "xdb"}))
		{
			return 1;
		}

		auto tools_type = ToolsType::AUTO;

		if(vm.count("unpack"))
		{
			tools_type = ToolsType::UNPACK;
		}

		if(vm.count("pack"))
		{
			tools_type = ToolsType::PACK;
		}

		std::string fs_spec;

		unsigned int fs_flags = 0;
		if(vm.count("ro"))
		{
			fs_flags |= xr_file_system::FSF_READ_ONLY;
			spdlog::info("Working in read-only mode");
		}

		xr_file_system& fs = xr_file_system::instance();
		if(!fs.initialize(fs_spec, fs_flags))
		{
			spdlog::critical("Can't initialize the file system");
			return 1;
		}

		auto version = DBVersion::DB_VERSION_AUTO;

		std::vector<std::pair<std::string, DBVersion>> db_versions =
		{
			{"xdb",    DBVersion::DB_VERSION_XDB},
			{"2947ru", DBVersion::DB_VERSION_2947RU},
			{"2947ww", DBVersion::DB_VERSION_2947WW},
			{"11xx",   DBVersion::DB_VERSION_AUTO},
			{"2215",   DBVersion::DB_VERSION_2215},
			{"2945",   DBVersion::DB_VERSION_2945}
		};

		for(const auto& db_version : db_versions)
		{
			if(vm.count(db_version.first))
			{
				version = db_version.second;
				break;
			}
		}

		auto extension_to_db_version = [](const std::string& extension)
		{
			if(DBTools::is_xdb(extension) || DBTools::is_db(extension))
			{
				spdlog::info("Auto-detected version: xdb");
				return DBVersion::DB_VERSION_XDB;
			}
			else if(DBTools::is_xrp(extension))
			{
				spdlog::info("Auto-detected version: 1114");
				return DBVersion::DB_VERSION_1114;
			}
			else if(DBTools::is_xp(extension))
			{
				spdlog::info("Auto-detected version: 2215");
				return DBVersion::DB_VERSION_2215;
			}

			return DBVersion::DB_VERSION_AUTO;
		};

		if(tools_type == ToolsType::UNPACK)
		{
			auto source_path = vm["unpack"].as<std::string>();
			auto path_splitted = xr_file_system::split_path(source_path);
			auto extension = path_splitted.extension;

			if(version == DBVersion::DB_VERSION_AUTO)
			{
				if(!DBTools::is_known(extension))
				{
					spdlog::error("Unknown input file extension");
					return 1;
				}

				version = extension_to_db_version(extension);
			}

			auto destination_path = vm.count("out") ? vm["out"].as<std::string>() : xr_file_system::current_path();

			std::string filter;
			if(vm.count("flt"))
			{
				filter = vm["flt"].as<std::string>();
			}

			DBTools::unpack(source_path, destination_path, version, filter);
		}
		else if(tools_type == ToolsType::PACK)
		{
			auto source_path = vm["pack"].as<std::string>();
			auto destination_path = vm.count("out") ? vm["out"].as<std::string>() : "";
			auto path_splitted = xr_file_system::split_path(destination_path);
			auto extension = path_splitted.extension;

			if(version == DBVersion::DB_VERSION_AUTO)
			{
				if(!DBTools::is_known(extension))
				{
					spdlog::error("Unknown output file extension");
					return 1;
				}

				version = extension_to_db_version(extension);
			}

			std::string filter;
			if(vm.count("flt"))
			{
				filter = vm["flt"].as<std::string>();
			}

			std::string xdb_ud;
			if(vm.count("xdb_ud"))
			{
				xdb_ud = vm["xdb_ud"].as<std::string>();
			}

			DBTools::pack(source_path, destination_path, version, xdb_ud);
		}
		else
		{
			spdlog::info("No tools selected");
			spdlog::info("Try \"db_converter --help\" for more information");
		}

		return 0;
	}
	catch(boost::program_options::unknown_option& e)
	{
		spdlog::error(e.what());
	}
	catch(std::exception& e)
	{
		spdlog::critical("Exception: {}", e.what());
	}
	catch(...)
	{
		spdlog::critical("Unknown exception");
	}

	return 0;
}
