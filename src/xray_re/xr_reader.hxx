#pragma once

#include "xr_types.hxx"

#include <functional>
#include <string>
#include <vector>

namespace xray_re
{
	class xr_packet;
	class xr_scrambler;

	class xr_reader
	{
	public:
		xr_reader();
		xr_reader(const void *data, std::size_t length);
		virtual ~xr_reader() = default;

		std::size_t find_chunk(uint32_t id, bool& compressed, bool reset = true);
		std::size_t find_chunk(uint32_t id);
		xr_reader* open_chunk(uint32_t id);
		xr_reader* open_chunk(uint32_t id, const xr_scrambler& scrambler);
		xr_reader* open_chunk_next(uint32_t& id, xr_reader *prev);
		void close_chunk(xr_reader*& r) const;

		std::size_t size() const;
		const void* data() const;
		void advance(std::size_t offset);
		void seek(std::size_t offset);
		bool eof() const;
		std::size_t tell() const;
		std::size_t elapsed() const;

		std::size_t r_raw_chunk(uint32_t id, void *dest, std::size_t dest_size);
		void r_raw(void* dest, std::size_t dest_size);

		template<typename T> T r();
		template<typename T> void r(T& value);
		template<typename T> const T* pointer() const;
		template<typename T> const T* skip(std::size_t n = 1);

		const char* skip_sz();
		void r_s(std::string& value);
		void r_sz(std::string& value);
		void r_sz(char *dest, std::size_t dest_size);
		uint32_t r_u32();
		int32_t r_s32();
		uint32_t r_u24();
		uint16_t r_u16();
		int16_t r_s16();
		uint8_t r_u8();
		int8_t r_s8();
		bool r_bool();
		float r_float();
		void r_packet(xr_packet& packet, std::size_t size);

	protected:
		const uint8_t *m_data{nullptr};
#if 1
		union
		{
			const uint8_t  *m_p{nullptr};
			const int8_t   *m_p_s8;
			const uint16_t *m_p_u16;
			const int16_t  *m_p_s16;
			const uint32_t *m_p_u32;
			const int32_t  *m_p_s32;
			const float    *m_p_f;
		};
#else
		const uint8_t *m_p{nullptr};
#endif
		const uint8_t *m_end{nullptr};
		const uint8_t *m_next{nullptr};

	private:
		const uint8_t *m_debug_find_chunk;
	};

	// for compressed chunks
	class xr_temp_reader: public xr_reader
	{
	public:
		xr_temp_reader(const uint8_t *data, std::size_t size);
		~xr_temp_reader() override;
	};

	inline std::size_t xr_reader::size() const { assert(m_data <= m_end); return static_cast<std::size_t>(m_end - m_data); }
	inline const void* xr_reader::data() const { return m_data; }
	inline void xr_reader::advance(std::size_t offset) { m_p += offset; assert(m_p <= m_end); }
	inline void xr_reader::seek(std::size_t offset) { m_p = m_data + offset; assert(m_p <= m_end); }
	inline bool xr_reader::eof() const { assert(m_p <= m_end); return m_p == m_end; }
	inline std::size_t xr_reader::tell() const { assert(m_p <= m_end); return static_cast<std::size_t>(m_p - m_data); }
	inline std::size_t xr_reader::elapsed() const { assert(m_p <= m_end); return static_cast<std::size_t>(m_end - m_p); }

	template<typename T> inline T xr_reader::r() { T value; r_raw(&value, sizeof(T)); return value; }
	template<typename T> inline void xr_reader::r(T& value) { value = *reinterpret_cast<const T*>(m_p); m_p += sizeof(T); }
	inline uint32_t xr_reader::r_u32() { return *m_p_u32++; }
	inline int32_t xr_reader::r_s32() { return *m_p_s32++; }
	inline uint32_t xr_reader::r_u24() { uint32_t u24 = 0; r_raw(&u24, 3); return u24; }
	inline uint16_t xr_reader::r_u16() { return *m_p_u16++; }
	inline int16_t xr_reader::r_s16() { return *m_p_s16++; }
	inline uint8_t xr_reader::r_u8() { return *m_p++; }
	inline int8_t xr_reader::r_s8() { return *m_p_s8++; }
	inline bool xr_reader::r_bool() { return *m_p++ != 0; }
	inline float xr_reader::r_float() { return *m_p_f++; }

	template<typename T> inline const T* xr_reader::pointer() const { return reinterpret_cast<const T*>(m_p); }
	template<typename T> inline const T* xr_reader::skip(std::size_t n)
	{
		const T* p = pointer<T>();
		advance(n*sizeof(T));
		return p;
	}
} // namespace xray_re
