#include "PathResolver.h"

#include "Settings.h"

namespace
{
	const char* TypeName(RuleType a_type)
	{
		return a_type == kRuleType_Actor ? "Actor" : "Race";
	}

	bool TryPath(const std::string& a_prefixPath, const std::string& a_subPath, ResolvedPath& a_out)
	{
		a_out.prefixPath = a_prefixPath;
		a_out.subPath = a_subPath;
		a_out.fullPath = "data\\" + a_out.prefixPath + a_out.subPath;
		return Utils::FileExists(a_out.fullPath);
	}
}

bool ResolvePath(
	const RuleSet& a_rules,
	RuleType a_type,
	UInt32 a_formID,
	const std::string& a_customPath,
	const std::string& a_subPath,
	ResolvedPath& a_out)
{
	if (a_customPath.empty()) {
		return false;
	}

	const std::string customPrefix = "meshes\\" + a_customPath;

	bool found = false;
	const auto& meshRules = a_rules.meshes[a_type];
	const auto rulesIt = meshRules.find(a_formID);
	if (rulesIt != meshRules.end()) {
		const auto meshIt = rulesIt->second.find(a_subPath);
		if (meshIt != rulesIt->second.end()) {
			found = TryPath(customPrefix, meshIt->second, a_out) || TryPath("meshes\\", meshIt->second, a_out);
		}
	}

	if (!found) {
		found = TryPath(customPrefix, a_subPath, a_out);
	}

	if (Settings::debug) {
		if (found) {
			_MESSAGE("Info::type[%s] cId[0x%08X] cPath[%s] subPath[%s] o_prefixPath[%s] o_subPath[%s] o_fullPath[%s]",
				TypeName(a_type), a_formID, a_customPath.c_str(), a_subPath.c_str(),
				a_out.prefixPath.c_str(), a_out.subPath.c_str(), a_out.fullPath.c_str());
		} else {
			_MESSAGE("Info::type[%s] cId[0x%08X] cPath[%s] subPath[%s] set default path...",
				TypeName(a_type), a_formID, a_customPath.c_str(), a_subPath.c_str());
		}
	}

	return found;
}
