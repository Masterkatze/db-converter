#pragma once

#include "xr_types.hxx"
#include "xr_reader.hxx"
#include "xr_writer.hxx"

#include <string>
#include <vector>
#include <ostream>

namespace xray_re
{
	struct SplitPath
	{
		std::string folder;
		std::string name;
		std::string extension;
	};

	struct PathAlias
	{
		std::string path;
		std::string root;
		std::string filter;
		std::string caption;

		PathAlias(const std::string& path, const std::string& root, const std::string& filter, const std::string& caption);

		std::string ToString() const;
	};

	class xr_file_system
	{
	public:
		xr_file_system();
		~xr_file_system();

		static xr_file_system& instance();

		bool is_read_only() const;
		void set_read_only(bool is_read_only);

		static xr_reader* r_open(const std::string& path);
		xr_reader* r_open(const std::string& path, const std::string& name) const;
		static void r_close(xr_reader*& reader);
		xr_writer* w_open(const std::string& path, bool ignore_ro = false) const;
		xr_writer* w_open(const std::string& path, const std::string& name, bool ignore_ro = false) const;
		static void w_close(xr_writer*& writer);

		bool copy_file(const std::string& src_path, const std::string& src_name, const std::string& dst_path, const std::string& tgt_name = nullptr) const;
		bool copy_file(const std::string& src_path, const std::string& dst_path) const;

		static size_t file_length(const std::string& path);
		static uint32_t file_age(const std::string& path);
		static bool file_exist(const std::string& path);
		static bool folder_exist(const std::string& path);
		bool create_path(const std::string& path) const;
		bool create_folder(const std::string& path) const;
		bool resolve_path(const std::string& path, const std::string& name, std::string& full_path) const;
		static void append_path_separator(std::string& path);
		static SplitPath split_path(const std::string& path);
		static std::string current_path();

	private:
		PathAlias& add_path_alias(const std::string& path, const std::string& root, const std::string& add);

		std::vector<PathAlias> m_aliases;
		bool m_is_read_only;
	};
}
