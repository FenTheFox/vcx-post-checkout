CMAKE_MINIMUM_REQUIRED (VERSION 3.6 FATAL_ERROR)

PROJECT(post-checkout CXX)

include_directories("${PROJECT_SOURCE_DIR}/fmt/include" "${PROJECT_SOURCE_DIR}/libgit2/include" "${PROJECT_SOURCE_DIR}/pugixml/src" "${PROJECT_SOURCE_DIR}/rapidjson/include" "${PROJECT_SOURCE_DIR}/stduuid/include")

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_CLAR OFF CACHE BOOL "" FORCE)
set(USE_NSEC OFF CACHE BOOL "" FORCE)
set(STATIC_CRT OFF CACHE BOOL "" FORCE)
set(USE_HTTPS OFF CACHE BOOL "" FORCE)

# compile in release with debug info mode by default
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

# Platform specific compilation flags
if(MSVC)
	# /GF - String pooling
	# /MP - Parallel build
	add_compile_definitions(_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING)
	set(CMAKE_C_FLAGS "/GF /MP /nologo ${CMAKE_C_FLAGS}")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /D_DEBUG")
	set(CMAKE_C_FLAGS_RELEASE "/Oy /GL /Gy")
	set(CMAKE_C_FLAGS_RELWITHDEBINFO "/O2 /Oy- /GL /Gy")
	set(CMAKE_C_FLAGS_MINSIZEREL "/Oy /GL /Gy")

	# /DEBUG - Create a PDB
	# /LTCG - Link time code generation (whole program optimization)
	# /OPT:REF /OPT:ICF - Fold out duplicate code at link step
	# /INCREMENTAL:NO - Required to use /LTCG
	# /DEBUGTYPE:cv,fixup - Additional data embedded in the PDB (requires /INCREMENTAL:NO, so not on for Debug)
	set(CMAKE_EXE_LINKER_FLAGS_DEBUG "/DEBUG")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/RELEASE /LTCG /OPT:REF /OPT:ICF /INCREMENTAL:NO")
	set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "/DEBUG /RELEASE /LTCG /OPT:REF /OPT:ICF /INCREMENTAL:NO /DEBUGTYPE:cv,fixup")
	set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "/RELEASE /LTCG /OPT:REF /OPT:ICF /INCREMENTAL:NO")
	set(CMAKE_FIND_LIBRARY_SUFFIXES .lib)
else()
	set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
endif()

add_subdirectory(fmt)
add_subdirectory(libgit2)
add_subdirectory(pugixml)

add_executable(post-checkout FileTree.cpp GitRepo.cpp post-checkout.cpp Sln.cpp Vcxproj.cpp)
target_compile_features(post-checkout PRIVATE cxx_std_17)
target_link_libraries(post-checkout fmt pugixml git2)

if(MSVC)
else()
	target_link_libraries(post-checkout stdc++fs)
endif()