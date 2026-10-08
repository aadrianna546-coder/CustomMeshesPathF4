#pragma once

#include "PathResolver.h"

// remembers which actor rules redirected a model, so the model processor can fix its BODYTRI path
namespace ModelCache
{
	void Insert(const std::string& a_fullPath, const ActorInfo& a_info);
	bool Find(const char* a_modelName, ActorInfo& a_out);
	void Clear();
}
