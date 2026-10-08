#include "Utils.h"

#include <algorithm>
#include <cctype>

namespace Utils
{
	std::string ToLower(std::string a_str)
	{
		std::transform(a_str.begin(), a_str.end(), a_str.begin(), [](char a_ch) {
			return static_cast<char>(std::tolower(static_cast<unsigned char>(a_ch)));
		});
		return a_str;
	}

	void Trim(std::string& a_str)
	{
		const auto isSpace = [](char a_ch) { return std::isspace(static_cast<unsigned char>(a_ch)) != 0; };

		const auto first = std::find_if_not(a_str.begin(), a_str.end(), isSpace);
		a_str.erase(a_str.begin(), first);

		const auto last = std::find_if_not(a_str.rbegin(), a_str.rend(), isSpace);
		a_str.erase(last.base(), a_str.end());
	}

	bool StartsWith(const std::string& a_str, const std::string& a_prefix)
	{
		return a_str.compare(0, a_prefix.size(), a_prefix) == 0;
	}

	bool FileExists(const std::string& a_path)
	{
		if (a_path.empty()) {
			return false;
		}

		const DWORD attributes = GetFileAttributesA(a_path.c_str());
		return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
	}
}
