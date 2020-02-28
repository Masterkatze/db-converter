#include "db_tools.hxx"
#include "spdlog/spdlog.h"
#include "xray_re/xr_scrambler.hxx"
#include "xray_re/xr_lzhuf.hxx"
#include "xray_re/xr_file_system.hxx"
#include "xray_re/xr_utils.hxx"
#include "lzo/minilzo.h"
#include "crc32/crc32.hxx"
#include <cstring>
#include <string>
#include <algorithm>
#include <filesystem>
#include <errno.h>

using namespace xray_re;

bool db_tools::m_debug = false;

bool db_tools::is_xrp(const std::string& extension)
{
	return extension == ".xrp";
}

bool db_tools::is_xp(const std::string& extension)
{
	return (extension.size() == 3 && extension == ".xp") ||
	       (extension.size() == 4 && extension.compare(0, 3, ".xp") == 0 && std::isalnum(extension[3]));
}

bool db_tools::is_xdb(const std::string& extension)
{
	return (extension.size() == 3 && extension == ".xdb") ||
	       (extension.size() == 4 && extension.compare(0, 3, ".xdb") == 0 && std::isalnum(extension[4]));
}

bool db_tools::is_db(const std::string& extension)
{
	return (extension.size() == 3 && extension == ".db") ||
	       (extension.size() == 4 && extension.compare(0, 3, ".db") == 0 && std::isalnum(extension[3]));
}

bool db_tools::is_known(const std::string& extension)
{
	return is_db(extension) || is_xdb(extension) || is_xrp(extension) || is_xp(extension);
}

void db_tools::set_debug(const bool value)
{
	m_debug = value;
}

static bool write_file(xr_file_system& fs, const std::string& path, const void *data, size_t size)
{
	xr_writer *w = fs.w_open(path);
	if (w)
	{
		w->w_raw(data, size);
		fs.w_close(w);

		return true;
	}

	return false;
}

void db_unpacker::process(std::string& source_path, std::string& destination_path, db_version& version, std::string& filter)
{
	if(version == DB_VERSION_AUTO)
	{
		spdlog::error("unspecified DB format");
		return;
	}

	if(source_path.empty())
	{
		spdlog::error("Missing source file path");
		return;
	}

	if(!xr_file_system::file_exist(source_path))
	{
		spdlog::error("File \"{}\" doesn't exist", source_path);
		return;
	}

	auto path_splitted = xr_file_system::split_path(source_path);

	std::string output_folder = destination_path.empty() ? path_splitted.folder : destination_path;
	std::string extension = path_splitted.extension;

	xr_file_system& fs = xr_file_system::instance();
	xr_reader *reader_full = fs.r_open(source_path);
	if (reader_full == nullptr)
	{
		spdlog::error("can't load {}", source_path);
		return;
	}

	if (fs.create_path(output_folder))
	{
		xr_file_system::append_path_separator(output_folder);

		xr_scrambler scrambler;
		xr_reader *reader_chunk = nullptr;

		if ((reader_chunk = reader_full->open_chunk(DB_CHUNK_USERDATA)) != nullptr)
		{
			std::string path = destination_path + "_userdata.ltx";
			write_file(fs, path, reader_chunk->data(), reader_chunk->size());
			reader_full->close_chunk(reader_chunk);
		}

		switch (version)
		{
			case DB_VERSION_1114:
			case DB_VERSION_2215:
			case DB_VERSION_2945:
			case DB_VERSION_XDB:
			{
				reader_chunk = reader_full->open_chunk(DB_CHUNK_HEADER);
				break;
			}
			case DB_VERSION_2947RU:
			{
				scrambler.init(xr_scrambler::CC_RU);
				reader_chunk = reader_full->open_chunk(DB_CHUNK_HEADER, scrambler);
				break;
			}
			case DB_VERSION_2947WW:
			{
				scrambler.init(xr_scrambler::CC_WW);
				reader_chunk = reader_full->open_chunk(DB_CHUNK_HEADER, scrambler);
				break;
			}
			default:
			{
				spdlog::error("unknown DB format");
				return;
			}
		}

		if (reader_chunk)
		{
			const uint8_t *data_full = static_cast<const uint8_t*>(reader_full->data());
			switch (version)
			{
				case DB_VERSION_1114:
				{
					extract_1114(output_folder, filter, reader_chunk, data_full);
					break;
				}
				case DB_VERSION_2215:
				{
					extract_2215(output_folder, filter, reader_chunk, data_full);
					break;
				}
				case DB_VERSION_2945:
				{
					extract_2945(output_folder, filter, reader_chunk, data_full);
					break;
				}
				case DB_VERSION_2947RU:
				case DB_VERSION_2947WW:
				case DB_VERSION_XDB:
				{
					extract_2947(output_folder, filter, reader_chunk, data_full);
					break;
				}
				default:
				{
					spdlog::error("unknown DB format");
					return;
				}
			}
			reader_full->close_chunk(reader_chunk);
		}
	}
	else
	{
		spdlog::error("can't create {}", output_folder);
	}
	fs.r_close(reader_full);
}

static bool write_file(xr_file_system& fs, const std::string& path, const uint8_t *data, uint32_t size_real, uint32_t size_compressed)
{
	if (size_real != size_compressed)
	{
		lzo_uint size = size_real;
		uint8_t *temp = new uint8_t[size];
		if (lzo1x_decompress_safe(data, size_compressed, temp, &size, nullptr) != LZO_E_OK)
		{
			delete[] temp;
			return false;
		}
		data = temp;
		size_real = uint32_t(size & UINT32_MAX);
	}

	if(!write_file(fs, path, data, size_real))
	{
		std::string folder = xr_file_system::split_path(path).folder;

		if(xr_file_system::folder_exist(folder))
		{
			spdlog::error("Failed to open file \"{}\": {} (errno={}) ", path, strerror(errno), errno);
			return false;
		}
		else
		{
			if(!fs.create_path(folder))
			{
				spdlog::error("Failed to create folder {}", folder);
				return false;
			}
		}

		if((!fs.read_only() && !write_file(fs, path, data, size_real)) || (fs.read_only() && !fs.file_exist(path)))
		{
			spdlog::error("Failed to open file \"{}\": {} (errno={}) ", path, strerror(errno), errno);
			return false;
		}
	}

	if (size_real != size_compressed)
		delete[] data;

	return true;
}

void db_unpacker::extract_1114(const std::string& prefix, const std::string& mask, xr_reader *s, const uint8_t *data)
{
	xr_file_system& fs = xr_file_system::instance();
	for (std::string temp, path, folder; !s->eof(); )
	{
		s->r_sz(temp);
		std::replace(path.begin(), path.end(), '\\', '/');

		unsigned uncompressed = s->r_u32();
		unsigned offset = s->r_u32();
		unsigned size = s->r_u32();

		if (mask.length() > 0 && offset != 0 && path.find(mask) == std::string::npos)
		{
			continue;
		}

		if (m_debug && fs.read_only())
		{
			spdlog::debug("{}", temp);
			spdlog::debug("  offset: {}", offset);

			if (uncompressed)
				spdlog::debug("  size (real): {}", size);
			else
				spdlog::debug("  size (compressed): {}", size);
		}
		else
		{
			path = prefix;
			auto path_splitted = xr_file_system::split_path(path.append(temp));

			std::string folder = path_splitted.folder;

			if (!xr_file_system::folder_exist(folder))
				fs.create_path(folder);

			if (uncompressed)
			{
				write_file(fs, path, data + offset, size);
			}
			else
			{
				size_t real_size;
				uint8_t *p;
				xr_lzhuf::decompress(p, real_size, data + offset, size);

				if (real_size)
					write_file(fs, path, p, real_size);

				free(p);
			}
		}
	}
}

void db_unpacker::extract_2215(const std::string& prefix, const std::string& mask, xr_reader *s, const uint8_t *data)
{
	xr_file_system& fs = xr_file_system::instance();
	for (std::string path; !s->eof(); )
	{
		s->r_sz(path);
		std::replace(path.begin(), path.end(), '\\', '/');

		unsigned offset = s->r_u32();
		unsigned size_real = s->r_u32();
		unsigned size_compressed = s->r_u32();

		if (mask.length() > 0 && offset != 0 && path.find(mask) == std::string::npos)
		{
			continue;
		}

		if (m_debug && fs.read_only())
		{
			spdlog::debug("{}", path);
			spdlog::debug("  offset: {}", offset);
			spdlog::debug("  size (real): {}", size_real);
			spdlog::debug("  size (compressed): {}", size_compressed);
		}
		else if (offset == 0)
		{
			fs.create_folder(prefix + path);
		}
		else
		{
			write_file(fs, prefix + path, data + offset, size_real, size_compressed);
		}
	}
}

void db_unpacker::extract_2945(const std::string& prefix, const std::string& mask, xr_reader *s, const uint8_t *data)
{
	xr_file_system& fs = xr_file_system::instance();
	for (std::string path; !s->eof(); )
	{
		s->r_sz(path);
		std::replace(path.begin(), path.end(), '\\', '/');

		unsigned crc = s->r_u32();
		unsigned offset = s->r_u32();
		unsigned size_real = s->r_u32();
		unsigned size_compressed = s->r_u32();

		if (mask.length() > 0 && offset != 0 && path.find(mask) == std::string::npos)
		{
			continue;
		}

		if (m_debug && fs.read_only())
		{
			spdlog::debug("{}", path);
			spdlog::debug("  crc: {0:#x}", crc);
			spdlog::debug("  offset: {}", offset);
			spdlog::debug("  size (real): {}", size_real);
			spdlog::debug("  size (compressed): {}", size_compressed);
		}
		else if (offset == 0)
		{
			fs.create_folder(prefix + path);
		}
		else
		{
			write_file(fs, prefix + path, data + offset, size_real, size_compressed);
		}
	}
}

void db_unpacker::extract_2947(const std::string& prefix, const std::string& mask, xr_reader *reader, const uint8_t *data)
{
	xr_file_system& fs = xr_file_system::instance();
	while (!reader->eof())
	{
		unsigned int name_size = reader->r_u16() - 16;              // unsigned 2 bytes <─┐
		unsigned int size_real = reader->r_u32();                   // unsigned 4 bytes   │
		unsigned int size_compressed = reader->r_u32();             // unsigned 4 bytes   │
		uint32_t crc = reader->r_u32();                             // unsigned 4 bytes   │
		std::string name(reader->skip<char>(name_size), name_size); // string   N bytes >─┘
		uint32_t offset = reader->r_u32();                          // unsigned 4 bytes

		std::replace(name.begin(), name.end(), '\\', '/');
		std::string path = prefix + name;

		if (mask.length() > 0 && offset != 0 && path.find(mask) == std::string::npos)
		{
			continue;
		}

		if (m_debug && fs.read_only())
		{
			spdlog::debug("{}", name);
			spdlog::debug("  offset: {}", offset);

			if(size_real != size_compressed)
			{
				spdlog::debug("  size (real): {}", size_real);
				spdlog::debug("  size (compressed): {}", size_compressed);
			}
			else
			{
				spdlog::debug("  size: {}", size_real);
			}

			spdlog::debug("  crc: {0:#x}", crc);
		}

		if (offset == 0)
		{
			fs.create_path(path);
			spdlog::info("{}", path);
		}
		else
		{
			write_file(fs, path, data + offset, size_real, size_compressed);
			static std::size_t file_counter = 0;
			spdlog::info("[{}] {}", ++file_counter, path);
		}
	}
}

db_packer::~db_packer()
{
	delete_elements(m_files);
}

void db_packer::process(std::string& source_path, std::string& destination_path, db_version& version, std::string& xdb_ud)
{
	if(source_path.empty())
	{
		spdlog::error("Missing source directory path");
		return;
	}

	if(destination_path.empty())
	{
		spdlog::error("Missing destination file path");
		return;
	}

	if (!xr_file_system::folder_exist(source_path))
	{
		spdlog::error("can't find {}", source_path);
		return;
	}

	if(version == DB_VERSION_AUTO)
	{
		spdlog::error("Unspecified DB format");
		return;
	}

	if(version == DB_VERSION_1114 || version == DB_VERSION_2215 || version == DB_VERSION_2945)
	{
		spdlog::error("Unsupported DB format");
		return;
	}

	xr_file_system::append_path_separator(m_root);

	auto path_splitted = xr_file_system::split_path(destination_path);
	std::string extension = path_splitted.extension;

	xr_file_system& fs = xr_file_system::instance();
	m_archive = fs.w_open(destination_path);
	if (m_archive == nullptr)
	{
		spdlog::error("Can't load {}", destination_path);
		return;
	}

	if (version == DB_VERSION_XDB && !xdb_ud.empty())
	{
		if (xr_reader *reader = fs.r_open(xdb_ud))
		{
			m_archive->open_chunk(DB_CHUNK_USERDATA);
			m_archive->w_raw(reader->data(), reader->size());
			m_archive->close_chunk();
			fs.r_close(reader);
		}
		else
		{
			spdlog::error("can't load {}", xdb_ud);
		}
	}

	m_archive->open_chunk(DB_CHUNK_DATA);
	xr_file_system::append_path_separator(source_path);
	m_root = source_path;
	process_folder(source_path);
	m_archive->close_chunk();

	auto w = new xr_memory_writer;

//	spdlog::info("folders: ");
//	for (const auto& folder : m_folders)
//	{
//		w->w_size_u16(folder.size() + 16);
//		w->w_u32(0);
//		w->w_u32(0);
//		w->w_u32(0);
//		w->w_raw(folder.data(), folder.size());
//		spdlog::info("  {}", folder);
//		w->w_u32(0);
//	}

	spdlog::info("files: ");
	for (const auto& file : m_files)
	{
		std::replace(file->path.begin(), file->path.end(), '/', '\\');
		w->w_size_u16(file->path.size() + 16);
		w->w_size_u32(file->size_real);
		w->w_size_u32(file->size_compressed);
		w->w_u32(file->crc);
		w->w_raw(file->path.data(), file->path.size());
		spdlog::info("  {}", file->path);
		w->w_size_u32(file->offset);
	}

	uint8_t *data = nullptr;
	size_t size = 0;
	xr_lzhuf::compress(data, size, w->data(), w->tell());
	delete w;

	if (version == DB_VERSION_2947RU)
	{
		xr_scrambler scrambler(xr_scrambler::CC_RU);
		scrambler.encrypt(data, data, size);
	}
	else if (version == DB_VERSION_2947WW)
	{
		xr_scrambler scrambler(xr_scrambler::CC_WW);
		scrambler.encrypt(data, data, size);
	}

	m_archive->open_chunk(DB_CHUNK_HEADER | CHUNK_COMPRESSED);
	m_archive->w_raw(data, size);
	m_archive->close_chunk();

	delete data;
	fs.w_close(m_archive);
}

void db_packer::process_folder(const std::string& path)
{
	auto root_path = std::filesystem::path(path);

	std::vector<std::filesystem::directory_entry> folders;
	std::vector<std::filesystem::directory_entry> files;

	for (auto& entry : std::filesystem::recursive_directory_iterator(path))
	{
		if(entry.is_directory())
		{
			folders.emplace_back(entry);
		}
		else if(entry.is_regular_file())
		{
			files.emplace_back(entry);
		}
	}

	std::sort(folders.begin(), folders.end(), [] (std::filesystem::directory_entry lhs, std::filesystem::directory_entry rhs)
	{
		return lhs.path() < rhs.path();
	});

	for(auto folder : folders)
	{
		auto entry_path = std::filesystem::path(folder);
		std::string relative_path = std::filesystem::relative(entry_path, root_path);
		m_folders.push_back(relative_path);
	}

	std::sort(files.begin(), files.end(), [] (std::filesystem::directory_entry lhs, std::filesystem::directory_entry rhs)
	{
		return lhs.path() < rhs.path();
	});

	for(auto file : files)
	{
		auto entry_path = std::filesystem::path(file);
		std::string relative_path = std::filesystem::relative(entry_path, root_path);
		process_file(relative_path);
	}
}

void db_packer::process_file(const std::string& path)
{
	constexpr bool compress_files = false;

	if constexpr (compress_files)
	{
		xr_file_system& fs = xr_file_system::instance();
		auto reader = fs.r_open(m_root + path);
		if (reader)
		{
			const uint8_t *data = static_cast<const uint8_t*>(reader->data());
			size_t offset = m_archive->tell();
			size_t size = reader->size();

			uint8_t *data_compressed = nullptr;
			size_t size_compressed = 0;
			xr_lzhuf::compress(data_compressed, size_compressed, data, size);
			spdlog::info("{}->{} {}", size, size_compressed, path);

			uint32_t crc = crc32(data_compressed, size_compressed);
			m_archive->w_raw(data_compressed, size_compressed);
			fs.r_close(reader);

			std::string path_lowercase = path;
			std::transform(path_lowercase.begin(), path_lowercase.end(), path_lowercase.begin(), [](unsigned char c) { return std::tolower(c); });

			db_file *file = new db_file;
			file->path = path_lowercase;
			file->crc = crc;
			file->offset = offset;
			file->size_real = size;
			file->size_compressed = size_compressed;
			m_files.push_back(file);
		}
	}
	else
	{
		xr_file_system& fs = xr_file_system::instance();
		auto reader = fs.r_open(m_root + path);
		if (reader)
		{
			size_t offset = m_archive->tell();
			size_t size = reader->size();
			uint32_t crc = crc32(reader->data(), size);
			m_archive->w_raw(reader->data(), size);
			fs.r_close(reader);

			std::string path_lowercase = path;
			std::transform(path_lowercase.begin(), path_lowercase.end(), path_lowercase.begin(), [](unsigned char c) { return std::tolower(c); });

			auto file = new db_file;
			file->path = path_lowercase;
			file->crc = crc;
			file->offset = offset;
			file->size_real = size;
			file->size_compressed = size;
			m_files.push_back(file);
		}
	}
}
