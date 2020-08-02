#pragma once

#include "xr_types.hxx"

#include <string>
#include <vector>
#include <stack>
#include <functional>

namespace xray_re
{
	class xr_packet;

	class xr_writer
	{
	public:
		xr_writer();
		virtual ~xr_writer();

		virtual void w_raw(const void *data, size_t size) = 0;
		virtual void seek(size_t pos) = 0;
		virtual size_t tell() = 0;

		void open_chunk(uint32_t id);
		void close_chunk();
		void w_raw_chunk(uint32_t id, const void *data, size_t size);
		void w_chunk(uint32_t id, const std::string& s);

		template<typename T> void w_chunk(uint32_t id, const T& value);
		template<typename T, typename F> void w_chunks(const T& container, F write);
		template<typename T, typename F> void w_seq(const T& container, F write);
		template<typename T> void w_seq(const T& container);
		template<typename T> void w_cseq(size_t n, const T values[]);
		template<typename T, typename F> void w_cseq(size_t n, const T values[], F write);
		template<typename T> void w(const T& value);

		void w_sz(const std::string& value);
		void w_sz(const char *value);
		void w_sf(const char *format, ...);
		void w_s(const std::string& value);
		void w_s(const char *value);
		void w_u32(uint32_t value);
		void w_s32(int32_t value);
		void w_u24(uint32_t value);
		void w_u16(uint16_t value);
		void w_s16(int16_t value);
		void w_u8(uint8_t value);
		void w_s8(int8_t value);
		void w_bool(bool value);
		void w_float(float value);
		void w_float_q16(float value, float min = 0, float max = 1.f);
		void w_float_q8(float value, float min = 0, float max = 1.f);
		void w_size_u32(size_t value);
		void w_size_u16(size_t value);
		void w_size_u8(size_t value);

		void w_packet(const xr_packet& packet);

		template<typename T> struct f_w: public std::binary_function<T, xr_writer, void> {};

		struct f_w_sz: public f_w<std::string>
		{
			void operator()(const std::string& s, xr_writer& w) { w.w_sz(s); }
		};

		template<typename T> struct f_w_const: public std::const_mem_fun1_t<void, T, xr_writer&>
		{
			explicit f_w_const(void (T::*_pmf)(xr_writer& w) const): std::const_mem_fun1_t<void, T, xr_writer&>(_pmf) {}
		};

	private:
		std::stack<size_t> m_open_chunks;
	};

	class xr_fake_writer: public xr_writer
	{
	public:
		xr_fake_writer();
		virtual ~xr_fake_writer();
		virtual void w_raw(const void *data, size_t size);
		virtual void seek(size_t pos);
		virtual size_t tell();

	private:
		size_t m_pos;
		size_t m_size;
	};

	class xr_memory_writer: public xr_writer
	{
	public:
		xr_memory_writer();
		~xr_memory_writer();

		virtual void w_raw(const void *data, size_t size);
		virtual void seek(size_t pos);
		virtual size_t tell();

		const uint8_t* data() const;

		bool save_to(const char *path);
		bool save_to(const std::string& path);
		bool save_to(const char *path, const std::string& name);

	private:
		std::vector<uint8_t> m_buffer;
		size_t m_pos;
	};

	inline xr_writer::xr_writer() {}
	inline xr_writer::~xr_writer() {}
	template<typename T> inline void xr_writer::w(const T& value) { w_raw(&value, sizeof(T)); }
	inline void xr_writer::w_u32(uint32_t value) { w<uint32_t>(value); }
	inline void xr_writer::w_s32(int32_t value) { w<int32_t>(value); }
	inline void xr_writer::w_u24(uint32_t value) { w_raw(&value, 3); }
	inline void xr_writer::w_u16(uint16_t value) { w<uint16_t>(value); }
	inline void xr_writer::w_s16(int16_t value) { w<int16_t>(value); }
	inline void xr_writer::w_u8(uint8_t value) { w<uint8_t>(value); }
	inline void xr_writer::w_s8(int8_t value) { w<int8_t>(value); }
	inline void xr_writer::w_bool(bool value) { w_u8(value ? 1 : 0); }
	inline void xr_writer::w_float(float value) { w<float>(value); }
	inline void xr_writer::w_size_u32(size_t value) { w_u32(static_cast<uint32_t>(value & UINT32_MAX)); }
	inline void xr_writer::w_size_u16(size_t value) { w_u16(static_cast<uint16_t>(value & UINT16_MAX)); }
	inline void xr_writer::w_size_u8(size_t value) { w_u8(static_cast<uint8_t>(value & UINT8_MAX)); }

	template<typename T> inline void xr_writer::w_cseq(size_t n, const T values[])
	{
		if (n)
			w_raw(values, n*sizeof(T));
	}

	template<typename T, typename F> inline void xr_writer::w_cseq(size_t n, const T values[], F write)
	{
		for (const T *p = values, *end = p + n; p != end; ++p)
			write(*p, *this);
	}

	template<typename T> inline void xr_writer::w_seq(const T& container)
	{
		if (!container.empty())
			w_raw(&container[0], container.size()*sizeof(typename T::value_type));
	}

	template<typename T, typename F> inline void xr_writer::w_seq(const T& container, F write)
	{
		for (typename T::const_iterator it = container.begin(), end = container.end(); it != end; ++it)
		{
			write(*it, *this);
		}
	}

	inline void xr_writer::w_chunk(uint32_t id, const std::string& s)
	{
		open_chunk(id);
		w_sz(s);
		close_chunk();
	}

	template<typename T> inline void xr_writer::w_chunk(uint32_t id, const T& value)
	{
		w_raw_chunk(id, &value, sizeof(T));
	}

	template<typename T, typename F> inline void xr_writer::w_chunks(const T& container, F write)
	{
		typename T::const_iterator it = container.begin(), end = container.end();
		for (uint32_t id = 0; it != end; ++it)
		{
			open_chunk(id++);
			write(*it, *this);
			close_chunk();
		}
	}

	inline const uint8_t* xr_memory_writer::data() const { return &m_buffer[0]; }
}
