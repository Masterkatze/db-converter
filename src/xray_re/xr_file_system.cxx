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

xr_file_system::xr_file_system() : m_is_read_only(false)
{
	add_path_alias(PA_FS_ROOT, "", "");
}

xr_file_system::~xr_file_system()
{
	delete_elements(m_aliases);
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

xr_reader* xr_file_system::r_open(const std::string& path)
{
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
	return r_open(find_path_alias(path)->root + name);
}

void xr_file_system::r_close(xr_reader *&reader)
{
	delete reader;
	reader = nullptr;
}

xr_writer* xr_file_system::w_open(const std::string& path, bool ignore_ro) const
{
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
	return w_open(find_path_alias(path)->root + name, ignore_ro);
}

void xr_file_system::w_close(xr_writer *&writer)
{
	delete writer;
	writer = nullptr;
}

bool xr_file_system::copy_file(const std::string &src_path, const std::string &src_name, const std::string &dst_path, const std::string &tgt_name) const
{
	auto src_pa = find_path_alias(src_path);
	if(!src_pa )
	{
		return false;
	}

	auto tgt_pa = find_path_alias(dst_path);
	if(!tgt_pa)
	{
		return false;
	}

	return copy_file(src_pa->root + src_name, tgt_pa->root + (tgt_name.empty() ? src_name : tgt_name));
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

const char* xr_file_system::resolve_path(const std::string& path) const
{
	return find_path_alias(path)->root.c_str();
}

bool xr_file_system::resolve_path(const std::string& path, const std::string& name, std::string& full_path) const
{
	auto pa = find_path_alias(path);
	if(pa == nullptr)
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

void xr_file_system::update_path(const std::string& path, const std::string& root, const std::string& add)
{
	PathAlias *new_pa;
	for(auto it = m_aliases.begin(), end = m_aliases.end(); it != end; ++it)
	{
		if((*it)->path == path)
		{
			new_pa = *it;
			goto found_or_created;
		}
	}
	new_pa = new PathAlias;
	new_pa->path = path;
	m_aliases.push_back(new_pa);

found_or_created:
	auto pa = find_path_alias(root);
	if(pa)
	{
		new_pa->root = pa->root;
	}
	else
	{
		new_pa->root = root;
		append_path_separator(new_pa->root);
	}
	new_pa->root += add;
	append_path_separator(new_pa->root);
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

const PathAlias* xr_file_system::find_path_alias(const std::string& path) const
{
//	return *std::find_if(m_aliases.begin(), m_aliases.end(), [&path](xr_file_system::path_alias* alias) {
//		return alias->path == path;
//	});

	for(const auto& alias : m_aliases)
	{
		if(alias->path == path)
		{
			return alias;
		}
	}

	return nullptr;
}

PathAlias* xr_file_system::add_path_alias(const std::string& path, const std::string& root, const std::string& add)
{
	spdlog::debug("add_path_alias: path={}, root={}, add={}", path, root, add);
	auto pa = find_path_alias(path);

	assert(!pa);

	if(pa)
	{
		spdlog::debug("add_path_alias: path \"{}\" not found", path);
		return nullptr;
	}

	auto new_pa = new PathAlias;
	m_aliases.push_back(new_pa);
	new_pa->path = path;

	pa = find_path_alias(root);
	if(pa)
	{
		new_pa->root = pa->root;
		spdlog::debug("add_path_alias: path alias found, root=\"{}\"", new_pa->root);
	}
	else
	{
		new_pa->root = root;
		append_path_separator(new_pa->root);
		spdlog::debug("add_path_alias: path alias not found, root=\"{}\"", new_pa->root);
	}

	new_pa->root += add;
	append_path_separator(new_pa->root);
	spdlog::debug("add_path_alias: returning path alias \"{}\"", new_pa->ToString());
	return new_pa;
}
