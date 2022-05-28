#include "xr_scrambler.hxx"

#include <numeric>
#include <utility>

using namespace xray_re;

const int SEED_MULT = 0x8088405;

const int SEED_RU = 0x131a9d3;
const int SEED0_RU = 0x1329436;
const int SIZE_MULT_RU = 8;

const int SEED_WW = 0x16eb2eb;
const int SEED0_WW = 0x5bbc4b;
const int SIZE_MULT_WW = 4;

xr_scrambler::xr_scrambler(cipher_config cc)
{
	init(cc);
}

void xr_scrambler::init(cipher_config cc)
{
	if(cc == CC_RU)
	{
		m_seed = SEED_RU;
		init_sboxes(SEED0_RU, SIZE_MULT_RU);
	}
	else if(cc == CC_WW)
	{
		m_seed = SEED_WW;
		init_sboxes(SEED0_WW, SIZE_MULT_WW);
	}
}

void xr_scrambler::init_sboxes(int seed, std::size_t size_mult)
{
	size_mult *= SBOX_SIZE;
	std::iota(m_enc_sbox.begin(), m_enc_sbox.end(), 0);

	for(std::size_t i = size_mult; i > 0; --i)
	{
		seed = 1 + seed * SEED_MULT;
		std::size_t a = (seed >> 24) & 0xff;
		std::size_t b;

		do
		{
			seed = 1 + seed * SEED_MULT;
			b = (seed >> 24) & 0xff;
		}
		while(a == b);

		std::swap(m_enc_sbox.at(a), m_enc_sbox.at(b));
	}

	for(std::size_t i = 0; i < SBOX_SIZE; ++i)
	{
		m_dec_sbox.at(m_enc_sbox.at(i)) = uint8_t(i);
	}
}

void xr_scrambler::decrypt(uint8_t *dest, const uint8_t *src, std::size_t size) const
{
	auto seed = m_seed;
	for(std::size_t i = 0; i != size; ++i)
	{
		seed = 1 + seed * SEED_MULT;
		dest[i] = m_dec_sbox.at(src[i] ^ ((seed >> 24) & 0xff));
	}
}

void xr_scrambler::encrypt(uint8_t *dest, const uint8_t *src, std::size_t size) const
{
	auto seed = m_seed;
	for(std::size_t i = 0; i != size; ++i)
	{
		seed = 1 + seed * SEED_MULT;
		dest[i] = uint8_t(m_enc_sbox.at(src[i]) ^ ((seed >> 24) & 0xff));
	}
}
