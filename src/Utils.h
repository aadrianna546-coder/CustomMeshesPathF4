#pragma once

#include <cstring>
#include <string>

namespace Utils
{
	std::string ToLower(std::string a_str);
	void Trim(std::string& a_str);
	bool StartsWith(const std::string& a_str, const std::string& a_prefix);

	// true when a_path names an existing loose file (archives are not searched)
	bool FileExists(const std::string& a_path);

	struct CaseInsensitiveLess
	{
		bool operator()(const std::string& a_lhs, const std::string& a_rhs) const
		{
			return _stricmp(a_lhs.c_str(), a_rhs.c_str()) < 0;
		}
	};
}
