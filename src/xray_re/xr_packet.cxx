#include <cstring>
#include "xr_packet.hxx"

using namespace xray_re;

xr_packet::xr_packet(): m_buf{}, m_w_pos(0), m_r_pos(0) { }

void xr_packet::r_raw(void *data, size_t size)
{
	xr_assert(m_r_pos + size <= sizeof(m_buf));
	std::memmove(data, m_buf + m_r_pos, size);
	m_r_pos += size;
}

void xr_packet::w_raw(const void *data, size_t size)
{
	assert(m_w_pos + size <= sizeof(m_buf));
	std::memmove(m_buf + m_w_pos, data, size);
	m_w_pos += size;
}

void xr_packet::r_begin(uint16_t& id)
{
	m_r_pos = 0;
	r_u16(id);
}

void xr_packet::w_begin(uint16_t id)
{
	m_w_pos = 0;
	w_u16(id);
}

const char* xr_packet::skip_sz()
{
	auto p = reinterpret_cast<const char*>(m_buf + m_r_pos);
	while (m_r_pos < sizeof(m_buf))
	{
		if (m_buf[m_r_pos++] == 0)
			return p;
	}

	// crash in debug mode if no 0 in the packet
	xr_assert(m_r_pos < sizeof(m_buf));
	return p;
}

void xr_packet::r_sz(std::string& value)
{
	auto p = &m_buf[m_r_pos];
	while (m_r_pos < sizeof(m_buf))
	{
		if (m_buf[m_r_pos++] == 0)
		{
			value.assign(p, &m_buf[m_r_pos - 1]);
			return;
		}
	}
	// crash in debug mode if no 0 in the packet
	assert(m_r_pos < sizeof(m_buf));
	value.assign(p, m_buf + m_r_pos);
}

float xr_packet::r_angle8()
{
	r_u8();
	return 0;
}

void xr_packet::w_sz(const std::string& value)
{
	w_raw(value.data(), value.length() + 1);
}

void xr_packet::init(const uint8_t *data, size_t size)
{
	xr_assert(size < sizeof(m_buf));
	m_r_pos = 0;
	m_w_pos = size;
	std::memmove(m_buf, data, size);
}
