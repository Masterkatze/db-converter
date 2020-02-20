#include "db_tools.hxx"
#include "xray_re/xr_file_system.hxx"
#include "spdlog/spdlog.h"
#include <boost/program_options.hpp>
#include <iostream>

using namespace xray_re;
using namespace boost::program_options;

bool conflicting_options_exist(const variables_map& vm, const std::vector<std::string>& options)
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
		options_description common_options("Common options", 82);
		common_options.add_options()
		    ("help", "produce help message")
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
		    ("pack", value<std::string>()->value_name("<DIR>"), "pack game archive")
		    ("xdb_ud", value<std::string>()->value_name("<FILE>"), "attach user data file");

		options_description all_options;
		all_options.add(common_options).add(unpack_options).add(pack_options);

		variables_map vm;
		store(parse_command_line(argc, argv, all_options), vm);
		notify(vm);

		if (vm.count("help"))
		{
			std::cout << "Usage examples:" << std::endl;
			std::cout << "  db_converter --unpack resources.db0 --xdb --dir ~/extracted" << std::endl;
			std::cout << "  db_converter --pack ~/dir_to_pack/ --out ~/packed.db --xdb" << std::endl;
			std::cout << all_options << std::endl;

			return 1;
		}

		if(conflicting_options_exist(vm, {"pack", "unpack"}))
		{
			return 1;
		}

		if(conflicting_options_exist(vm, {"11xx", "2215", "2945", "2947ru", "2947ww", "xdb"}))
		{
			return 1;
		}

		unsigned short int tools_type = db_tools::TOOLS_AUTO;

		if (vm.count("unpack"))
			tools_type = db_tools::TOOLS_DB_UNPACK;

		if (vm.count("pack"))
			tools_type = db_tools::TOOLS_DB_PACK;

		std::string fs_spec;

		unsigned int fs_flags = 0;
		if (vm.count("ro"))
		{
			fs_flags |= xr_file_system::FSF_READ_ONLY;
			spdlog::info("Working in read-only mode");
		}

		xr_file_system& fs = xr_file_system::instance();
		if (!fs.initialize(fs_spec, fs_flags))
		{
			spdlog::critical("Can't initialize the file system");
			return 1;
		}

		auto get_db_version = [](const variables_map& vm, const std::string& extension)
		{
			db_tools::db_version version = db_tools::DB_VERSION_AUTO;

			if (vm.count("11xx"))
				version = db_tools::DB_VERSION_1114;
			if (vm.count("2215"))
				version = db_tools::DB_VERSION_2215;
			if (vm.count("2945"))
				version = db_tools::DB_VERSION_2945;
			if (vm.count("2947ru"))
				version = db_tools::DB_VERSION_2947RU;
			if (vm.count("2947ww"))
				version = db_tools::DB_VERSION_2947WW;
			if (vm.count("xdb"))
				version = db_tools::DB_VERSION_XDB;

			if (version == db_tools::DB_VERSION_AUTO)
			{
				if (db_tools::is_xdb(extension) || db_tools::is_db(extension))
				{
					spdlog::info("Auto-detected version: xdb");
					version = db_tools::DB_VERSION_XDB;
				}
				else if (db_tools::is_xrp(extension))
				{
					spdlog::info("Auto-detected version: 1114");
					version = db_tools::DB_VERSION_1114;
				}
				else if (db_tools::is_xp(extension))
				{
					spdlog::info("Auto-detected version: 2215");
					version = db_tools::DB_VERSION_2215;
				}
			}
			return version;
		};

		switch (tools_type)
		{
			case db_tools::TOOLS_DB_UNPACK:
			{
				std::string source_path = vm["unpack"].as<std::string>();
				auto path_splitted = xr_file_system::split_path(source_path);
				std::string extension = path_splitted.extension;

				std::string destination_path = vm.count("out") ? vm["out"].as<std::string>() : "";

				db_tools::db_version version = get_db_version(vm, extension);

				if (version == db_tools::DB_VERSION_AUTO)
				{
					spdlog::error("unspecified DB format");
					break;
				}

				std::string filter;
				if(vm.count("flt"))
				{
					filter = vm["flt"].as<std::string>();
				}

				db_unpacker unpacker;
				unpacker.process(source_path, destination_path, version, filter);
				break;
			}
			case db_tools::TOOLS_DB_PACK:
			{
				std::string source_path = vm["pack"].as<std::string>();

				std::string destination_path = vm.count("out") ? vm["out"].as<std::string>() : "";
				auto path_splitted = xr_file_system::split_path(destination_path);
				std::string extension = path_splitted.extension;

				db_tools::db_version version = get_db_version(vm, extension);

				if (version == db_tools::DB_VERSION_AUTO)
				{
					spdlog::error("unspecified DB format");
					break;
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

				db_packer packer;
				packer.process(source_path, destination_path, version, xdb_ud);
				break;
			}
			default:
			{
				spdlog::info("No tools selected");
				spdlog::info("Try \"db_converter --help\" for more information");
				return 0;
			}
		}
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
