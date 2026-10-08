cmake_minimum_required(VERSION 3.21)

project(
	CustomMeshesPathF4
	VERSION 2.0.0
	LANGUAGES C CXX
)

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
	message(FATAL_ERROR "In-source builds are not allowed. Configure into a separate build directory.")
endif()

# ---- Options ----

set(F4SE_SOURCE_DIR "" CACHE PATH "Path to an F4SE 0.7.2 source tree (the folder containing f4se/, f4se_common/ and xbyak/). Downloaded when empty.")
set(COMMON_SOURCE_DIR "" CACHE PATH "Path to ianpatt/common. Downloaded when empty.")
set(MINHOOK_SOURCE_DIR "" CACHE PATH "Path to TsudaKageyu/minhook. Downloaded when empty.")
option(COPY_TO_GAME "Copy the built DLL to $ENV{Fallout4Path}/Data/F4SE/Plugins after building" OFF)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "")

# ---- Dependencies ----

include(FetchContent)

if(COMMON_SOURCE_DIR)
	add_subdirectory("${COMMON_SOURCE_DIR}" common)
else()
	FetchContent_Declare(
		common
		GIT_REPOSITORY https://github.com/ianpatt/common.git
		GIT_TAG 64e233c096735551f6ac9a773726a8a3960e46cd
	)
	FetchContent_MakeAvailable(common)
endif()

if(F4SE_SOURCE_DIR)
	add_subdirectory("${F4SE_SOURCE_DIR}/f4se_common" f4se_common)
else()
	FetchContent_Declare(
		f4se
		GIT_REPOSITORY https://github.com/ianpatt/f4se.git
		GIT_TAG v0.7.2
		SOURCE_SUBDIR f4se_common
	)
	FetchContent_MakeAvailable(f4se)
	set(F4SE_SOURCE_DIR "${f4se_SOURCE_DIR}")
endif()

# only the parts of f4se the plugin uses (the full library also carries f4se's own hooks and papyrus bindings)
add_library(
	f4se_runtime
	STATIC
		"${F4SE_SOURCE_DIR}/f4se/BSModelDB.cpp"
		"${F4SE_SOURCE_DIR}/f4se/GameAPI.cpp"
		"${F4SE_SOURCE_DIR}/f4se/GameData.cpp"
		"${F4SE_SOURCE_DIR}/f4se/GameExtraData.cpp"
		"${F4SE_SOURCE_DIR}/f4se/GameForms.cpp"
		"${F4SE_SOURCE_DIR}/f4se/GameRTTI.cpp"
		"${F4SE_SOURCE_DIR}/f4se/GameTypes.cpp"
		"${F4SE_SOURCE_DIR}/f4se/GameUtilities.cpp"
		"${F4SE_SOURCE_DIR}/f4se/NiObjects.cpp"
)

target_compile_definitions(
	f4se_runtime
	PUBLIC
		RUNTIME
		RUNTIME_VERSION=0x010A3D80	# 1.10.984
)

target_include_directories(f4se_runtime PUBLIC "${F4SE_SOURCE_DIR}")
target_link_libraries(f4se_runtime PUBLIC f4se::f4se_common xse::common)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
if(MINHOOK_SOURCE_DIR)
	add_subdirectory("${MINHOOK_SOURCE_DIR}" minhook)
	set(minhook_SOURCE_DIR "${MINHOOK_SOURCE_DIR}")
else()
	FetchContent_Declare(
		minhook
		GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
		GIT_TAG v1.3.4
	)
	FetchContent_MakeAvailable(minhook)
endif()

# ---- Plugin ----

set(headers
	src/AddressLibrary.h
	src/Hooks.h
	src/ModelCache.h
	src/ModelProcessor.h
	src/PathResolver.h
	src/Rules.h
	src/Settings.h
	src/Utils.h
)

set(sources
	src/AddressLibrary.cpp
	src/Hooks.cpp
	src/main.cpp
	src/ModelCache.cpp
	src/ModelProcessor.cpp
	src/PathResolver.cpp
	src/Rules.cpp
	src/Settings.cpp
	src/Utils.cpp
)

add_library(${PROJECT_NAME} SHARED ${headers} ${sources})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

target_compile_definitions(
	${PROJECT_NAME}
	PRIVATE
		PLUGIN_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
		PLUGIN_VERSION_MINOR=${PROJECT_VERSION_MINOR}
		PLUGIN_VERSION_PATCH=${PROJECT_VERSION_PATCH}
		_CRT_SECURE_NO_WARNINGS
)

target_include_directories(
	${PROJECT_NAME}
	PRIVATE
		src
		"${minhook_SOURCE_DIR}/src/hde"
)

target_link_libraries(
	${PROJECT_NAME}
	PRIVATE
		f4se_runtime
		minhook
)

if(MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE /W3 /MP)
	target_link_options(${PROJECT_NAME} PRIVATE "$<$<CONFIG:Release,RelWithDebInfo>:/OPT:REF;/OPT:ICF>")
endif()

if(COPY_TO_GAME)
	add_custom_command(
		TARGET ${PROJECT_NAME}
		POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory "$ENV{Fallout4Path}/Data/F4SE/Plugins"
		COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> "$ENV{Fallout4Path}/Data/F4SE/Plugins"
	)
endif()
