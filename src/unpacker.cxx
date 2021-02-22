#include "unpacker.hxx"
#include "xray_re/xr_file_system.hxx"
#include "xray_re/xr_scrambler.hxx"
#include "xray_re/xr_lzhuf.hxx"
#include "lzo/minilzo.h"

#include <spdlog/spdlog.h>

using namespace xray_re;

extern bool m_debug;

void Unpacker::process(const std::string& source_path, const std::string& destination_path, const DBVersion& version, const std::string& filter)
{
	if(version == DBVersion::DB_VERSION_AUTO)
	{
		spdlog::error("Unspecified DB format");
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
	auto output_folder = destination_path.empty() ? path_splitted.folder : destination_path;
	auto extension = path_splitted.extension;

	xr_file_system& fs = xr_file_system::instance();
	auto reader_full = fs.r_open(source_path);
	if(!reader_full)
	{
		spdlog::error("Can't load {}", source_path);
		return;
	}

	if(fs.create_path(output_folder))
	{
		xr_file_system::append_path_separator(output_folder);

		auto reader_chunk = reader_full->open_chunk(DB_CHUNK_USERDATA);

		if(reader_chunk)
		{
			std::string path = destination_path + "_userdata.ltx";
			write_file(fs, path, reader_chunk->data(), reader_chunk->size());
			reader_full->close_chunk(reader_chunk);
		}

		switch(version)
		{
			case DBVersion::DB_VERSION_1114:
			case DBVersion::DB_VERSION_2215:
			case DBVersion::DB_VERSION_2945:
			case DBVersion::DB_VERSION_XDB:
			{
				reader_chunk = reader_full->open_chunk(DB_CHUNK_HEADER);
				break;
			}
			case DBVersion::DB_VERSION_2947RU:
			{
				xr_scrambler scrambler(xr_scrambler::CC_RU);
				reader_chunk = reader_full->open_chunk(DB_CHUNK_HEADER, scrambler);
				break;
			}
			case DBVersion::DB_VERSION_2947WW:
			{
				xr_scrambler scrambler(xr_scrambler::CC_WW);
				reader_chunk = reader_full->open_chunk(DB_CHUNK_HEADER, scrambler);
				break;
			}
			default:
			{
				spdlog::error("Unknown DB format");
				return;
			}
		}

		if(reader_chunk)
		{
			auto data_full = static_cast<const uint8_t*>(reader_full->data());
			switch (version)
			{
				case DBVersion::DB_VERSION_1114:
				{
					extract_1114(output_folder, filter, reader_chunk, data_full);
					break;
				}
				case DBVersion::DB_VERSION_2215:
				{
					extract_2215(output_folder, filter, reader_chunk, data_full);
					break;
				}
				case DBVersion::DB_VERSION_2945:
				{
					extract_2945(output_folder, filter, reader_chunk, data_full);
					break;
				}
				case DBVersion::DB_VERSION_2947RU:
				case DBVersion::DB_VERSION_2947WW:
				case DBVersion::DB_VERSION_XDB:
				{
					extract_2947(output_folder, filter, reader_chunk, data_full);
					break;
				}
				default:
				{
					spdlog::error("Unknown DB format");
					return;
				}
			}
			reader_full->close_chunk(reader_chunk);
		}
	}
	else
	{
		spdlog::error("Failed to create {}", output_folder);
	}
	fs.r_close(reader_full);
}

void Unpacker::extract_1114(const std::string& prefix, const std::string& mask, xr_reader *reader, const uint8_t *data)
{
	xr_file_system& fs = xr_file_system::instance();
	while(!reader->eof())
	{
		std::string name;
		reader->r_sz(name);

		auto uncompressed = reader->r_u32();
		auto offset = reader->r_u32();
		auto size = reader->r_u32();

		std::replace(name.begin(), name.end(), '\\', '/');
		auto path = prefix + name;

		if(mask.length() > 0 && offset != 0 && path.find(mask) == std::string::npos)
		{
			continue;
		}

		spdlog::debug("{}", path);
		spdlog::debug("  offset: {}", offset);

		if(uncompressed)
		{
			spdlog::debug("  size (real): {}", size);
		}
		else
		{
			spdlog::debug("  size (compressed): {}", size);
		}

		if(fs.is_read_only())
		{
			continue;
		}

		auto path_splitted = xr_file_system::split_path(path.append(name));
		auto folder = path_splitted.folder;
		fs.create_path(folder);

		if(uncompressed)
		{
			write_file(fs, path, data + offset, size);
		}
		else
		{
			uint32_t real_size;
			uint8_t *p;
			xr_lzhuf::decompress(p, real_size, data + offset, size);

			if(real_size)
			{
				write_file(fs, path, p, real_size);
			}

			free(p);
		}
	}
}

void Unpacker::extract_2215(const std::string& prefix, const std::string& mask, xr_reader *reader, const uint8_t *data)
{
	xr_file_system& fs = xr_file_system::instance();
	while(!reader->eof())
	{
		std::string path;
		reader->r_sz(path);
		std::replace(path.begin(), path.end(), '\\', '/');

		auto offset = reader->r_u32();
		auto size_real = reader->r_u32();
		auto size_compressed = reader->r_u32();

		if(mask.length() > 0 && offset != 0 && path.find(mask) == std::string::npos)
		{
			continue;
		}

		spdlog::debug("{}", path);
		spdlog::debug("  offset: {}", offset);
		spdlog::debug("  size (real): {}", size_real);
		spdlog::debug("  size (compressed): {}", size_compressed);

		if(fs.is_read_only())
		{
			continue;
		}

		if(offset == 0)
		{
			fs.create_folder(prefix + path);
		}
		else
		{
			write_file(fs, prefix + path, data + offset, size_real, size_compressed);
		}
	}
}

void Unpacker::extract_2945(const std::string& prefix, const std::string& mask, xr_reader *reader, const uint8_t *data)
{
	xr_file_system& fs = xr_file_system::instance();
	while(!reader->eof())
	{
		std::string path;
		reader->r_sz(path);
		std::replace(path.begin(), path.end(), '\\', '/');

		auto crc = reader->r_u32();
		auto offset = reader->r_u32();
		auto size_real = reader->r_u32();
		auto size_compressed = reader->r_u32();

		if(mask.length() > 0 && offset != 0 && path.find(mask) == std::string::npos)
		{
			continue;
		}

		spdlog::debug("{}", path);
		spdlog::debug("  crc: {0:#x}", crc);
		spdlog::debug("  offset: {}", offset);
		spdlog::debug("  size (real): {}", size_real);
		spdlog::debug("  size (compressed): {}", size_compressed);

		if(fs.is_read_only())
		{
			continue;
		}

		if(offset == 0)
		{
			fs.create_folder(prefix + path);
		}
		else
		{
			write_file(fs, prefix + path, data + offset, size_real, size_compressed);
		}
	}
}

void Unpacker::extract_2947(const std::string& prefix, const std::string& mask, xr_reader *reader, const uint8_t *data)
{
	xr_file_system& fs = xr_file_system::instance();
	while(!reader->eof())
	{
		auto name_size = reader->r_u16() - 16;                      // unsigned 2 bytes <─┐
		auto size_real = reader->r_u32();                           // unsigned 4 bytes   │
		auto size_compressed = reader->r_u32();                     // unsigned 4 bytes   │
		auto crc = reader->r_u32();                                 // unsigned 4 bytes   │
		std::string name(reader->skip<char>(name_size), name_size); // string   N bytes >─┘
		auto offset = reader->r_u32();                              // unsigned 4 bytes

		std::replace(name.begin(), name.end(), '\\', '/');
		auto path = prefix + name;

		if(mask.length() > 0 && offset != 0 && path.find(mask) == std::string::npos)
		{
			continue;
		}

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

		if(fs.is_read_only())
		{
			continue;
		}

		if(offset == 0)
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

bool Unpacker::write_file(xr_file_system& fs, const std::string& path, const void *data, size_t size)
{
	auto w = fs.w_open(path);
	if(w)
	{
		w->w_raw(data, size);
		fs.w_close(w);

		return true;
	}

	return false;
}

bool Unpacker::write_file(xr_file_system& fs, const std::string& path, const uint8_t *data, uint32_t size_real, uint32_t size_compressed)
{
	if(size_real != size_compressed)
	{
		lzo_uint size = size_real;
		auto temp = new uint8_t[size];
		if(lzo1x_decompress_safe(data, size_compressed, temp, &size, nullptr) != LZO_E_OK)
		{
			delete[] temp;
			return false;
		}
		data = temp;
		size_real = uint32_t(size & UINT32_MAX);
	}

	if(!write_file(fs, path, data, size_real))
	{
		auto folder = xr_file_system::split_path(path).folder;

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

		if((!fs.is_read_only() && !write_file(fs, path, data, size_real)) || (fs.is_read_only() && !fs.file_exist(path)))
		{
			spdlog::error("Failed to open file \"{}\": {} (errno={}) ", path, strerror(errno), errno);
			return false;
		}
	}

	if(size_real != size_compressed)
	{
		delete[] data;
	}

	return true;
}