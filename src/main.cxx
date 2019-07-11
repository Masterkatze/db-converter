#include "tools_base.hxx"
#include "db_tools.hxx"
#include "xray_re/xr_file_system.hxx"
#include "xray_re/xr_log.hxx"

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
		    ("xdb_ud", "")
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

		unsigned format = tools_base::TOOLS_AUTO;

		if (vm.count("unpack"))
			format |= tools_base::TOOLS_DB;

		if (vm.count("pack"))
			format |= tools_base::TOOLS_FOLDER;

		if ((format & (format - 1)) != 0)
		{
			msg("Conflicting source formats");
			return 1;
		}

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

		//xr_log::instance().init("converter", "");

		tools_base* tools = nullptr;
		switch (format)
		{
			case tools_base::TOOLS_DB:
			{
				tools = new db_unpacker;
				break;
			}
			case tools_base::TOOLS_FOLDER:
			{
				tools = new db_packer;
				break;
			}
		}
		if (tools == nullptr)
		{
			msg("No tools selected");
			msg("Try \"db_converter --help\" for more information");
			return 0;
		}
		tools->process(vm);
		delete tools;
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
