#include "packer.hxx"
#include "db_tools.hxx"

#include "xray_re/xr_file_system.hxx"
#include "xray_re/xr_utils.hxx"
#include "xray_re/xr_lzhuf.hxx"
#include "xray_re/xr_scrambler.hxx"
#include "crc32/crc32.hxx"

#include <spdlog/spdlog.h>

#include <filesystem>

using namespace xray_re;

extern bool m_debug;

Packer::~Packer()
{
	delete_elements(m_files);
}

void Packer::process(const std::string& source_path, const std::string& destination_path, const DBVersion& version, const std::string& xdb_ud, bool is_read_only)
{
	if(source_path.empty())
	{
		spdlog::error("Missing source directory path");
		return;
	}

	if(!xr_file_system::folder_exist(source_path))
	{
		spdlog::error("Failed to find folder {}", source_path);
		return;
	}

	if(destination_path.empty())
	{
		spdlog::error("Missing destination file path");
		return;
	}

	xr_file_system& fs = xr_file_system::instance();
	fs.set_read_only(is_read_only);

	auto path_splitted = fs.split_path(destination_path);

	if(!xr_file_system::folder_exist(path_splitted.folder))
	{
		spdlog::info("Destination folder {} doesn't exist, creating", path_splitted.folder);
		fs.create_path(path_splitted.folder);
	}

	if(version == DBVersion::DB_VERSION_AUTO)
	{
		spdlog::error("Unspecified DB format");
		return;
	}

	if(version == DBVersion::DB_VERSION_1114 || version == DBVersion::DB_VERSION_2215 || version == DBVersion::DB_VERSION_2945)
	{
		spdlog::error("Unsupported DB format");
		return;
	}

	fs.append_path_separator(m_root);

	m_archive = fs.w_open(destination_path);
	if(!m_archive)
	{
		spdlog::error("Failed to load {}", destination_path);
		return;
	}

	if(version == DBVersion::DB_VERSION_XDB && !xdb_ud.empty())
	{
		if(auto reader = fs.r_open(xdb_ud))
		{
			m_archive->open_chunk(DB_CHUNK_USERDATA);
			m_archive->w_raw(reader->data(), reader->size());
			m_archive->close_chunk();
			fs.r_close(reader);
		}
		else
		{
			spdlog::error("Failed to load {}", xdb_ud);
		}
	}

	m_archive->open_chunk(DB_CHUNK_DATA);
	m_root = source_path;
	fs.append_path_separator(m_root);
	process_folder(m_root);
	m_archive->close_chunk();

	auto w = new xr_memory_writer;

//	spdlog::info("folders: ");
//	for(const auto& folder : m_folders)
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
	for(const auto& file : m_files)
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
	uint32_t size = 0;
	xr_lzhuf::compress(data, size, w->data(), w->tell());
	delete w;

	if(version == DBVersion::DB_VERSION_2947RU)
	{
		xr_scrambler scrambler(xr_scrambler::CC_RU);
		scrambler.encrypt(data, data, size);
	}
	else if(version == DBVersion::DB_VERSION_2947WW)
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

void Packer::process_folder(const std::string& path)
{
	std::vector<std::filesystem::directory_entry> files, folders;

	for(auto& entry : std::filesystem::recursive_directory_iterator(path))
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

	auto comparator = [](auto lhs, auto rhs)
	{
		return lhs.path() < rhs.path();
	};

	std::sort(folders.begin(), folders.end(), comparator);

	auto root_path = std::filesystem::path(path);

	for(const auto& folder : folders)
	{
		auto entry_path = std::filesystem::path(folder);
		auto relative_path = std::filesystem::relative(entry_path, root_path);
		m_folders.push_back(relative_path);
	}

	std::sort(files.begin(), files.end(), comparator);

	for(const auto& file : files)
	{
		auto entry_path = std::filesystem::path(file);
		auto relative_path = std::filesystem::relative(entry_path, root_path);
		process_file(relative_path);
	}
}

void Packer::process_file(const std::string& path)
{
	constexpr bool compress_files = false;

	if constexpr(compress_files)
	{
		xr_file_system& fs = xr_file_system::instance();
		auto reader = fs.r_open(m_root + path);
		if(reader)
		{
			auto data = static_cast<const uint8_t*>(reader->data());
			auto offset = m_archive->tell();
			auto size = reader->size();

			uint8_t *data_compressed = nullptr;
			uint32_t size_compressed = 0u;
			xr_lzhuf::compress(data_compressed, size_compressed, data, size);
			spdlog::info("{}->{} {}", size, size_compressed, path);

			auto crc = crc32(data_compressed, size_compressed);
			m_archive->w_raw(data_compressed, size_compressed);
			fs.r_close(reader);

			auto path_lowercase = path;
			std::transform(path_lowercase.begin(), path_lowercase.end(), path_lowercase.begin(), [](unsigned char c) { return std::tolower(c); });

			auto file = new db_file;
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
		if(reader)
		{
			auto offset = m_archive->tell();
			auto size = reader->size();
			auto crc = crc32(reader->data(), size);
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
