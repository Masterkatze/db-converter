#pragma once

#include "xray_re/xr_types.hxx"

#include <string>

namespace xray_re
{
	class xr_reader;
	class xr_writer;
	class xr_file_system;
};

class Unpacker
{
public:
	~Unpacker() = default;

	void process(const std::string& source_path, const std::string& destination_path, const xray_re::DBVersion& version, const std::string& filter);

private:
	static void extract_1114(const std::string& prefix, const std::string& mask, xray_re::xr_reader *reader, const uint8_t *data);
	static void extract_2215(const std::string& prefix, const std::string& mask, xray_re::xr_reader *reader, const uint8_t *data);
	static void extract_2945(const std::string& prefix, const std::string& mask, xray_re::xr_reader *reader, const uint8_t *data);
	static void extract_2947(const std::string& prefix, const std::string& mask, xray_re::xr_reader *reader, const uint8_t *data);

	static bool write_file(xray_re::xr_file_system& fs, const std::string& path, const void *data, size_t size);
	static bool write_file(xray_re::xr_file_system& fs, const std::string& path, const uint8_t *data, uint32_t size_real, uint32_t size_compressed);
};