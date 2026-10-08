#pragma once

#include <cstdint>

// Reads the "Address Library for F4SE Plugins" database (Data\F4SE\Plugins\version-1-10-984-0.bin)
namespace AddressLibrary
{
	bool Load(UInt32 a_runtimeVersion);

	// offset of a_id from the image base, or 0 when the id is unknown
	std::uint64_t GetOffset(std::uint64_t a_id);

	// id of the entry at a_offset, or 0 when there is none (slow, diagnostics only)
	std::uint64_t GetID(std::uint64_t a_offset);
}
