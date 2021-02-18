#include "xr_reader.hxx"
#include "xr_lzhuf.hxx"
#include "xr_packet.hxx"
#include "xr_utils.hxx"

#include <spdlog/spdlog.h>

#include <fstream>
#include <iostream>
#include <cstdlib>
#include <cstring>

using namespace xray_re;

const uint32_t CHUNK_ID_MASK = ~CHUNK_COMPRESSED;

xr_reader::xr_reader() : m_data(nullptr), m_p(nullptr), m_end(nullptr), m_next(nullptr) { }

xr_reader::xr_reader(const void *data, size_t length)
{
	m_next = m_p = m_data = static_cast<const uint8_t*>(data);
	m_end = static_cast<const uint8_t*>(data) + length;
}

size_t xr_reader::find_chunk(uint32_t id, bool& compressed, bool reset)
{
	if(reset)
	{
		m_p = m_data;
	}

	while(m_p < m_end)
	{
		assert(m_p + 8 <= m_end);
		auto chunk_id = r_u32();
		auto chunk_size = r_u32();
		assert(m_p + chunk_size <= m_end);

		spdlog::debug("xr_reader::find_chunk chunk_id={} compressed={}", chunk_id & ~CHUNK_COMPRESSED, (chunk_id & CHUNK_COMPRESSED) != 0);

		if(id == (chunk_id & CHUNK_ID_MASK))
		{
			//xr_assert((chunk_id & CHUNK_COMPRESSED) == 0);
			compressed = (chunk_id & CHUNK_COMPRESSED) != 0;
			return chunk_size;
		}
		m_p += chunk_size;
	}
	return 0;
}

size_t xr_reader::find_chunk(uint32_t id)
{
	bool compressed;
	return find_chunk(id, compressed);
}

xr_reader* xr_reader::open_chunk(uint32_t id)
{
	bool compressed;
	auto size = find_chunk(id, compressed);

	if(size == 0)
	{
		return nullptr;
	}

	spdlog::debug("xr_reader::open_chunk chunk_id={} compressed={}", id & ~CHUNK_COMPRESSED, (id & CHUNK_COMPRESSED) != 0);

	if(compressed)
	{
		uint32_t real_size;
		uint8_t* data;
		xr_lzhuf::decompress(data, real_size, m_p, size);

//		if(id == 1)
//		{
//			std::ofstream test_file_header;
//			test_file_header.open("/tmp/stalker/test_data/cop/bin/header.bin", std::ios::binary);
//			test_file_header.write((const char*)data, real_size);
//			test_file_header.close();
//		}

		return new xr_temp_reader(data, real_size);
	}
	else
	{
		return new xr_reader(m_p, size);
	}
}

void xr_reader::close_chunk(xr_reader *&r) const
{
	assert(!r || (r != this && r->m_p <= r->m_end));
	delete r;
	r = nullptr;
}

xr_reader* xr_reader::open_chunk_next(uint32_t& id, xr_reader *prev)
{
	if(prev)
	{
		delete prev;
	}
	else
	{
		m_next = m_data;
	}

	if(m_next < m_end)
	{
		assert(m_next + 8 <= m_end);
		m_p = m_next;
		auto chunk_id = r_u32();
		auto chunk_size = r_u32();
		assert(m_p + chunk_size <= m_end);
		m_next = m_p + chunk_size;
		id = chunk_id;

		spdlog::debug("xr_reader::open_chunk_next chunk_id={} compressed={}", chunk_id & ~CHUNK_COMPRESSED, (chunk_id & CHUNK_COMPRESSED) != 0);

		if(chunk_id & CHUNK_COMPRESSED)
		{
			uint32_t real_size;
			uint8_t* data;
			xr_lzhuf::decompress(data, real_size, m_p, chunk_size);
			return new xr_temp_reader(data, real_size);
		}
		else
		{
			return new xr_reader(m_p, chunk_size);
		}
	}
	return nullptr;
}

size_t xr_reader::r_raw_chunk(uint32_t id, void *dest, size_t dest_size)
{
	bool compressed;
	auto size = find_chunk(id, compressed);
	if(size == 0)
	{
		return 0;
	}

	assert(!compressed);
	assert(size <= dest_size);
	r_raw(dest, size);
	return size;
}

void xr_reader::r_raw(void *dest, size_t dest_size)
{
	assert(m_p + dest_size <= m_end);
	std::memmove(dest, m_p, dest_size);
	m_p += dest_size;
}

const char* xr_reader::skip_sz()
{
	auto p = m_p;
	while(m_p < m_end)
	{
		if(*m_p++ == 0)
		{
			return reinterpret_cast<const char*>(p);
		}
	}

	// always crash if no '\0' in the chunk
	assert(m_p < m_end);
	return reinterpret_cast<const char*>(p);
}

void xr_reader::r_s(std::string& value)
{
	auto p = m_p;
	assert(p < m_end);
	while(p != m_end && *p != '\n' && *p != '\r')
	{
		++p;
	}

	value.assign(m_p, p);
	while(p != m_end && (*p == '\n' || *p == '\r'))
	{
		++p;
	}

	m_p = p;
}

void xr_reader::r_sz(std::string& value)
{
	auto p = m_p;
	assert(p < m_end);
	while(*p++)
	{
		// FIXME: we should crash in debug mode if no '\0' in the chunk,
		// but that is not good for older models (from 2215).
		// update: OGF_S_LODS 
		// assert(p < m_end);
		if(p >= m_end)
		{
			value.assign(m_p, p);
			m_p = p;
			return;
		}
	}
	value.assign(m_p, p - 1);
	m_p = p;
}

void xr_reader::r_sz(char *dest, size_t dest_size)
{
	auto p = m_p;
	assert(p < m_end && dest_size > 0);
	auto end = p + dest_size;

	if(end > m_end)
	{
		end = m_end;
	}

	while(*p++)
	{
		// crash in debug mode if we don't fit in buffer or chunk
		assert(p < end);
		if(p >= end)
		{
			std::memmove(dest, m_p, static_cast<size_t>(p - m_p));
			dest[dest_size - 1] = 0;
			m_p = p;
			return;
		}
	}
	std::memmove(dest, m_p, static_cast<size_t>(p - m_p));
	m_p = p;
}

void xr_reader::r_packet(xr_packet& packet, size_t size)
{
	packet.init(skip<uint8_t>(size), size);
}

xr_temp_reader::~xr_temp_reader()
{
	if(m_data)
	{
		free(const_cast<uint8_t*>(m_data));
		m_data = nullptr;
	}
}
