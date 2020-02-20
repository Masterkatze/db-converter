#pragma once

#include <cstddef>
#include "xr_types.hxx"

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
		int match_position, match_length, lson[N + 1], rson[N + 257], dad[N + 1];
		unsigned int freq[T + 1];
		int prnt[T + N_CHAR];
		int son[T];

		size_t textsize;
		uint8_t *m_dest;
		size_t m_dest_pos;
		size_t m_dest_limit;

		size_t codesize;
		const uint8_t *m_src;
		size_t m_src_pos;
		size_t m_src_limit;

		unsigned int getbuf;
		unsigned char getlen;

		unsigned int putbuf;
		unsigned char putlen;

		static const uint8_t p_len[64];
		static const uint8_t p_code[64];
		static const uint8_t d_code[256];
		static const uint8_t d_len[256];

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
		_lzhuf();
		~_lzhuf();

		void Encode(uint8_t *&_code, size_t& _codesize, const uint8_t *_text, size_t _textsize);
		void Decode(uint8_t *&_text, size_t& _textsize, const uint8_t *_code, size_t _codesize);
	};

	class xr_lzhuf
	{
	private:
		_lzhuf m_lzhuf;
		static xr_lzhuf* instance();

	protected:
		xr_lzhuf();
		xr_lzhuf(const xr_lzhuf& that);
		xr_lzhuf& operator=(const xr_lzhuf& right);

	public:
		static void	compress(uint8_t *&_code, size_t& _codesize, const uint8_t *_text, size_t _textsize);
		static void	decompress(uint8_t *&_text, size_t& _textsize, const uint8_t *_code, size_t _codesize);
	};

	inline _lzhuf::_lzhuf() {}
	inline _lzhuf::~_lzhuf() {}
	inline xr_lzhuf::xr_lzhuf() {}

	inline void xr_lzhuf::compress(uint8_t *&_code, size_t& _codesize, const uint8_t *_text, size_t _textsize)
	{
		instance()->m_lzhuf.Encode(_code, _codesize, _text, _textsize);
	}

	inline void xr_lzhuf::decompress(uint8_t *&_text, size_t& _textsize, const uint8_t *_code, size_t _codesize)
	{
		instance()->m_lzhuf.Decode(_text, _textsize, _code, _codesize);
	}
}
