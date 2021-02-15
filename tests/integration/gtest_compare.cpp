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

void CompareChecksums(const std::string& file_path, db_tools::db_version version, unsigned int crc32, unsigned int number_of_files)
{
	std::string temp_path = "/tmp/db_converter/gtest_compare/";
	std::string unpack_path = temp_path + "unpacked";
	std::string packed_file_path = temp_path + "packed/packed.db";
	std::string userdata_file_path = unpack_path + "_userdata.ltx";
	std::string filter = "";

	ASSERT_TRUE(fs::exists(file_path));

	fs::remove_all(unpack_path);
	ASSERT_FALSE(fs::exists(unpack_path));

	fs::remove_all(packed_file_path);
	ASSERT_FALSE(fs::exists(packed_file_path));

	{
		auto checksum = GetChecksum(file_path);
		ASSERT_TRUE(checksum.has_value());
		EXPECT_EQ(checksum.value(), crc32);
	}
	{
		db_unpacker unpacker;
		unpacker.set_debug(true);
		unpacker.process(file_path, unpack_path, version, filter);

		EXPECT_EQ(GetNumberOfFilesAndDirectories(unpack_path), number_of_files);
	}
	{
		db_packer packer;
		packer.set_debug(true);
		packer.process(unpack_path, packed_file_path, version, userdata_file_path);

		auto checksum = GetChecksum(packed_file_path);
		ASSERT_TRUE(checksum.has_value());
		EXPECT_EQ(checksum.value(), crc32);
	}

	ASSERT_EQ(fs::file_size(file_path), fs::file_size(packed_file_path));
}

TEST(Compare, ClearSkyChecksums)
{
	auto path = std::string(getenv("HOME")) + "/.local/share/GSC Game World/S.T.A.L.K.E.R. - Clear Sky/mp/mp_pool.db";
	CompareChecksums(path, db_tools::db_version::DB_VERSION_XDB, 1778816644, 15);
}

TEST(Compare, CallOfPripyatChecksums)
{
	auto path = std::string(getenv("HOME")) + "/.local/share/GSC Game World/S.T.A.L.K.E.R. - Call of Pripyat/mp/mp_pool.db";
	CompareChecksums(path, db_tools::db_version::DB_VERSION_XDB, 2187792425, 16);
}
