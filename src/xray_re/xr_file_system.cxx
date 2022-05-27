#include "xr_file_system.hxx"
#include "xr_mmap_reader_posix.hxx"
#include "xr_file_writer_posix.hxx"
#include "xr_utils.hxx"

#include <spdlog/spdlog.h>

#include <filesystem>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>

using namespace xray_re;

auto FindPathAlias(const std::vector<PathAlias>& aliases, const std::string& path)
{
	return std::find_if(aliases.begin(), aliases.end(), [&path](const PathAlias& alias) { return alias.path == path; });
}

PathAlias::PathAlias(const std::string& path, const std::string& root, const std::string& filter, const std::string& caption) :
	path(path), root(root), filter(filter), caption(caption) {}

std::string PathAlias::ToString() const
{
	return "{path=" + path + ", root=" + root + ", filter=" + filter + ", caption=" + caption + "}";
}

xr_file_system::xr_file_system() : m_is_read_only(false)
{
	add_path_alias(PA_FS_ROOT, "", "");
}

xr_file_system::~xr_file_system()
{

}

xr_file_system& xr_file_system::instance()
{
	static xr_file_system instance;
	return instance;
}

bool xr_file_system::is_read_only() const
{
	return m_is_read_only;
}

void xr_file_system::set_read_only(bool is_read_only)
{
	m_is_read_only = is_read_only;
}

xr_reader* xr_file_system::r_open(const std::string& path)
{
	spdlog::debug("r_open: path={}", path);
	try
	{
		return new xr_mmap_reader_posix(path);
	}
	catch(const std::exception& e)
	{
		spdlog::critical("Exception: {}", e.what());
	}

	return nullptr;
}

xr_reader* xr_file_system::r_open(const std::string& path, const std::string& name) const
{
	spdlog::debug("r_open: path={}, name={}", path, name);
	auto pa = FindPathAlias(m_aliases, path);
	if(pa != m_aliases.end())
	{
		return r_open(pa->root + name);
	}
	else
	{
		return nullptr;
	}
}

void xr_file_system::r_close(xr_reader *&reader)
{
	spdlog::debug("r_close");
	delete reader;
	reader = nullptr;
}

xr_writer* xr_file_system::w_open(const std::string& path, bool ignore_ro) const
{
	spdlog::debug("w_open: path={}, ignore_ro={}", path, ignore_ro);
	if(!ignore_ro && is_read_only())
	{
		return new xr_fake_writer();
	}

	auto fd = open(path.c_str(), O_RDWR | O_CREAT, 0666);

	if(fd == -1)
	{
		return nullptr;
	}

	return new xr_file_writer_posix(fd);
}

xr_writer* xr_file_system::w_open(const std::string& path, const std::string& name,  bool ignore_ro) const
{
	spdlog::debug("w_open: path={}, name={}, ignore_ro={}", path, name, ignore_ro);
	auto pa = FindPathAlias(m_aliases, path);
	if(pa != m_aliases.end())
	{
		return w_open(pa->root + name, ignore_ro);
	}
	else
	{
		return nullptr;
	}
}

void xr_file_system::w_close(xr_writer *&writer)
{
	delete writer;
	writer = nullptr;
}

bool xr_file_system::copy_file(const std::string &src_path, const std::string &src_name, const std::string &dst_path, const std::string &dst_name) const
{
	auto src_pa = FindPathAlias(m_aliases, src_path);
	if(src_pa == m_aliases.end())
	{
		return false;
	}

	auto dst_pa = FindPathAlias(m_aliases, dst_path);
	if(dst_pa == m_aliases.end())
	{
		return false;
	}

	return copy_file(src_pa->root + src_name, dst_pa->root + (dst_name.empty() ? src_name : dst_name));
}

bool xr_file_system::copy_file(const std::string& src_path, const std::string& dst_path) const
{
	if(is_read_only())
	{
		return true;
	}

	return std::filesystem::copy_file(src_path, dst_path);
}

size_t xr_file_system::file_length(const std::string& path)
{
	return std::filesystem::file_size(path);
}

uint32_t xr_file_system::file_age(const std::string& path)
{
	struct stat st{};
	if(stat(path.c_str(), &st) == 0)
	{
		return static_cast<uint32_t>(st.st_mtime);
	}

	return 0;
}

bool xr_file_system::file_exist(const std::string& path)
{
	return std::filesystem::exists(path) && std::filesystem::is_regular_file(path);
}

bool xr_file_system::folder_exist(const std::string& path)
{
	return std::filesystem::exists(path) && std::filesystem::is_directory(path);
}

bool xr_file_system::create_path(const std::string& path) const
{
	if(is_read_only() || std::filesystem::exists(path))
	{
		return true;
	}

	return std::filesystem::create_directories(path);
}

bool xr_file_system::create_folder(const std::string& path) const
{
	if(is_read_only() || std::filesystem::exists(path))
	{
		return true;
	}

	return std::filesystem::create_directory(path);
}

bool xr_file_system::resolve_path(const std::string& path, const std::string& name, std::string& full_path) const
{
	auto pa = FindPathAlias(m_aliases, path);
	if(pa == m_aliases.end())
	{
		return false;
	}

	full_path = pa->root;

	if(name.empty())
	{
		full_path.append(name);
	}

	return true;
}

void xr_file_system::append_path_separator(std::string& path)
{
	if(!path.empty() && path.back() != '/')
	{
		path += '/';
	}
}

SplitPath xr_file_system::split_path(const std::string& path)
{
	std::filesystem::path fs_path(path);

	return {fs_path.parent_path(), fs_path.stem(), fs_path.extension()};
}

std::string xr_file_system::current_path()
{
	return std::filesystem::current_path();
}

PathAlias& xr_file_system::add_path_alias(const std::string& path, const std::string& root, const std::string& add)
{
	spdlog::debug("add_path_alias: path={}, root={}, add={}", path, root, add);

	std::string new_root;

	auto pa = FindPathAlias(m_aliases, path);
	if(pa != m_aliases.end())
	{
		new_root = pa->root;
		spdlog::debug("add_path_alias: path alias found, root=\"{}\"", new_root);
	}
	else
	{
		new_root = root;
		append_path_separator(new_root);
		spdlog::debug("add_path_alias: path alias not found, root=\"{}\"", new_root);
	}

	new_root += add;
	append_path_separator(new_root);

	spdlog::debug("add_path_alias: adding new alias, path={}, root={}", path, new_root);
	return m_aliases.emplace_back(path, new_root, "", "");
}
