#pragma once

#include "xr_types.hxx"
#include "xr_reader.hxx"

namespace xray_re
{
	class xr_mmap_reader_posix: public xr_reader
	{
	public:
		xr_mmap_reader_posix();
		xr_mmap_reader_posix(const std::string& path);
		~xr_mmap_reader_posix() override;

	private:
		int m_fd{-1};
		std::size_t m_file_length{0};
		std::size_t m_mem_length{0};
	};
} // namespace xray_re
