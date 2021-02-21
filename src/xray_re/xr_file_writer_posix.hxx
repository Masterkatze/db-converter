#pragma once

#include "xr_types.hxx"
#include "xr_writer.hxx"

#include <string>

namespace xray_re
{
	class xr_file_writer_posix: public xr_writer
	{
	public:
		xr_file_writer_posix();
		explicit xr_file_writer_posix(int fd);
		virtual ~xr_file_writer_posix() override;
		virtual void w_raw(const void *data, size_t length) override;
		virtual void seek(size_t pos) override;
		virtual size_t tell() override;

	private:
		int m_fd;
	};
}
