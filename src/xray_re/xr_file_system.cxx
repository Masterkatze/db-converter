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

xr_file_system::xr_file_system() : is_read_only(false) {}

xr_file_system::~xr_file_system()
{
	delete_elements(m_aliases);
}

xr_file_system& xr_file_system::instance()
{
	static xr_file_system instance;
	return instance;
}

bool xr_file_system::read_only() const { return is_read_only; }

bool xr_file_system::initialize(const std::string& fs_spec, bool is_read_only)
{
	if(!fs_spec.empty() && fs_spec[0] != '\0')
	{
		auto reader = r_open(fs_spec);
		if(!reader)
		{
			return false;
		}

		if(parse_fs_spec(*reader))
		{
			auto path_splitted = xr_file_system::split_path(fs_spec);
			std::string folder = path_splitted.folder;
			add_path_alias(PA_FS_ROOT, folder, "");
		}
		r_close(reader);
	}
	else
	{
		add_path_alias(PA_FS_ROOT, "", "");
	}

	this->is_read_only = is_read_only;
	return !m_aliases.empty();
}

xr_reader* xr_file_system::r_open(const std::string& path)
{
	try
	{
		return new xr_mmap_reader_posix(/*fd, data, file_size, mem_size*/path);
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
	if(!ignore_ro && read_only())
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
	if(read_only())
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
	if(read_only() || std::filesystem::exists(path))
	{
		return true;
	}

	return std::filesystem::create_directories(path);
}

bool xr_file_system::create_folder(const std::string& path) const
{
	if(read_only() || std::filesystem::exists(path))
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
	path_alias *new_pa;
	for(auto it = m_aliases.begin(), end = m_aliases.end(); it != end; ++it)
	{
		if((*it)->path == path)
		{
			new_pa = *it;
			goto found_or_created;
		}
	}
	new_pa = new path_alias;
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

split_path_t xr_file_system::split_path(const std::string& path)
{
	std::filesystem::path fs_path(path);

	return {fs_path.parent_path(), fs_path.stem(), fs_path.extension()};
}

std::string xr_file_system::current_path()
{
	return std::filesystem::current_path();
}

const xr_file_system::path_alias* xr_file_system::find_path_alias(const std::string& path) const
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

xr_file_system::path_alias* xr_file_system::add_path_alias(const std::string& path, const std::string& root, const std::string& add)
{
	auto pa = find_path_alias(path);

	assert(!pa);

	if(pa)
	{
		return nullptr;
	}

	auto new_pa = new path_alias;
	m_aliases.push_back(new_pa);
	new_pa->path = path;

	pa = find_path_alias(root);
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
	return new_pa;
}

static inline const char* next_line(const char *p, const char *end)
{
	while(p < end && *p != '\n')
	{
		p++;
	}
	return p;
}

static inline const char* read_alias(const char *p, const char *end)
{
	if(p >= end || *p++ != '$')
	{
		return nullptr;
	}

	if(p >= end || (!std::isalnum(*p) && *p != '_'))
	{
		return nullptr;
	}

	for(++p; p < end; p++)
	{
		int c = *p;
		if(c == '$')
		{
			return p + 1;
		}

		if(!std::isalnum(c) && c != '_')
		{
			break;
		}
	}
	return nullptr;
}

static inline const char* skip_ws(const char *p, const char *end)
{
	while(p < end)
	{
		int c = *p;

		if(c != ' ' && c != '\t')
		{
			break;
		}

		++p;
	}
	return p;
}

static inline const char* read_value(const char *&_p, const char *end)
{
	auto p = skip_ws(_p, end);
	_p = p;
	decltype(p) last_ws = nullptr;
	while(p < end)
	{
		int c = *p;
		if(c == ' ' || c =='\t')
		{
			if(!last_ws)
			{
				last_ws = p;
			}
		}
		else if(c == '\n' || c == '\r' || c == '|')
		{
			if(!last_ws)
			{
				last_ws = p;
			}

			break;
		}
		else
		{
			last_ws = nullptr;
		}
		++p;
	}
	return last_ws ? last_ws : p;
}

bool xr_file_system::parse_fs_spec(xr_reader& reader)
{
	auto p = reader.pointer<const char>();
	auto end = p + reader.size();
	std::string alias;
	std::array<std::string, 4> values;
	for(unsigned line = 1; p < end; p = next_line(p, end), ++line)
	{
		int c = *p;
		if(c == '$')
		{
			auto last = read_alias(p, end);
			if(last == nullptr)
			{
				spdlog::error("can't parse line {}", line);
				return false;
			}
			alias.assign(p, last);

			p = skip_ws(last, end);
			if(p == end || *p++ != '=')
			{
				spdlog::error("can't parse line {}", line);
				return false;
			}

			int i = -2;
			while(i < 4)
			{
				last = read_value(p, end);
				if(i < 0 && (last == end || *last != '|'))
				{
					spdlog::error("can't parse line {}", line);
					return false;
				}
				if(i >= 0)
				{
					values.at(static_cast<std::size_t>(i)).assign(p, last);
				}

				p = last + 1;
				++i;

				if(p == end || *last != '|')
				{
					break;
				}
			}

			assert(i > 0);

			if(i < 2)
			{
				values.at(1).clear();
			}

			auto pa = add_path_alias(alias, values.at(0), values.at(1));
			if(pa == nullptr)
			{
				spdlog::error("can't parse line {}", line);
				return false;
			}

			if(i > 2)
			{
				pa->filter = values.at(2);
			}
			if(i > 3)
			{
				pa->caption = values.at(3);
			}
		}
		else if(c != ';' && !std::isspace(c))
		{
			spdlog::error("can't parse line {}", line);
			return false;
		}
	}
	return true;
}
