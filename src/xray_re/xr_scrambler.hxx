#pragma once

#include "xr_types.hxx"

#include <array>

namespace xray_re
{
	class xr_scrambler
	{
	public:
		enum cipher_config
		{
			CC_RU,
			CC_WW,
		};

		xr_scrambler() = delete;
		explicit xr_scrambler(cipher_config cc);

		void init(cipher_config cc);
		void encrypt(uint8_t *dest, const uint8_t *src, std::size_t size) const;
		void decrypt(uint8_t *dest, const uint8_t *src, std::size_t size) const;

	private:
		enum
		{
			SBOX_SIZE = 256
		};

		void init_sboxes(int seed, std::size_t size_mult);

		int m_seed;
		std::array<uint8_t, SBOX_SIZE> m_enc_sbox;
		std::array<uint8_t, SBOX_SIZE> m_dec_sbox;
	};
} // namespace xray_re
