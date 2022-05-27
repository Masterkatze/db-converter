#include "xr_utils.hxx"

bool xray_re::is_xrp(const std::string& extension)
{
	return extension == ".xrp";
}

bool xray_re::is_xp(const std::string& extension)
{
	return (extension.size() == 2 && extension == "xp") ||
	       (extension.size() == 3 && extension.compare(0, 2, "xp") == 0 && std::isalnum(extension.at(3)));
}

bool xray_re::is_xdb(const std::string& extension)
{
	return (extension.size() == 3 && extension == "xdb") ||
	       (extension.size() == 4 && extension.compare(0, 3, "xdb") == 0 && std::isalnum(extension.at(4)));
}

bool xray_re::is_db(const std::string& extension)
{
	return (extension.size() == 2 && extension == "db") ||
	       (extension.size() == 3 && extension.compare(0, 2, "db") == 0 && std::isalnum(extension.at(3)));
}

bool xray_re::is_known(const std::string& extension)
{
	return is_db(extension) || is_xdb(extension) || is_xrp(extension) || is_xp(extension);
}
