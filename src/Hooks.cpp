#include "Hooks.h"

#include "AddressLibrary.h"
#include "ModelCache.h"
#include "PathResolver.h"
#include "Settings.h"

#include "f4se/GameExtraData.h"
#include "f4se/GameForms.h"
#include "f4se/GameReferences.h"
#include "f4se_common/Relocation.h"

#include "MinHook.h"
#include "hde64.h"

#include <cstdint>
#include <map>
#include <set>

namespace Hooks
{
	namespace
	{
		// Address Library IDs (1.10.980+). The 1.10.163 build hooked 0x005B93F0 (ID 203719) and 0x01B7CD40 (ID 1053009).
		//
		// ActorLoad: TESNPC code that builds an actor's 3D, (unknown*, Actor*). NG ID derived from the surrounding
		// TESNPC functions, which map 1:1 between both runtimes.
		constexpr std::uint64_t kActorLoadID = 2207456;

		// MeshPath: BSResource function that opens a file relative to a folder prefix, (?, ?, const char* name, const char* prefix).
		// It sits a few functions before BSResource::RegisterLocation (2269481); the exact ID is confirmed at runtime
		// by checking which candidate the game calls with a "meshes\" prefix.
		constexpr std::uint64_t kMeshPathIDs[] = {
			2269474, 2269473, 2269475, 2269472, 2269476, 2269471, 2269477, 2269470, 2269478, 2269469,
			2269479, 2269468, 2269480, 2269467, 2269466, 2269465, 2269464
		};

		struct ActorContext
		{
			std::shared_ptr<const RuleSet> rules;
			ActorInfo info;
		};

		thread_local const ActorContext* t_context = nullptr;

		using ActorLoad_t = std::uint64_t (*)(void*, Actor*, void*, void*);
		using MeshPath_t = std::uint64_t (*)(void*, void*, const char*, const char*);

		ActorLoad_t _ActorLoad = nullptr;
		MeshPath_t _MeshPath = nullptr;

		struct Section
		{
			std::uintptr_t begin = 0;
			std::uintptr_t end = 0;

			bool Contains(std::uintptr_t a_address) const { return a_address >= begin && a_address < end; }
		};

		// ---- detours ----

		UInt32 GetActorBaseID(Actor* a_actor)
		{
			TESForm* baseForm = a_actor->baseForm;
			UInt32 formID = baseForm ? baseForm->formID : kInvalidFormID;

			// temporary (leveled) bases: use the base they were generated from
			if ((!baseForm || (formID & 0xFF000000) == 0xFF000000) && a_actor->extraDataList) {
				BSExtraData* extra = a_actor->extraDataList->GetByType(kExtraData_LeveledCreature);
				if (extra) {
					auto* originalBase = *reinterpret_cast<TESForm**>(reinterpret_cast<std::uintptr_t>(extra) + 0x18);  // ExtraLeveledCreature::originalBase
					if (originalBase) {
						formID = originalBase->formID;
					}
				}
			}

			return formID;
		}

		std::uint64_t ActorLoad_Hook(void* a_this, Actor* a_actor, void* a_arg3, void* a_arg4)
		{
			const auto actorAddress = reinterpret_cast<std::uintptr_t>(a_actor);
			if (actorAddress < 0x10000 || (actorAddress & 7) != 0 || a_actor->formType != kFormType_ACHR) {
				return _ActorLoad(a_this, a_actor, a_arg3, a_arg4);
			}

			ActorContext context;
			context.rules = Rules::Get();

			ActorInfo& info = context.info;
			info.raceID = a_actor->race ? a_actor->race->formID : kInvalidFormID;
			info.actorID = GetActorBaseID(a_actor);

			bool matched = false;
			if (info.raceID != kInvalidFormID) {
				const auto& paths = context.rules->paths[kRuleType_Race];
				const auto it = paths.find(info.raceID);
				if (it != paths.end()) {
					info.racePath = it->second;
					matched = true;
				}
			}
			if (info.actorID != kInvalidFormID) {
				const auto& paths = context.rules->paths[kRuleType_Actor];
				const auto it = paths.find(info.actorID);
				if (it != paths.end()) {
					info.actorPath = it->second;
					matched = true;
				}
			}

			if (!matched) {
				return _ActorLoad(a_this, a_actor, a_arg3, a_arg4);
			}

			const ActorContext* previous = t_context;
			t_context = &context;
			const auto result = _ActorLoad(a_this, a_actor, a_arg3, a_arg4);
			t_context = previous;
			return result;
		}

		bool HasNifExtension(const char* a_fileName)
		{
			const char* extension = std::strrchr(a_fileName, '.');
			return extension && _stricmp(extension + 1, "nif") == 0;
		}

		std::uint64_t MeshPath_Hook(void* a_arg1, void* a_arg2, const char* a_fileName, const char* a_prefix)
		{
			const ActorContext* context = t_context;
			if (context && a_fileName && a_prefix && _stricmp(a_prefix, "meshes\\") == 0 && HasNifExtension(a_fileName)) {
				const std::string prefix = Utils::ToLower(a_prefix);
				std::string subPath = Utils::ToLower(a_fileName);
				if (Utils::StartsWith(subPath, prefix)) {
					subPath.erase(0, prefix.size());
				}

				const ActorInfo& info = context->info;
				ResolvedPath resolved;
				if (ResolvePath(*context->rules, kRuleType_Actor, info.actorID, info.actorPath, subPath, resolved) ||
					ResolvePath(*context->rules, kRuleType_Race, info.raceID, info.racePath, subPath, resolved)) {
					ModelCache::Insert(resolved.fullPath, info);
					return _MeshPath(a_arg1, a_arg2, resolved.subPath.c_str(), resolved.prefixPath.c_str());
				}
			}

			return _MeshPath(a_arg1, a_arg2, a_fileName, a_prefix);
		}

		// ---- address resolution ----

		bool FindSection(std::uintptr_t a_base, const char* a_name, Section& a_out)
		{
			const auto* dosHeader = reinterpret_cast<const IMAGE_DOS_HEADER*>(a_base);
			const auto* ntHeaders = reinterpret_cast<const IMAGE_NT_HEADERS64*>(a_base + dosHeader->e_lfanew);
			const auto* section = IMAGE_FIRST_SECTION(ntHeaders);
			for (WORD i = 0; i < ntHeaders->FileHeader.NumberOfSections; ++i, ++section) {
				if (std::strncmp(reinterpret_cast<const char*>(section->Name), a_name, IMAGE_SIZEOF_SHORT_NAME) == 0) {
					a_out.begin = a_base + section->VirtualAddress;
					a_out.end = a_out.begin + section->Misc.VirtualSize;
					return true;
				}
			}
			return false;
		}

		bool IsFunctionStart(std::uintptr_t a_address)
		{
			DWORD64 imageBase = 0;
			const auto* entry = RtlLookupFunctionEntry(a_address, &imageBase, nullptr);
			return entry && imageBase + entry->BeginAddress == a_address;
		}

		void LogFunction(const char* a_name, std::uint64_t a_id, std::uintptr_t a_base, std::uintptr_t a_address)
		{
			const auto* bytes = reinterpret_cast<const UInt8*>(a_address);
			_MESSAGE("%s: ID %llu -> Fallout4.exe+%08llX [%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X]",
				a_name, a_id, static_cast<unsigned long long>(a_address - a_base),
				bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7],
				bytes[8], bytes[9], bytes[10], bytes[11], bytes[12], bytes[13], bytes[14], bytes[15]);
		}

		std::uintptr_t ResolveFunction(const char* a_name, std::uint64_t a_id, std::uintptr_t a_base)
		{
			const std::uint64_t offset = AddressLibrary::GetOffset(a_id);
			if (!offset) {
				_ERROR("%s: ID %llu is not in the Address Library", a_name, a_id);
				return 0;
			}

			const std::uintptr_t address = a_base + static_cast<std::uintptr_t>(offset);
			if (!IsFunctionStart(address)) {
				_ERROR("%s: ID %llu (Fallout4.exe+%08llX) is not the start of a function", a_name, a_id, offset);
				return 0;
			}

			LogFunction(a_name, a_id, a_base, address);
			return address;
		}

		// Functions the game calls with the literal a_prefix as their 4th argument
		// (lea r9, [rip+literal] or mov r9, [rip+pointer to literal], followed by a call).
		std::map<std::uintptr_t, UInt32> FindPrefixCallTargets(std::uintptr_t a_base, const char* a_prefix)
		{
			std::map<std::uintptr_t, UInt32> targets;

			Section text, rdata;
			if (!FindSection(a_base, ".text", text) || !FindSection(a_base, ".rdata", rdata)) {
				return targets;
			}

			const std::size_t prefixLength = std::strlen(a_prefix);
			std::set<std::uintptr_t> literals;
			for (std::uintptr_t address = rdata.begin; address + prefixLength < rdata.end; ++address) {
				const auto* str = reinterpret_cast<const char*>(address);
				if (str[prefixLength] == '\0' && _strnicmp(str, a_prefix, prefixLength) == 0) {
					literals.insert(address);
				}
			}
			if (literals.empty()) {
				return targets;
			}

			Section data;
			FindSection(a_base, ".data", data);

			for (std::uintptr_t address = text.begin; address + 7 <= text.end; ++address) {
				const auto* code = reinterpret_cast<const UInt8*>(address);
				if (code[0] != 0x4C || code[2] != 0x0D || (code[1] != 0x8D && code[1] != 0x8B)) {  // lea/mov r9, [rip+disp32]
					continue;
				}

				const std::uintptr_t operand = address + 7 + *reinterpret_cast<const std::int32_t*>(code + 3);
				std::uintptr_t literal = operand;
				if (code[1] == 0x8B) {
					if (!rdata.Contains(operand) && !data.Contains(operand)) {
						continue;
					}
					literal = *reinterpret_cast<const std::uintptr_t*>(operand);
				}
				if (!literals.count(literal)) {
					continue;
				}

				std::uintptr_t ip = address + 7;
				for (int i = 0; i < 16 && text.Contains(ip); ++i) {
					hde64s insn;
					hde64_disasm(reinterpret_cast<const void*>(ip), &insn);
					if (insn.flags & F_ERROR) {
						break;
					}

					const std::uintptr_t next = ip + insn.len;
					if (insn.opcode == 0xE8 || insn.opcode == 0xE9) {  // call/jmp rel32
						const std::uintptr_t callee = next + static_cast<std::int32_t>(insn.imm.imm32);
						if (text.Contains(callee) && IsFunctionStart(callee)) {
							++targets[callee];
						}
						break;
					}
					if (insn.opcode == 0xC3 || insn.opcode == 0xC2 || insn.opcode == 0xEB || insn.opcode == 0xFF) {
						break;
					}
					ip = next;
				}
			}

			return targets;
		}

		std::uintptr_t ResolveMeshPathFunction(std::uintptr_t a_base)
		{
			if (Settings::meshPathID) {
				return ResolveFunction("MeshPath", Settings::meshPathID, a_base);
			}

			const auto targets = FindPrefixCallTargets(a_base, "meshes\\");
			for (const std::uint64_t id : kMeshPathIDs) {
				const std::uint64_t offset = AddressLibrary::GetOffset(id);
				if (offset && targets.count(a_base + static_cast<std::uintptr_t>(offset))) {
					return ResolveFunction("MeshPath", id, a_base);
				}
			}

			_ERROR("MeshPath: none of the expected IDs is called with a \"meshes\\\" prefix");
			for (const auto& [address, calls] : targets) {
				const std::uint64_t offset = address - a_base;
				_ERROR("  candidate Fallout4.exe+%08llX (ID %llu) - %u call(s)", offset, AddressLibrary::GetID(offset), calls);
			}
			_ERROR("  if one of these is the right function, set iMeshPathID in CustomMeshesPathF4.ini");
			return 0;
		}

		template <class T>
		bool CreateHook(const char* a_name, std::uintptr_t a_target, T a_detour, T& a_original)
		{
			const MH_STATUS status = MH_CreateHook(
				reinterpret_cast<LPVOID>(a_target),
				reinterpret_cast<LPVOID>(a_detour),
				reinterpret_cast<LPVOID*>(&a_original));
			if (status != MH_OK) {
				_ERROR("%s: couldn't create hook (%s)", a_name, MH_StatusToString(status));
				return false;
			}
			return true;
		}
	}

	bool Install()
	{
		const std::uintptr_t base = RelocationManager::s_baseAddr;

		const std::uintptr_t actorLoad = ResolveFunction("ActorLoad", Settings::actorLoadID ? Settings::actorLoadID : kActorLoadID, base);
		const std::uintptr_t meshPath = ResolveMeshPathFunction(base);
		if (!actorLoad || !meshPath) {
			return false;
		}

		MH_STATUS status = MH_Initialize();
		if (status != MH_OK && status != MH_ERROR_ALREADY_INITIALIZED) {
			_ERROR("couldn't initialize MinHook (%s)", MH_StatusToString(status));
			return false;
		}

		if (!CreateHook("ActorLoad", actorLoad, &ActorLoad_Hook, _ActorLoad) ||
			!CreateHook("MeshPath", meshPath, &MeshPath_Hook, _MeshPath)) {
			MH_RemoveHook(MH_ALL_HOOKS);
			return false;
		}

		status = MH_EnableHook(MH_ALL_HOOKS);
		if (status != MH_OK) {
			_ERROR("couldn't enable hooks (%s)", MH_StatusToString(status));
			MH_RemoveHook(MH_ALL_HOOKS);
			return false;
		}

		_MESSAGE("hooks installed");
		return true;
	}
}
