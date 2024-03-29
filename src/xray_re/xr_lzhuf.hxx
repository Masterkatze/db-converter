#pragma once

#include "xr_types.hxx"

#include <array>

namespace xray_re
{
	class _lzhuf
	{
	private:
		enum basic_params
		{
			N = 4096,
			F = 60,
			THRESHOLD = 2,
			MAX_FREQ = 0x4000,
		};

		enum derived_params
		{
			NIL = N,
			N_CHAR = 256 - THRESHOLD + F,
			T = N_CHAR * 2 - 1,
			R = T - 1,
		};

		unsigned char text_buf[N + F - 1];
		int match_position, lson[N + 1], rson[N + 257], dad[N + 1];
		uint32_t match_length;
		unsigned int freq[T + 1];
		int prnt[T + N_CHAR];
		int son[T];

		uint32_t textsize;
		uint8_t *m_dest;
		uint32_t m_dest_pos;
		uint32_t m_dest_limit;

		uint32_t codesize;
		const uint8_t *m_src;
		uint32_t m_src_pos;
		uint32_t m_src_limit;

		unsigned int getbuf;
		unsigned char getlen;

		unsigned int putbuf;
		unsigned char putlen;

		static const std::array<uint8_t, 64> p_len;
		static const std::array<uint8_t, 64> p_code;
		static const std::array<uint8_t, 256> d_code;
		static const std::array<uint8_t, 256> d_len;

		void InitTree();
		void InsertNode(int r);
		void DeleteNode(int p);

		int GetBit();
		int GetByte(void);
		void Putcode(int l, unsigned int c);

		void StartHuff();
		void reconst();
		void update(int c);
		void EncodeChar(unsigned int c);
		void EncodePosition(unsigned int c);
		void EncodeEnd();
		int DecodeChar();
		int DecodePosition();

		int getc();
		void putc(int c);

	public:
		void Encode(uint8_t *&_code, uint32_t& _codesize, const uint8_t *_text, uint32_t _textsize);
		void Decode(uint8_t *&_text, uint32_t& _textsize, const uint8_t *_code, uint32_t _codesize);
	};

	class xr_lzhuf
	{
	private:
		_lzhuf m_lzhuf;
		static xr_lzhuf* instance();

	protected:
		xr_lzhuf() = default;
		xr_lzhuf(const xr_lzhuf& that);
		xr_lzhuf& operator=(const xr_lzhuf& right);

	public:
		static void compress(uint8_t *&_code, uint32_t& _codesize, const uint8_t *_text, uint32_t _textsize);
		static void decompress(uint8_t *&_text, uint32_t& _textsize, const uint8_t *_code, uint32_t _codesize);
	};

	inline void xr_lzhuf::compress(uint8_t *&_code, uint32_t& _codesize, const uint8_t *_text, uint32_t _textsize)
	{
		instance()->m_lzhuf.Encode(_code, _codesize, _text, _textsize);
	}

	inline void xr_lzhuf::decompress(uint8_t *&_text, uint32_t& _textsize, const uint8_t *_code, uint32_t _codesize)
	{
		instance()->m_lzhuf.Decode(_text, _textsize, _code, _codesize);
	}
} // namespace xray_re
