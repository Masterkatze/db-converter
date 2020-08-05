#include "db_tools.hxx"

#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
#include <boost/crc.hpp>

#include <optional>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

std::streamsize const buffer_size = 1024;

std::optional<unsigned int> GetChecksum(const std::string& file_path)
{
	boost::crc_32_type result;

	std::ifstream file_stream(file_path, std::ios_base::binary);

	if(file_stream)
	{
		do
		{
			char buffer[buffer_size];
			file_stream.read(buffer, buffer_size);
			result.process_bytes(buffer, file_stream.gcount());
		} while(file_stream);
	}
	else
	{
		spdlog::error("Failed to open file \"{}\"", file_path);
		return std::nullopt;
	}

	spdlog::info("\"{}\": {}", file_path, result.checksum());
	return result.checksum();
}

std::size_t GetNumberOfFilesAndDirectories(const std::string &path)
{
	using fs::recursive_directory_iterator;
	return std::distance(recursive_directory_iterator(path), recursive_directory_iterator{});
}

void CompareChecksums(const std::string& game, unsigned int crc32, unsigned int number_of_files) {
	std::string file_name = "mp_pool.db";
	std::string folder_path = "/home/orange/dev/db_converter_data/";
	std::string original_file_path = folder_path + game + "/original/" + file_name;
	std::string unpack_path = folder_path + game + "/unpacked";
	std::string packed_file_path = folder_path + game + "/packed/" + file_name;
	std::string userdata_file_path = unpack_path + "_userdata.ltx";
	std::string filter = "";

	ASSERT_TRUE(fs::exists(original_file_path));
	fs::remove_all(unpack_path);
	ASSERT_FALSE(fs::exists(unpack_path));

	{
		auto checksum = GetChecksum(original_file_path);
		ASSERT_TRUE(checksum.has_value());
		EXPECT_EQ(checksum.value(), crc32);
	}
	{
		db_unpacker unpacker;
		unpacker.set_debug(true);
		unpacker.process(original_file_path, unpack_path, db_tools::db_version::DB_VERSION_XDB, filter);

		EXPECT_EQ(GetNumberOfFilesAndDirectories(unpack_path), number_of_files);
	}
	{
		db_packer packer;
		packer.set_debug(true);
		packer.process(unpack_path, packed_file_path, db_tools::db_version::DB_VERSION_XDB, userdata_file_path);

		auto checksum = GetChecksum(packed_file_path);
		ASSERT_TRUE(checksum.has_value());
		EXPECT_EQ(checksum.value(), crc32);
	}

	ASSERT_EQ(fs::file_size(original_file_path), fs::file_size(packed_file_path));
}

TEST(Compare, ClearSkyChecksums)
{
	CompareChecksums("cs", 1778816644, 15);
}

TEST(Compare, CallOfPripyatChecksums)
{
	CompareChecksums("cop", 2187792425, 16);
}
