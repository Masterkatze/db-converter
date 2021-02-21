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
		virtual ~xr_mmap_reader_posix();

	private:
		int m_fd;
		size_t m_file_length;
		size_t m_mem_length;
	};
}
