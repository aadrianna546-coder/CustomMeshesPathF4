#include "ModelProcessor.h"

#include "ModelCache.h"

#include "f4se/BSModelDB.h"
#include "f4se/GameRTTI.h"
#include "f4se/NiExtraData.h"
#include "f4se/NiObjects.h"

namespace ModelProcessor
{
	namespace
	{
		// Points the body morph data (BODYTRI) of a redirected model at the custom folder too,
		// when a matching .tri file exists there.
		bool PrefixBodyTri(NiStringExtraData* a_data, const std::string& a_triPath, const std::string& a_prefix)
		{
			if (a_prefix.empty() || a_triPath.find(a_prefix) != std::string::npos) {
				return false;
			}

			const std::string newPath = a_prefix + a_triPath;
			if (!Utils::FileExists("data\\meshes\\" + newPath)) {
				return false;
			}

			BSFixedString replacement(newPath.c_str());
			a_data->m_string.Release();
			a_data->m_string = replacement;
			return true;
		}

		void FixBodyTri(NiAVObject* a_root, const ActorInfo& a_info)
		{
			BSFixedString key("BODYTRI");
			NiExtraData* extraData = a_root->GetExtraData(key);
			key.Release();

			auto* bodyTri = static_cast<NiStringExtraData*>(Runtime_DynamicCast(extraData, RTTI_NiExtraData, RTTI_NiStringExtraData));
			if (!bodyTri) {
				return;
			}

			const char* current = bodyTri->m_string.c_str();
			const std::string triPath = current ? current : "";
			if (!PrefixBodyTri(bodyTri, triPath, a_info.actorPath)) {
				PrefixBodyTri(bodyTri, triPath, a_info.racePath);
			}
		}

		class CustomMeshesProcessor : public BSModelDB::BSModelProcessor
		{
		public:
			explicit CustomMeshesProcessor(BSModelDB::BSModelProcessor* a_next) :
				m_next(a_next)
			{}

			~CustomMeshesProcessor() override = default;

			void Process(BSModelDB::ModelData* a_modelData, const char* a_modelName, NiAVObject** a_root, UInt32* a_typeOut) override
			{
				ActorInfo info;
				if (a_modelName && a_root && *a_root && ModelCache::Find(a_modelName, info)) {
					FixBodyTri(*a_root, info);
				}

				if (m_next) {
					m_next->Process(a_modelData, a_modelName, a_root, a_typeOut);
				}
			}

		private:
			BSModelDB::BSModelProcessor* m_next;  // 08
		};
	}

	void Install()
	{
		static bool installed = false;
		if (installed) {
			return;
		}
		installed = true;

		BSModelDB::BSModelProcessor*& processor = *g_TESProcessor;
		processor = new CustomMeshesProcessor(processor);
		_MESSAGE("model processor installed");
	}
}
