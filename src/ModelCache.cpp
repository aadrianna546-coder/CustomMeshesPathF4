#include "ModelCache.h"

#include <map>
#include <mutex>

namespace ModelCache
{
	namespace
	{
		std::mutex g_lock;
		std::map<std::string, ActorInfo, Utils::CaseInsensitiveLess> g_models;
	}

	void Insert(const std::string& a_fullPath, const ActorInfo& a_info)
	{
		std::lock_guard<std::mutex> lock(g_lock);
		g_models[a_fullPath] = a_info;
	}

	bool Find(const char* a_modelName, ActorInfo& a_out)
	{
		std::lock_guard<std::mutex> lock(g_lock);
		const auto it = g_models.find(a_modelName);
		if (it == g_models.end()) {
			return false;
		}

		a_out = it->second;
		return true;
	}

	void Clear()
	{
		std::lock_guard<std::mutex> lock(g_lock);
		g_models.clear();
	}
}
