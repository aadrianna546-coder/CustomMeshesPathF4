#include "Rules.h"

#include "f4se/GameData.h"
#include "f4se/GameForms.h"

#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <mutex>
#include <sys/stat.h>

namespace Rules
{
	namespace
	{
		std::mutex g_lock;
		std::shared_ptr<const RuleSet> g_rules = std::make_shared<RuleSet>();
		__time64_t g_lastWriteTime = 0;

		// reads up to a_delim; a '#' comments out the rest of the line
		std::string NextField(const std::string& a_line, std::size_t& a_pos, char a_delim)
		{
			std::string field;
			while (a_pos < a_line.size()) {
				const char ch = a_line[a_pos];
				if (ch == '\0' || ch == '#') {
					a_pos = a_line.size();
					break;
				}

				++a_pos;
				if (ch == a_delim) {
					break;
				}
				field += ch;
			}

			Utils::Trim(field);
			return field;
		}

		std::string NormalizeMeshPath(const std::string& a_path)
		{
			auto path = Utils::ToLower(a_path);
			if (Utils::StartsWith(path, "meshes\\")) {
				path.erase(0, 7);
			}
			return path;
		}

		bool ParseFormID(const std::string& a_str, UInt32& a_out)
		{
			char* end = nullptr;
			errno = 0;
			const unsigned long value = std::strtoul(a_str.c_str(), &end, 16);
			if (end == a_str.c_str() || errno == ERANGE) {
				return false;
			}

			a_out = value & 0xFFFFFF;
			return true;
		}

		TESForm* LookupForm(const std::string& a_pluginName, UInt32 a_localID)
		{
			DataHandler* dataHandler = *g_dataHandler;
			const ModInfo* modInfo = dataHandler ? dataHandler->LookupModByName(a_pluginName.c_str()) : nullptr;
			if (!modInfo) {
				return nullptr;
			}

			return LookupFormByID(modInfo->GetFormID(a_localID));
		}

		std::shared_ptr<RuleSet> Parse(std::istream& a_in)
		{
			auto rules = std::make_shared<RuleSet>();

			std::string line;
			while (std::getline(a_in, line)) {
				Utils::Trim(line);
				if (line.empty() || line[0] == '#') {
					continue;
				}

				std::size_t pos = 0;

				const auto ruleType = NextField(line, pos, '|');
				if (ruleType.empty()) {
					_MESSAGE("Cannot read the ruleType - %s", line.c_str());
					continue;
				}

				const auto pluginName = NextField(line, pos, '|');
				if (pluginName.empty()) {
					_MESSAGE("Cannot read the pluginName - %s", line.c_str());
					continue;
				}

				const auto formIDStr = NextField(line, pos, ':');
				UInt32 localID = 0;
				if (formIDStr.empty() || !ParseFormID(formIDStr, localID)) {
					_MESSAGE("Cannot read the formId - %s", line.c_str());
					continue;
				}

				auto meshesPath = NextField(line, pos, ':');
				if (meshesPath.empty()) {
					_MESSAGE("Cannot read the meshesPath - %s", line.c_str());
					continue;
				}
				meshesPath = NormalizeMeshPath(meshesPath);

				const bool hasCustomPath = pos < line.size();
				std::string customPath;
				if (hasCustomPath) {
					customPath = NextField(line, pos, '\0');
					if (customPath.empty()) {
						_MESSAGE("Cannot read the customPath - %s", line.c_str());
						continue;
					}
					customPath = NormalizeMeshPath(customPath);
				}

				TESForm* form = LookupForm(pluginName, localID);
				if (!form) {
					_MESSAGE("Cannot find the Form - %s", line.c_str());
					continue;
				}

				RuleType type;
				if (_stricmp(ruleType.c_str(), "Actor") == 0) {
					type = kRuleType_Actor;
				} else if (_stricmp(ruleType.c_str(), "Race") == 0) {
					type = kRuleType_Race;
				} else {
					_MESSAGE("Unknown ruleType - %s", line.c_str());
					continue;
				}

				if (hasCustomPath) {
					rules->meshes[type][form->formID].emplace(meshesPath, customPath);
					_MESSAGE("ruleType[%s] pluginName[%s] formId[0x%08X] meshesPath[%s] customPath[%s]",
						ruleType.c_str(), pluginName.c_str(), form->formID, meshesPath.c_str(), customPath.c_str());
				} else {
					if (meshesPath.back() != '\\') {
						meshesPath += '\\';
					}
					rules->paths[type].emplace(form->formID, meshesPath);
					_MESSAGE("ruleType[%s] pluginName[%s] formId[0x%08X] meshesPath[%s]",
						ruleType.c_str(), pluginName.c_str(), form->formID, meshesPath.c_str());
				}
			}

			return rules;
		}
	}

	void ReloadIfChanged()
	{
		struct _stat64 info;
		if (_stat64(kRulesPath, &info) != 0) {
			_MESSAGE("No Rules file found");
			return;
		}

		if (g_lastWriteTime != 0 && g_lastWriteTime == info.st_mtime) {
			return;
		}
		g_lastWriteTime = info.st_mtime;

		_MESSAGE("Load Rules...");

		std::ifstream file(kRulesPath);
		if (!file) {
			_MESSAGE("No Rules file found");
			return;
		}

		std::shared_ptr<const RuleSet> rules = Parse(file);

		std::lock_guard<std::mutex> lock(g_lock);
		g_rules = std::move(rules);
	}

	std::shared_ptr<const RuleSet> Get()
	{
		std::lock_guard<std::mutex> lock(g_lock);
		return g_rules;
	}
}
