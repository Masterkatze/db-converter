#pragma once

#include "xr_types.hxx"
#include "xr_writer.hxx"

#include <string>

namespace xray_re
{
	class xr_file_writer_posix: public xr_writer
	{
	public:
		explicit xr_file_writer_posix(const std::string& path);
		~xr_file_writer_posix() override;
		void w_raw(const void *data, std::size_t length) override;
		void seek(std::size_t pos) override;
		std::size_t tell() override;

	private:
		int m_fd{-1};
	};
} // namespace xray_re
