#pragma once

namespace Settings
{
	inline constexpr const char* kIniPath = "Data\\F4SE\\Plugins\\CustomMeshesPathF4.ini";

	// [Debug] bDebug - log every path resolution
	extern bool debug;

	// [Advanced] Address Library ID overrides for the two hooked game functions (0 = built-in value)
	extern UInt32 actorLoadID;
	extern UInt32 meshPathID;

	void Load();
}
