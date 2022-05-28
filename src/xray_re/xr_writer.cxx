#include "xr_writer.hxx"
#include "xr_file_system.hxx"
#include "xr_packet.hxx"
#include "xr_utils.hxx"

#include <spdlog/spdlog.h>

#include <string>
#include <cstring>
#include <cstdarg>

using namespace xray_re;

void xr_writer::open_chunk(uint32_t id)
{
	spdlog::debug("xr_writer::open_chunk chunk_id={} compressed={}", id & ~CHUNK_COMPRESSED, (id & CHUNK_COMPRESSED) != 0);

	w_u32(id);
	w_u32(0);
	m_open_chunks.push(tell());
}

void xr_writer::close_chunk()
{
	assert(!m_open_chunks.empty());

	auto pos = tell();
	auto chunk_pos = m_open_chunks.top();
	assert(chunk_pos <= pos);

	seek(chunk_pos - 4);
	w_size_u32(pos - chunk_pos);
	seek(pos);
	m_open_chunks.pop();
}

void xr_writer::w_raw_chunk(uint32_t id, const void *data, std::size_t size)
{
	spdlog::debug("xr_writer::w_raw_chunk chunk_id={} compressed={}", id & ~CHUNK_COMPRESSED, (id & CHUNK_COMPRESSED) != 0);

	w_u32(id);
	w_size_u32(size);
	w_raw(data, size);
}

void xr_writer::w_sz(const std::string& value)
{
	// do not write extra '\0'
	// std::size_t length = value.length() + 1;
	// const char *c_str = value.c_str();
	// if(len > 0 && c_str[len] == '\0')
	w_raw(value.data(), value.length() + 1);
}

void xr_writer::w_sz(const char *value)
{
	assert(value);
	w_raw(value, std::strlen(value) + 1);
}

void xr_writer::w_s(const char *value)
{
	w_raw(value, std::strlen(value));
	w_raw("\r\n", 2);
}

void xr_writer::w_s(const std::string& value)
{
	w_raw(value.data(), value.length());
	w_raw("\r\n", 2);
}

void xr_writer::w_packet(const xr_packet& packet)
{
	w_raw(packet.buf(), packet.w_tell());
}

void xr_memory_writer::w_raw(const void *data, std::size_t size)
{
	if(!size)
	{
		return;
	}

	if(m_pos + size > m_buffer.size())
	{
		m_buffer.resize(m_pos + size);
	}

	std::memmove(&m_buffer[m_pos], data, size);
	m_pos += size;
}

void xr_memory_writer::seek(std::size_t pos)
{
	assert(pos <= m_buffer.size());
	m_pos = pos;
}

std::size_t xr_memory_writer::tell()
{
	return m_pos;
}

bool xr_memory_writer::save_to(const std::string& path, const std::string& name)
{
	xr_file_system& fs = xr_file_system::instance();
	auto w = fs.w_open(path, name);
	if(!w)
	{
		return false;
	}

	w->w_raw(&m_buffer[0], m_buffer.size());
	fs.w_close(w);
	return true;
}

bool xr_memory_writer::save_to(const std::string& path)
{
	xr_file_system& fs = xr_file_system::instance();
	auto w = fs.w_open(path);
	if(!w)
	{
		return false;
	}

	w->w_raw(&m_buffer[0], m_buffer.size());
	fs.w_close(w);
	return true;
}

void xr_fake_writer::w_raw(const void *, std::size_t size)
{
	m_pos += size;
	if(m_size < m_pos)
	{
		m_size = m_pos;
	}
}

void xr_fake_writer::seek(std::size_t pos)
{
	assert(pos < m_size);
	m_pos = m_size;
}

std::size_t xr_fake_writer::tell()
{
	return m_pos;
}
