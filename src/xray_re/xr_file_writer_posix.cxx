#include "xr_file_writer_posix.hxx"
#include "xr_utils.hxx"

#include <spdlog/spdlog.h>

#include <filesystem>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>

using namespace xray_re;

xr_file_writer_posix::xr_file_writer_posix() : m_fd(-1) {}

xr_file_writer_posix::xr_file_writer_posix(int fd) : m_fd(fd) {}

xr_file_writer_posix::~xr_file_writer_posix()
{
	assert(m_fd != -1);

	auto res = ::close(m_fd);
	if(res == -1)
	{
		spdlog::error("Failed to close file descriptor {}: {} (errno={}) ", m_fd, strerror(errno), errno);
	}
}

void xr_file_writer_posix::w_raw(const void *data, size_t length)
{
	auto res = ::write(m_fd, data, length);

	if(length != 0 && data != MAP_FAILED)
	{
		if(res == -1)
		{
			spdlog::error("Failed to write to descriptor {}: {} (errno={}) ", m_fd, strerror(errno), errno);
		}

		assert(static_cast<size_t>(res) == length);
	}
}

void xr_file_writer_posix::seek(size_t pos)
{
	auto res = ::lseek64(m_fd, static_cast<off_t>(pos), SEEK_SET);

	assert(static_cast<size_t>(res) == pos);
}

size_t xr_file_writer_posix::tell()
{
	auto res = ::lseek64(m_fd, 0, SEEK_CUR);
	if(res == -1)
	{
		spdlog::error("Failed to close file descriptor {}: {} (errno={}) ", m_fd, strerror(errno), errno);
	}

	return static_cast<size_t>(res);
}
