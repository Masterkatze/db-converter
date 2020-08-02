/**************************************************************
	lzhuf.c
	written by Haruyasu Yoshizaki 11/20/1988
	some minor changes 4/6/1989
	comments translated by Haruhiko Okumura 4/7/1989
**************************************************************/

/*
LZHUF.C (c)1989 by Haruyasu Yoshizaki, Haruhiko Okumura, and Kenji Rikitake.
All rights reserved. Permission granted for non-commercial use.
*/

#include "xr_lzhuf.hxx"

#include <cstring>
#include <cstdlib>

using namespace xray_re;

void _lzhuf::InitTree()
{
	int i;

	for(i = N + 1; i <= N + 256; i++)
	{
		rson[i] = NIL; // Root
	}

	for(i = 0; i < N; i++)
	{
		dad[i] = NIL; // Node
	}
}

void _lzhuf::InsertNode(int r)
{
	int cmp = 1;
	auto key = &text_buf[r];
	int p = N + 1 + key[0];
	rson[r] = lson[r] = NIL;
	match_length = 0;
	while(true)
	{
		if(cmp >= 0)
		{
			if(rson[p] != NIL)
			{
				p = rson[p];
			}
			else
			{
				rson[p] = r;
				dad[r] = p;
				return;
			}
		}
		else
		{
			if(lson[p] != NIL)
			{
				p = lson[p];
			}
			else
			{
				lson[p] = r;
				dad[r] = p;
				return;
			}
		}

		uint32_t i;
		for(i = 1; i < F; i++)
		{
			if((cmp = key[i] - text_buf[p + i]) != 0)
			{
				break;
			}
		}

		if(i > THRESHOLD)
		{
			if(i > match_length)
			{
				match_position = ((r - p) & (N - 1)) - 1;
				if((match_length = i) >= F) // ????
				{
					break;
				}
			}
			if(i == match_length)
			{
				int c = ((r - p) & (N - 1)) - 1;
				if(c  < match_position)
				{
					match_position = c;
				}
			}
		}
	}
	dad[r] = dad[p];
	lson[r] = lson[p];
	rson[r] = rson[p];
	dad[lson[p]] = r;
	dad[rson[p]] = r;

	if(rson[dad[p]] == p)
	{
		rson[dad[p]] = r;
	}
	else
	{
		lson[dad[p]] = r;
	}

	dad[p] = NIL;  // Remove p
}

void _lzhuf::DeleteNode(int p)
{
	int q;

	if(dad[p] == NIL)
	{
		return; // Not registered
	}

	if(rson[p] == NIL)
	{
		q = lson[p];
	}
	else if(lson[p] == NIL)
	{
		q = rson[p];
	}
	else
	{
		q = lson[p];
		if(rson[q] != NIL)
		{
			do
			{
				q = rson[q];
			}
			while (rson[q] != NIL);

			rson[dad[q]] = lson[q];
			dad[lson[q]] = dad[q];
			lson[q] = lson[p];
			dad[lson[p]] = q;
		}

		rson[q] = rson[p];
		dad[rson[p]] = q;
	}

	dad[q] = dad[p];

	if(rson[dad[p]] == p)
	{
		rson[dad[p]] = q;
	}
	else
	{
		lson[dad[p]] = q;
	}

	dad[p] = NIL;
}

// Huffman coding

// Table for encoding and decoding the upper 6 bits of position

// For encoding
const std::array<uint8_t, 64> _lzhuf::p_len =
{
	0x03, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05,
	0x05, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x06,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
	0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08
};

const std::array<uint8_t, 64> _lzhuf::p_code =
{
	0x00, 0x20, 0x30, 0x40, 0x50, 0x58, 0x60, 0x68,
	0x70, 0x78, 0x80, 0x88, 0x90, 0x94, 0x98, 0x9C,
	0xA0, 0xA4, 0xA8, 0xAC, 0xB0, 0xB4, 0xB8, 0xBC,
	0xC0, 0xC2, 0xC4, 0xC6, 0xC8, 0xCA, 0xCC, 0xCE,
	0xD0, 0xD2, 0xD4, 0xD6, 0xD8, 0xDA, 0xDC, 0xDE,
	0xE0, 0xE2, 0xE4, 0xE6, 0xE8, 0xEA, 0xEC, 0xEE,
	0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
	0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

// For decoding
const std::array<uint8_t, 256> _lzhuf::d_code =
{
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
	0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
	0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
	0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A,
	0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B,
	0x0C, 0x0C, 0x0C, 0x0C, 0x0D, 0x0D, 0x0D, 0x0D,
	0x0E, 0x0E, 0x0E, 0x0E, 0x0F, 0x0F, 0x0F, 0x0F,
	0x10, 0x10, 0x10, 0x10, 0x11, 0x11, 0x11, 0x11,
	0x12, 0x12, 0x12, 0x12, 0x13, 0x13, 0x13, 0x13,
	0x14, 0x14, 0x14, 0x14, 0x15, 0x15, 0x15, 0x15,
	0x16, 0x16, 0x16, 0x16, 0x17, 0x17, 0x17, 0x17,
	0x18, 0x18, 0x19, 0x19, 0x1A, 0x1A, 0x1B, 0x1B,
	0x1C, 0x1C, 0x1D, 0x1D, 0x1E, 0x1E, 0x1F, 0x1F,
	0x20, 0x20, 0x21, 0x21, 0x22, 0x22, 0x23, 0x23,
	0x24, 0x24, 0x25, 0x25, 0x26, 0x26, 0x27, 0x27,
	0x28, 0x28, 0x29, 0x29, 0x2A, 0x2A, 0x2B, 0x2B,
	0x2C, 0x2C, 0x2D, 0x2D, 0x2E, 0x2E, 0x2F, 0x2F,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
};

const std::array<uint8_t, 256> _lzhuf::d_len =
{
	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
	0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
	0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
	0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
	0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
	0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
	0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
	0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
};

int _lzhuf::GetBit()
{
	int i;

	while (getlen <= 8)
	{
		if((i = getc()) < 0)
		{
			i = 0;
		}

		getbuf |= static_cast<unsigned int>(i << (8 - getlen));
		getlen += 8;
	}

	i = static_cast<int>(getbuf);
	getbuf <<= 1;
	getlen--;

	return (i >> 15) & 1;
}

int _lzhuf::GetByte()
{
	int i;

	while (getlen <= 8)
	{
		int c = getc();
		i = (c < 0) ? 0 : c;
		getbuf |= static_cast<unsigned int>(i << (8 - getlen));
		getlen += 8;
	}

	i = static_cast<int>(getbuf);
	getbuf <<= 8;
	getlen -= 8;

	return (i & 0xff00) >> 8;
}

// Output c bits of code
void _lzhuf::Putcode(int l, unsigned int c)
{
	putbuf |= c >> putlen;
	if((putlen += l) >= 8)
	{
		putc(putbuf >> 8);
		if((putlen -= 8) >= 8)
		{
			putc(static_cast<int>(putbuf));
			codesize += 2;
			putlen -= 8;
			putbuf = c << (l - putlen);
		}
		else
		{
			putbuf <<= 8;
			codesize++;
		}
	}
}

void _lzhuf::StartHuff()
{
	int i, j;

	for(i = 0; i < N_CHAR; i++)
	{
		freq[i] = 1;
		son[i] = i + T;
		prnt[i + T] = i;
	}

	i = 0; j = N_CHAR;

	while (j <= R)
	{
		freq[j] = freq[i] + freq[i + 1];
		son[j] = i;
		prnt[i] = prnt[i + 1] = j;
		i += 2; j++;
	}

	freq[T] = 0xffff;
	prnt[R] = 0;
}

void _lzhuf::reconst()
{
	int i, j, k;
	unsigned int f, l;

	// Collect leaf nodes in the first half of the table and replace the freq by (freq + 1) / 2.
	j = 0;
	for(i = 0; i < T; i++)
	{
		if(son[i] >= T)
		{
			freq[j] = (freq[i] + 1) / 2;
			son[j] = son[i];
			j++;
		}
	}

	// Begin constructing tree by connecting sons
	for(i = 0, j = N_CHAR; j < T; i += 2, j++)
	{
		k = i + 1;
		f = freq[j] = freq[i] + freq[k];

		k = j - 1;
		while (f < freq[k])
		{
			k--;
		}
		k++;

		l = static_cast<unsigned int>(j - k) * sizeof(freq[0]);
		std::memmove(&freq[k + 1], &freq[k], l);
		freq[k] = f;
		std::memmove(&son[k + 1], &son[k], l);
		son[k] = i;
	}

	// Connect prnt
	for(i = 0; i < T; i++)
	{
		if((k = son[i]) >= T)
		{
			prnt[k] = i;
		}
		else
		{
			prnt[k] = prnt[k + 1] = i;
		}
	}
}


// Increment frequency of given code by one, and update tree
void _lzhuf::update(int c)
{
	int i, j, l;
	unsigned int k;

	if(freq[R] == MAX_FREQ)
	{
		reconst();
	}
	c = prnt[c + T];

	do
	{
		k = ++freq[c];

		// If the order is disturbed, exchange nodes
		if(k > freq[l = c + 1])
		{
			while (k > freq[++l]);
			l--;
			freq[c] = freq[l];
			freq[l] = k;

			i = son[c];
			prnt[i] = l;
			if(i < T)
			{
				prnt[i + 1] = l;
			}

			j = son[l];
			son[l] = i;

			prnt[j] = c;
			if(j < T)
			{
				prnt[j + 1] = c;
			}
			son[c] = j;

			c = l;
		}
	}
	while ((c = prnt[c]) != 0);	// Repeat up to root
}

void _lzhuf::EncodeChar(unsigned int c)
{
	unsigned i;
	int j, k;

	i = 0;
	j = 0;
	k = prnt[c + T];

	// Travel from leaf to root
	do
	{
		i >>= 1;

		// If node's address is odd-numbered, choose bigger brother node
		if(k & 1)
		{
			i += 0x8000;
		}

		j++;
	}
	while ((k = prnt[k]) != R);

	Putcode(j, i);
//	code = i;
//	len = j;
	update(static_cast<int>(c));
}

void _lzhuf::EncodePosition(unsigned int c)
{
	unsigned i;

	// Output upper 6 bits by table lookup
	i = c >> 6;
	Putcode(p_len.at(i), static_cast<unsigned int>(p_code.at(i)) << 8);

	// Output lower 6 bits verbatim
	Putcode(6, (c & 0x3f) << 10);
}

void _lzhuf::EncodeEnd()
{
	if(putlen)
	{
		putc(putbuf >> 8);
		codesize++;
	}
}

int _lzhuf::DecodeChar()
{
	int c;

	c = son[R];

	// Travel from root to leaf, choosing the smaller child node (son[]) if the read bit is 0, the bigger (son[]+1} if 1
	while (c < T)
	{
		c += GetBit();
		c = son[c];
	}

	c -= T;
	update(c);
	return c;
}

int _lzhuf::DecodePosition()
{
	int i, j, c;

	// recover upper 6 bits from table
	i = GetByte();
	c = d_code.at(i) << 6;
	j = d_len.at(i);

	// read lower 6 bits verbatim
	j -= 2;
	while (j--)
	{
		i = (i << 1) + GetBit();
	}
	return c | (i & 0x3f);
}

void _lzhuf::Encode(uint8_t *&_code, uint32_t& _codesize, const uint8_t *_text, uint32_t _textsize)
{
	int i, c, r, s, last_match_length;
	uint32_t len;

	m_src_limit = _textsize;
	m_src_pos = 0;
	m_src = _text;

	m_dest_limit = _textsize/2;
	m_dest_pos = 4;
	m_dest = static_cast<uint8_t*>(malloc(m_dest_limit));
	*(uint32_t*)m_dest = uint32_t(_textsize);

	putbuf = 0;
	putlen = 0;

	StartHuff();
	InitTree();
	s = 0;
	r = N - F;
	for(i = s; i < r; i++)
	{
		text_buf[i] = ' ';
	}

	for(len = 0; len < F && (c = getc()) >= 0; len++)
	{
		text_buf[r + len] = static_cast<unsigned char>(c);
	}

	textsize = len;

	for(i = 1; i <= F; i++)
	{
		InsertNode(r - i);
	}

	InsertNode(r);

	do
	{
		if(match_length > len)
		{
			match_length = len;
		}
		if(match_length <= THRESHOLD)
		{
			match_length = 1;
			EncodeChar(text_buf[r]);
		}
		else
		{
			EncodeChar(static_cast<unsigned int>(255 - THRESHOLD + match_length));
			EncodePosition(static_cast<unsigned int>(match_position));
		}

		last_match_length = match_length;
		for(i = 0; i < last_match_length && (c = getc()) >= 0; i++)
		{
			DeleteNode(s);
			text_buf[s] = static_cast<unsigned char>(c);

			if(s < F - 1)
			{
				text_buf[s + N] = static_cast<unsigned char>(c);
			}

			s = (s + 1) & (N - 1);
			r = (r + 1) & (N - 1);
			InsertNode(r);
		}
		while (i++ < last_match_length)
		{
			DeleteNode(s);
			s = (s + 1) & (N - 1);
			r = (r + 1) & (N - 1);

			if(--len)
			{
				InsertNode(r);
			}
		}
	}
	while (len > 0);
	EncodeEnd();
	_code = m_dest;
	_codesize = m_dest_pos;
}

void _lzhuf::Decode(uint8_t *&_text, uint32_t& _textsize, const uint8_t *_code, uint32_t _codesize)
{
	int i, j, k, r, c;
	uint32_t count;

	m_dest_limit = textsize = *(uint32_t*)_code;
	m_dest = static_cast<uint8_t*>(malloc(m_dest_limit));
	m_dest_pos = 0;

	m_src_limit = codesize = _codesize;
	m_src = _code;
	m_src_pos = 4;

	getbuf = 0;
	getlen = 0;

	StartHuff();
	for(i = 0; i < N - F; i++)
	{
		text_buf[i] = ' ';
	}

	r = N - F;
	for(count = 0; count < textsize;)
	{
		c = DecodeChar();
		if(c < 256)
		{
			putc(c);
			text_buf[r++] = static_cast<unsigned char>(c);
			r &= (N - 1);
			count++;
		}
		else
		{
			i = (r - DecodePosition() - 1) & (N - 1);
			j = c - 255 + THRESHOLD;

			for(k = 0; k < j; k++)
			{
				c = text_buf[(i + k) & (N - 1)];
				putc(c);
				text_buf[r++] = static_cast<unsigned char>(c);
				r &= (N - 1);
				count++;
			}
		}
	}
	xr_assert(m_dest_pos == textsize);
	_text = m_dest;
	_textsize = textsize;
}

int _lzhuf::getc()
{
	return m_src_pos < m_src_limit ? m_src[m_src_pos++] : -1;
}

void _lzhuf::putc(int c)
{
	if(m_dest_pos >= m_dest_limit)
	{
		m_dest_limit = m_dest_pos*2;
		m_dest = static_cast<uint8_t*>(realloc(m_dest, m_dest_limit));
		
	}

	xr_assert(m_dest_pos < m_dest_limit);
	m_dest[m_dest_pos++] = static_cast<unsigned char>(c);
}

xr_lzhuf* xr_lzhuf::instance()
{
	static xr_lzhuf instance;
	return &instance;
}
