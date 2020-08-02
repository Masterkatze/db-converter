// separated to avoid unnecessary inclusion of scrambler code.

#include "xr_reader.hxx"
#include "xr_lzhuf.hxx"
#include "xr_scrambler.hxx"
#include "xr_utils.hxx"

#include <spdlog/spdlog.h>

using namespace xray_re;

xr_reader* xr_reader::open_chunk(uint32_t id, const xr_scrambler& scrambler)
{
	spdlog::debug("xr_reader_scrambler::open_chunk chunk_id={} compressed={}", id & ~CHUNK_COMPRESSED, (id & CHUNK_COMPRESSED) != 0);

	bool compressed;
	size_t size = find_chunk(id, compressed);

	if(size == 0)
	{
		return nullptr;
	}

	if(compressed)
	{
		auto temp = new uint8_t[size];
		scrambler.decrypt(temp, m_p, size);
		uint8_t* data;
		uint32_t real_size;
		xr_lzhuf::decompress(data, real_size, temp, size);
		delete[] temp;
		return new xr_temp_reader(data, real_size);
	}
	else
	{
		return new xr_reader(m_p, size);
	}
}
