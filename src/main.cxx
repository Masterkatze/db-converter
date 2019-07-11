#include "db_tools.hxx"
#include "xray_re/xr_file_system.hxx"
#include "xray_re/xr_log.hxx"
#include <boost/program_options.hpp>

using namespace xray_re;

void usage()
{
	printf("Usage examples:\n");
	printf(" db_converter --unpack resources.db0 --xdb --dir ~/extracted\n");
	printf(" db_converter --pack ~/dir_to_pack/ --out ~/packed.db --xdb\n\n");

	printf("Common options:\n");
	printf(" --ro            perform all the steps but do not write anything on disk\n\n");

	printf("Unpack options:\n");
	printf(" --unpack <FILE> unpack game archive\n");
	printf(" --out <PATH>    output folder name\n");
	printf(" --11xx          assume 1114/1154 archive format\n");
	printf(" --2215          assume 2215 archive format\n");
	printf(" --2945          assume 2945/2939 archive format\n");
	printf(" --2947ru        assume release version format\n");
	printf(" --2947ww        assume worldwide release version and 3120 format\n");
	printf(" --xdb           assume .xdb or .db archive format\n");
	printf(" --flt <MASK>    extract only files, filtered by mask\n\n");

	printf("Pack options:\n");
	printf(" --pack <FILE>   pack game archive\n");
	printf(" --out <PATH>    output file name\n");
	printf(" --2947ru        assume release version format\n");
	printf(" --2947ww        assume world-wide release version and 3120 format\n");
	printf(" --xdb           assume .xdb or .db archive format\n");
	printf(" --xdb_ud <FILE> attach user data from <FILE>\n");
}

bool conflicting_options_exist(const boost::program_options::variables_map& vm, std::vector<std::string> options)
{
	std::string found_option = "";
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
				msg("Conflicting options \"%s\" and \"%s\" specified", found_option.c_str(), option.c_str());
				return true;
			}
		}
	}

	return false;
}

int main(int argc, char* argv[])
{
	try
	{
		boost::program_options::options_description desc("Allowed options");
		desc.add_options()
		    ("help", "produce help message")
		    ("ro", "")
		    ("unpack", boost::program_options::value<std::string>(), "")
		    ("pack", boost::program_options::value<std::string>(), "")
		    ("11xx", "")
		    ("2215", "")
		    ("2945", "")
		    ("2947ru", "")
		    ("2947ww", "")
		    ("xdb", "")
		    ("xdb_ud", boost::program_options::value<std::string>(), "")
		    ("out", boost::program_options::value<std::string>(), "")
		    ("dir", boost::program_options::value<std::string>(), "")
		    ("flt", boost::program_options::value<std::string>(), "");

		boost::program_options::variables_map vm;
		boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
		boost::program_options::notify(vm);

		if (vm.count("help"))
		{
			usage();
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
			msg("Working in read-only mode");
		}

		xr_file_system& fs = xr_file_system::instance();
		if (!fs.initialize(fs_spec, fs_flags))
		{
			msg("Can't initialize the file system");
			return 1;
		}

		auto get_db_version = [](boost::program_options::variables_map& vm, const std::string& extension) -> db_tools::db_version
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
					msg("Auto-detected version: xdb");
					version = db_tools::DB_VERSION_XDB;
				}
				else if (db_tools::is_xrp(extension))
				{
					msg("Auto-detected version: 1114");
					version = db_tools::DB_VERSION_1114;
				}
				else if (db_tools::is_xp(extension))
				{
					msg("Auto-detected version: 2215");
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
					msg("unspecified DB format");
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
					msg("unspecified DB format");
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
				msg("No tools selected");
				msg("Try \"db_converter --help\" for more information");
				return 0;
			}
		}
	}
	catch(std::exception& e)
	{
		msg("Exception: %s", e.what());
	}
	catch(...)
	{
		msg("Unknown exception");
	}

	return 0;
}
