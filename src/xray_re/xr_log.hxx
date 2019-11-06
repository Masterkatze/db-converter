#pragma once

#include <string>
#include <cstdarg>
#include "xr_types.hxx"

namespace xray_re
{
	void die(const char* msg, const char* file, unsigned line);
	void msg(const char* format, ...);
	void dbg(const char* format, ...);

	class xr_writer;

	class xr_log
	{
	public:
		xr_log();
		~xr_log();
		static xr_log& instance();

		void init(std::string name, std::string prefix);

		void diagnostic(const char* format, ...);
		void diagnostic(const char* format, va_list ap);

		void fatal(const char* msg, const char* file, unsigned line);

	private:
		char m_buf[1023 + 1];
		size_t m_buf_size;
		char *m_buf_p;
		xr_writer *m_log;
	};

	inline xr_log::xr_log(): m_buf(), m_buf_size(sizeof(m_buf) - 1), m_buf_p(m_buf), m_log(nullptr) {}

	inline xr_log& xr_log::instance()
	{
		static xr_log instance0;
		return instance0;
	}
}
