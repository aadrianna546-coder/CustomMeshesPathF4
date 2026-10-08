#include "AddressLibrary.h"

#include <algorithm>
#include <fstream>
#include <vector>

namespace AddressLibrary
{
	namespace
	{
		struct Mapping
		{
			std::uint64_t id;
			std::uint64_t offset;
		};

		std::vector<Mapping> g_mappings;
	}

	bool Load(UInt32 a_runtimeVersion)
	{
		char path[MAX_PATH];
		sprintf_s(path, "Data\\F4SE\\Plugins\\version-%u-%u-%u-0.bin",
			(a_runtimeVersion >> 24) & 0xFF,
			(a_runtimeVersion >> 16) & 0xFF,
			(a_runtimeVersion >> 4) & 0xFFF);

		std::ifstream file(path, std::ios::in | std::ios::binary | std::ios::ate);
		if (!file) {
			_ERROR("couldn't open %s - install \"Address Library for F4SE Plugins\" for this game version", path);
			return false;
		}

		const auto size = static_cast<std::uint64_t>(file.tellg());
		file.seekg(0);

		std::uint64_t count = 0;
		file.read(reinterpret_cast<char*>(&count), sizeof(count));
		if (!file || size != sizeof(count) + count * sizeof(Mapping)) {
			_ERROR("%s has an unexpected format", path);
			return false;
		}

		g_mappings.resize(static_cast<std::size_t>(count));
		file.read(reinterpret_cast<char*>(g_mappings.data()), static_cast<std::streamsize>(count * sizeof(Mapping)));
		if (!file) {
			_ERROR("couldn't read %s", path);
			g_mappings.clear();
			return false;
		}

		const auto byID = [](const Mapping& a_lhs, const Mapping& a_rhs) { return a_lhs.id < a_rhs.id; };
		if (!std::is_sorted(g_mappings.begin(), g_mappings.end(), byID)) {
			std::sort(g_mappings.begin(), g_mappings.end(), byID);
		}

		_MESSAGE("loaded %s (%llu entries)", path, count);
		return true;
	}

	std::uint64_t GetOffset(std::uint64_t a_id)
	{
		const auto it = std::lower_bound(
			g_mappings.begin(),
			g_mappings.end(),
			a_id,
			[](const Mapping& a_lhs, std::uint64_t a_rhs) { return a_lhs.id < a_rhs; });

		return it != g_mappings.end() && it->id == a_id ? it->offset : 0;
	}

	std::uint64_t GetID(std::uint64_t a_offset)
	{
		for (const auto& mapping : g_mappings) {
			if (mapping.offset == a_offset) {
				return mapping.id;
			}
		}
		return 0;
	}
}
