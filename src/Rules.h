#pragma once

#include "Utils.h"

#include <map>
#include <memory>
#include <string>
#include <unordered_map>

enum RuleType : UInt32
{
	kRuleType_Actor = 0,
	kRuleType_Race,

	kRuleType_Max
};

struct RuleSet
{
	using MeshMap = std::map<std::string, std::string, Utils::CaseInsensitiveLess>;

	// "Type | Plugin | FormID : MeshesPath" - root folder under Data\Meshes\, lowercase, ends with '\'
	std::unordered_map<UInt32, std::string> paths[kRuleType_Max];

	// "Type | Plugin | FormID : MeshPath : CustomPath" - per-mesh replacement, both lowercase and relative to the meshes folder
	std::unordered_map<UInt32, MeshMap> meshes[kRuleType_Max];
};

namespace Rules
{
	inline constexpr const char* kRulesPath = "Data\\F4SE\\Plugins\\CustomMeshesPathF4_Rules.txt";

	// re-parses the rules file when its modification time changed since the last load
	void ReloadIfChanged();

	std::shared_ptr<const RuleSet> Get();
}
