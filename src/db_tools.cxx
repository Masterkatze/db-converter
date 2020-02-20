#define NOMINMAX
#include <cstring>
#include "db_tools.hxx"
#include "xray_re/xr_scrambler.hxx"
#include "xray_re/xr_lzhuf.hxx"
#include "xray_re/xr_file_system.hxx"
#include "xray_re/xr_utils.hxx"
#include "xray_re/xr_log.hxx"
#include "lzo/minilzo.h"
#include "crc32/crc32.hxx"
#include <string>
#include <algorithm>
#include <filesystem>

#define DB_DEBUG false

using namespace xray_re;

constexpr const bool DB_DEBUG = false;

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

db_unpacker::~db_unpacker() {}

void db_unpacker::process(std::string& source_path, std::string& destination_path, db_version& version, std::string& filter)
{
	if(source_path.empty())
	{
		msg("Missing source file path");
		return;
	}

	auto path_splitted = xr_file_system::split_path(source_path);

	std::string output_folder = destination_path.empty() ? path_splitted.folder : destination_path;
	std::string extension = path_splitted.extension;

	xr_file_system& fs = xr_file_system::instance();
	xr_reader *r = fs.r_open(source_path);
	if (r == nullptr)
	{
		msg("can't load %s", source_path.c_str());
		return;
	}

	if (fs.create_path(output_folder))
	{
		xr_file_system::append_path_separator(output_folder);

		xr_scrambler scrambler;
		xr_reader *s = nullptr;
		switch (version)
		{
			case DB_VERSION_AUTO:
			{
				msg("unspecified DB format");
				break;
			}
			case DB_VERSION_1114:
			case DB_VERSION_2215:
			case DB_VERSION_2945:
			case DB_VERSION_XDB:
			{
				s = r->open_chunk(DB_CHUNK_HEADER);
				break;
			}
			case DB_VERSION_2947RU:
			{
				scrambler.init(xr_scrambler::CC_RU);
				s = r->open_chunk(DB_CHUNK_HEADER, scrambler);
				break;
			}
			case DB_VERSION_2947WW:
			{
				scrambler.init(xr_scrambler::CC_WW);
				s = r->open_chunk(DB_CHUNK_HEADER, scrambler);
				break;
			}
		}
		if (s)
		{
			const uint8_t *data = static_cast<const uint8_t*>(r->data());
			switch (version)
			{
				case DB_VERSION_AUTO:
				{
					msg("unspecified DB format");
					break;
				}
				case DB_VERSION_1114:
				{
					extract_1114(output_folder, filter, s, data);
					break;
				}
				case DB_VERSION_2215:
				{
					extract_2215(output_folder, filter, s, data);
					break;
				}
				case DB_VERSION_2945:
				{
					extract_2945(output_folder, filter, s, data);
					break;
				}
				case DB_VERSION_2947RU:
				case DB_VERSION_2947WW:
				case DB_VERSION_XDB:
				{
					extract_2947(output_folder, filter, s, data);
					break;
				}
			}
			r->close_chunk(s);
		}

//		if (s = r->open_chunk(DB_CHUNK_USERDATA))
//		{
//			auto path_splitted = xr_file_system::split_path(source_path);

//			std::string file_name = path_splitted.name; // TODO: folder instead of name?

//			write_file(fs, file_name.append("_userdata.ltx"), s->data(), s->size());
//			r->close_chunk(s);
//		}
	}
	else
	{
		msg("can't create %s", output_folder.c_str());
	}
	fs.r_close(r);
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

	bool success = write_file(fs, path, data, size_real);
	if (!success)
	{
		auto path_splitted = xr_file_system::split_path(path);

		std::string folder = path_splitted.folder;

		if(!xr_file_system::folder_exist(folder))
		{
			fs.create_path(folder);
		}

		success = write_file(fs, path, data, size_real);
		//success = !fs.folder_exist(folder) && fs.create_path(folder) && write_file(fs, path, data, size_real);
	}
	if (size_real != size_compressed)
		delete[] data;

	if (!success)
		msg("can't write %s", path.c_str());

	return success;
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

		if (DB_DEBUG && fs.read_only())
		{
			msg("%s", temp.c_str());
			msg("  offset: %u", offset);

			if (uncompressed)
				msg("  size (real): %u", size);
			else
				msg("  size (compressed): %u", size);
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

		if (DB_DEBUG && fs.read_only())
		{
			msg("%s", path.c_str());
			msg("  offset: %u", offset);
			msg("  size (real): %u", size_real);
			msg("  size (compressed): %u", size_compressed);
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

		if (DB_DEBUG && fs.read_only())
		{
			msg("%s", path.c_str());
			msg("  crc: 0x%8.8x", crc);
			msg("  offset: %u", offset);
			msg("  size (real): %u", size_real);
			msg("  size (compressed): %u", size_compressed);
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

void db_unpacker::extract_2947(const std::string& prefix, const std::string& mask, xr_reader *s, const uint8_t *data)
{
	xr_file_system& fs = xr_file_system::instance();
	for (std::string path; !s->eof(); )
	{
		size_t name_size = s->r_u16() - 16;
		unsigned size_real = s->r_u32();
		unsigned size_compressed = s->r_u32();
		uint32_t crc = s->r_u32();
		path.assign(prefix);

		std::string name(s->skip<char>(name_size), name_size);
		std::replace(name.begin(), name.end(), '\\', '/');

		path.append(name);
		uint32_t offset = s->r_u32();

		if (mask.length() > 0 && offset != 0 && path.find(mask) == std::string::npos)
		{
			continue;
		}

		if (DB_DEBUG && fs.read_only())
		{
			msg("%s", std::string(name, name_size).c_str());
			msg("  offset: %u", offset);
			msg("  size (real): %u", size_real);
			msg("  size (compressed): %u", size_compressed);
			msg("  crc: 0x%8.8" PRIx32, crc);
		}
		else if (offset == 0)
		{
			fs.create_path(path);
		}
		else
		{
			write_file(fs, path, data + offset, size_real, size_compressed);
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
		msg("Missing source directory path");
		return;
	}

	if(destination_path.empty())
	{
		msg("Missing destination file path");
		return;
	}

	if (!xr_file_system::folder_exist(source_path))
	{
		msg("can't find %s", source_path.c_str());
		return;
	}

	if(version == DB_VERSION_AUTO)
	{
		msg("Unspecified DB format");
		return;
	}

	if(version == DB_VERSION_1114 || version == DB_VERSION_2215 || version == DB_VERSION_2945)
	{
		msg("Unsupported DB format");
		return;
	}

	xr_file_system::append_path_separator(m_root);

	auto path_splitted = xr_file_system::split_path(destination_path);
	std::string extension = path_splitted.extension;

	xr_file_system& fs = xr_file_system::instance();
	m_archive = fs.w_open(destination_path);
	if (m_archive == nullptr)
	{
		msg("Can't load %s", destination_path.c_str());
		return;
	}

	if (version == DB_VERSION_XDB && !xdb_ud.empty())
	{
		if (xr_reader *r = fs.r_open(xdb_ud))
		{
			m_archive->open_chunk(DB_CHUNK_USERDATA);
			m_archive->w_raw(r->data(), r->size());
			m_archive->close_chunk();
			fs.r_close(r);
		}
		else
		{
			msg("can't load %s", xdb_ud.c_str());
		}
	}

	m_archive->open_chunk(DB_CHUNK_DATA);
	xr_file_system::append_path_separator(source_path);
	m_root = source_path;
	process_folder(source_path);
	m_archive->close_chunk();

	auto w = new xr_memory_writer;

	msg("folders:");
	std::sort(m_folders.begin(), m_folders.end());
	for (const auto& folder : m_folders)
	{
		w->w_size_u16(folder.size() + 16);
		w->w_u32(0);
		w->w_u32(0);
		w->w_u32(0);
		w->w_raw(folder.data(), folder.size());
		msg("  %s", folder.c_str());
		w->w_u32(0);
	}

	msg("files: ");

	std::sort(m_files.begin(), m_files.end(), [] (const db_file *lhs, const db_file *rhs)
	{
		return lhs->path < rhs->path;
	});

	for (const auto& file : m_files)
	{
		w->w_size_u16(file->path.size() + 16);
		w->w_size_u32(file->size_real);
		w->w_size_u32(file->size_compressed);
		w->w_u32(file->crc);
		w->w_raw(file->path.data(), file->path.size());
		msg("  %s", file->path.c_str());
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

	m_archive->open_chunk(DB_CHUNK_HEADER | xr_reader::CHUNK_COMPRESSED);
	m_archive->w_raw(data, size);
	m_archive->close_chunk();
	delete data;
	fs.w_close(m_archive);
}

void db_packer::process_folder(const std::string& path)
{
	auto root_path = std::filesystem::path(path);

	for (const auto& entry : std::filesystem::recursive_directory_iterator(path))
	{
		auto entry_path = std::filesystem::path(entry);
		std::string relative_path = std::filesystem::relative(entry_path, root_path);

		if(entry.is_directory())
		{
			m_folders.push_back(relative_path);
		}
		else if(entry.is_regular_file())
		{
			process_file(relative_path);
		}
	}
}

void db_packer::process_file(const std::string& path)
{
//	xr_file_system& fs = xr_file_system::instance();
//	xr_reader *reader = fs.r_open(m_root + path);
//	if (reader)
//	{
//		const uint8_t *data = static_cast<const uint8_t*>(reader->data());
//		size_t offset = m_archive->tell();
//		size_t size = reader->size();

//		uint8_t *data_compressed = nullptr;
//		size_t size_compressed = 0;
//		xr_lzhuf::compress(data_compressed, size_compressed, data, size);
//		msg("%d->%d %s", size, size_compressed, path.c_str());

//		uint32_t crc = crc32(data_compressed, size_compressed);
//		m_archive->w_raw(data_compressed, size_compressed);
//		fs.r_close(reader);

//		std::string path_lowercase = path;
//		std::transform(path_lowercase.begin(), path_lowercase.end(), path_lowercase.begin(), [](unsigned char c) { return std::tolower(c); });

//		db_file *file = new db_file;
//		file->path = path_lowercase;
//		file->crc = crc;
//		file->offset = offset;
//		file->size_real = size;
//		file->size_compressed = size_compressed;
//		m_files.push_back(file);
//	}

	xr_file_system& fs = xr_file_system::instance();
	xr_reader *r = fs.r_open(m_root + path);
	if (r)
	{
		size_t offset = m_archive->tell();
		size_t size = r->size();
		uint32_t crc = crc32(r->data(), size);
		m_archive->w_raw(r->data(), size);
		fs.r_close(r);

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
