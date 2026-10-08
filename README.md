# CustomMeshesPathF4

F4SE plugin that makes specific actors or races load their `.nif` meshes from a custom folder under `Data\Meshes\` instead of the default path, for example child races reading their body from `Data\Meshes\CWC\Meshes\` instead of `Data\Meshes\`. It also points the model's body morph data (`BODYTRI`) at the custom folder when a matching `.tri` file exists there.

This is the **Fallout 4 Next-Gen port**: built against **F4SE 0.7.2** for runtime **1.10.984**. The original build for 1.10.163 (F4SE 0.6.23) is kept in `legacy/1.10.163/`.

## Requirements

- Fallout 4 **1.10.984**
- F4SE **0.7.2** (the 1.10.984 build)
- **Address Library for F4SE Plugins** for 1.10.984 (`Data\F4SE\Plugins\version-1-10-984-0.bin`)

## Installation

Copy into `Data\F4SE\Plugins\`:

- `CustomMeshesPathF4.dll`
- `CustomMeshesPathF4.ini`
- `CustomMeshesPathF4_Rules.txt`

The log is written to `Documents\My Games\Fallout4\F4SE\CustomMeshesPathF4.log`.

## Rules

`CustomMeshesPathF4_Rules.txt` is read when a save is loaded or a new game starts, and re-read whenever the file has changed since the last load. `#` starts a comment.

```
# RuleType | Plugin | FormID : MeshesPath
Race | CWC-Main.esp | F99 : CWC\Meshes\

# RuleType | Plugin | FormID : MeshPath : CustomPath
Race | CWC-Main.esp | F99 : Actors\Character\CharacterAssets\FemaleBody.nif : Custom\FemaleBody.nif
```

- `RuleType` is `Actor` (an NPC base form; leveled actors use the base they were generated from) or `Race`.
- `FormID` is the hex ID inside the plugin (no load-order prefix). ESL plugins are supported.
- **MeshesPath** rules: while that actor's 3D is built, every requested `meshes\<file>.nif` is loaded from `meshes\<MeshesPath>\<file>.nif` when that loose file exists.
- **MeshPath : CustomPath** rules replace one mesh with another. They only apply to forms that also have a MeshesPath rule; `CustomPath` is looked up inside the MeshesPath folder first, then directly under `Data\Meshes\`.
- Actor rules are tried before race rules.
- Only loose files are considered; meshes inside `.ba2` archives are not detected as replacements.

## Settings

`CustomMeshesPathF4.ini`:

| Key | Description |
| --- | --- |
| `[Debug] bDebug` | `1` logs every path decision. |
| `[Advanced] iActorLoadID`, `iMeshPathID` | Address Library ID overrides for the two hooked functions. Leave at `0`. |

## How the NG port finds the game functions

The plugin hooks two game functions that F4SE does not expose. They were located in the original 1.10.163 DLL and mapped to their Next-Gen Address Library IDs:

| Hook | 1.10.163 | 1.10.163 ID | 1.10.984 ID |
| --- | --- | --- | --- |
| Actor 3D build (TESNPC code, `(?, Actor*)`) | `0x005B93F0` | 203719 | 2207456 |
| File open with folder prefix (BSResource, `(?, ?, name, prefix)`) | `0x01B7CD40` | 1053009 | 2269474 |

These IDs are derived from the ordering of known functions around them, not read from the game executable, so the plugin verifies them at startup before hooking anything:

- each address must be the start of a function in `Fallout4.exe`;
- the file-open function must be one the game calls with a `"meshes\"` prefix argument. Nearby IDs are tried if 2269474 doesn't pass.

If a check fails, the plugin logs the reason (with any candidate functions it found) and disables itself instead of patching the wrong code. A verified ID can then be set in the `[Advanced]` section.

Everything else (forms, data handler, extra data, `BSFixedString`, the model processor chain) uses F4SE 0.7.2's own 1.10.984 addresses. Hooks are installed with [MinHook](https://github.com/TsudaKageyu/minhook), which relocates the Next-Gen function prologues instead of assuming the old ones.

### Changes from the 1.10.163 version

- F4SE 0.7.x plugin format (`F4SEPlugin_Version` instead of `F4SEPlugin_Query`).
- Hook addresses come from the Address Library and are verified (see above).
- A `#` comment after a rule no longer breaks the rule.
- An invalid form ID in the rules file is logged instead of crashing the game.
- Nested actor loads on the same thread keep the outer actor's rules.
- Reloading the rules can't race with meshes being loaded on other threads.
- Replacing the `BODYTRI` string releases the old one.

## Building

Requires Visual Studio 2022 (MSVC, x64) and CMake 3.21+. F4SE 0.7.2, [ianpatt/common](https://github.com/ianpatt/common) and MinHook 1.3.4 are downloaded automatically; point `F4SE_SOURCE_DIR`, `COMMON_SOURCE_DIR` or `MINHOOK_SOURCE_DIR` at local copies to use those instead.

```
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Set `-DCOPY_TO_GAME=ON` with the `Fallout4Path` environment variable set to copy the DLL into `Data\F4SE\Plugins` after each build.

## Third-party code

The DLL contains [MinHook](https://github.com/TsudaKageyu/minhook) (BSD 2-Clause, see `licenses/MinHook-LICENSE.txt`) and parts of [F4SE](https://github.com/ianpatt/f4se) and [ianpatt/common](https://github.com/ianpatt/common).
