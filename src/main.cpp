#include "AddressLibrary.h"
#include "Hooks.h"
#include "ModelCache.h"
#include "ModelProcessor.h"
#include "Rules.h"
#include "Settings.h"

#include "f4se/PluginAPI.h"
#include "f4se_common/f4se_version.h"

#include <shlobj.h>

namespace
{
	constexpr const char* kPluginName = "CustomMeshesPathF4";

	PluginHandle g_pluginHandle = kPluginHandle_Invalid;
	F4SEMessagingInterface* g_messaging = nullptr;

	void OnF4SEMessage(F4SEMessagingInterface::Message* a_msg)
	{
		switch (a_msg->type) {
		case F4SEMessagingInterface::kMessage_PreLoadGame:
		case F4SEMessagingInterface::kMessage_NewGame:
			ModelProcessor::Install();
			ModelCache::Clear();
			Rules::ReloadIfChanged();
			break;
		default:
			break;
		}
	}

	void OpenLog(const F4SEInterface* a_f4se)
	{
		const char* saveFolder = "Fallout4";
		if (a_f4se->f4seVersion >= MAKE_EXE_VERSION(0, 7, 1) && a_f4se->GetSaveFolderName) {
			if (const char* name = a_f4se->GetSaveFolderName(); name && *name) {
				saveFolder = name;
			}
		}

		char path[MAX_PATH];
		sprintf_s(path, "\\My Games\\%s\\F4SE\\%s.log", saveFolder, kPluginName);
		gLog.OpenRelative(CSIDL_MYDOCUMENTS, path);
		gLog.SetPrintLevel(IDebugLog::kLevel_Error);
		gLog.SetLogLevel(IDebugLog::kLevel_DebugMessage);
	}
}

extern "C"
{
	__declspec(dllexport) F4SEPluginVersionData F4SEPlugin_Version = {
		F4SEPluginVersionData::kVersion,

		MAKE_EXE_VERSION(PLUGIN_VERSION_MAJOR, PLUGIN_VERSION_MINOR, PLUGIN_VERSION_PATCH),
		"CustomMeshesPathF4",
		"",

		0,                                    // uses F4SE's 1.10.984 addresses
		0,                                    // uses 1.10.984 structure layouts
		{ RUNTIME_VERSION_1_10_984, 0 },

		0,
		0,
		0,
		{ 0 }
	};

	__declspec(dllexport) bool F4SEPlugin_Load(const F4SEInterface* a_f4se)
	{
		OpenLog(a_f4se);
		_MESSAGE("%s %d.%d.%d Loaded", kPluginName, PLUGIN_VERSION_MAJOR, PLUGIN_VERSION_MINOR, PLUGIN_VERSION_PATCH);

		if (a_f4se->isEditor) {
			_ERROR("loaded in editor, disabling");
			return false;
		}

		if (a_f4se->runtimeVersion != RUNTIME_VERSION_1_10_984) {
			_ERROR("unsupported runtime version %08X", a_f4se->runtimeVersion);
			return false;
		}

		g_pluginHandle = a_f4se->GetPluginHandle();
		g_messaging = static_cast<F4SEMessagingInterface*>(a_f4se->QueryInterface(kInterface_Messaging));
		if (!g_messaging) {
			_ERROR("couldn't get messaging interface");
			return false;
		}

		Settings::Load();

		if (!AddressLibrary::Load(a_f4se->runtimeVersion) || !Hooks::Install()) {
			_ERROR("couldn't install hooks, plugin disabled");
			return false;
		}

		g_messaging->RegisterListener(g_pluginHandle, "F4SE", OnF4SEMessage);
		return true;
	}
}
