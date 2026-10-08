#pragma once

#include "Rules.h"

#include <string>

inline constexpr UInt32 kInvalidFormID = 0xFFFFFFFF;

// rules that matched the actor whose 3D is being built on the current thread
struct ActorInfo
{
	UInt32 raceID = kInvalidFormID;
	std::string racePath;
	UInt32 actorID = kInvalidFormID;
	std::string actorPath;
};

struct ResolvedPath
{
	std::string prefixPath;  // passed to the game as the new folder prefix, e.g. "meshes\cwc\meshes\"
	std::string subPath;     // passed to the game as the new file name
	std::string fullPath;    // "data\" + prefixPath + subPath
};

// Finds a replacement for a mesh request (a_subPath is lowercase and relative to Data\Meshes\).
// Tries a per-mesh rule inside the custom folder, the per-mesh rule directly under Data\Meshes\,
// then the same file name inside the custom folder. Only loose files are considered.
bool ResolvePath(
	const RuleSet& a_rules,
	RuleType a_type,
	UInt32 a_formID,
	const std::string& a_customPath,
	const std::string& a_subPath,
	ResolvedPath& a_out);
