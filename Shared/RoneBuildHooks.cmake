# What every release build gets without touching any plugin's own CMakeLists.
# CI passes it to each configure:
#   -DCMAKE_PROJECT_INCLUDE=<repo>/Shared/RoneBuildHooks.cmake
# so a local build is untouched unless it asks for the same.
#
#   1. Symbols. A shipped Windows binary gets a matching PDB, so a crash report's
#      module+offset can be turned back into a file and line. /Z7 keeps the debug
#      info in the objects, which is what Ninja needs in parallel; /OPT:REF,ICF are
#      what /DEBUG would otherwise switch off, so the code stays as it was.
#   2. RoneDawCrashGuard.cpp in each plugin's VST3 target (not the Standalone), with
#      the DLL's entry point moved to it. The file's header comment says why.
#   3. The C++ runtime inside every binary (/MT), not msvcp140.dll from the
#      customer's System32. A binary built with a newer toolset than the
#      customer's runtime dies at its first std::mutex lock with no window and no
#      error: Viki, 2026-11-12, Windows with msvcp140 14.36 - the Center and
#      VST MANAGER both crashed in MSVCP140.dll+0x12f58 the moment they started,
#      and every plugin would have done the same inside the DAW.
#      WebView2LoaderStatic.lib carries no runtime, so it links either way.
#   4. macOS: the fixes to the JUCE the product fetched (Shared/RoneJuceFixes.cmake),
#      applied when the configure ends - by then the project has brought its JUCE in.
#
# 1 - 3 are Windows only, 4 is macOS only.

if(APPLE AND NOT RONE_JUCE_FIXES_HOOKED)
    set(RONE_JUCE_FIXES_HOOKED TRUE)
    include(${CMAKE_CURRENT_LIST_DIR}/RoneJuceFixes.cmake)
    cmake_language(DEFER DIRECTORY ${CMAKE_SOURCE_DIR} CALL rone_apply_juce_fixes)
endif()

if(RONE_BUILD_HOOKS_DONE OR NOT MSVC)
    return()
endif()
set(RONE_BUILD_HOOKS_DONE TRUE)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:/Z7>)
add_link_options(/DEBUG:FULL /OPT:REF /OPT:ICF /PDBALTPATH:%_PDB%)

set(RONE_SHARED_DIR ${CMAKE_CURRENT_LIST_DIR})

function(rone_attach_guard dir)
    get_directory_property(targets DIRECTORY ${dir} BUILDSYSTEM_TARGETS)
    foreach(t IN LISTS targets)
        get_target_property(kind ${t} TYPE)
        if(t MATCHES "_VST3$" AND kind STREQUAL "MODULE_LIBRARY" AND CMAKE_CXX_COMPILER_ARCHITECTURE_ID STREQUAL "x64")
            target_sources(${t} PRIVATE ${RONE_SHARED_DIR}/RoneDawCrashGuard.cpp)
            target_link_options(${t} PRIVATE /ENTRY:roneGuardEntry)
            message(STATUS "RONE crash guard: ${t}")
        endif()
    endforeach()
    get_directory_property(subdirs DIRECTORY ${dir} SUBDIRECTORIES)
    foreach(s IN LISTS subdirs)
        rone_attach_guard(${s})
    endforeach()
endfunction()

cmake_language(DEFER DIRECTORY ${CMAKE_SOURCE_DIR} CALL rone_attach_guard ${CMAKE_SOURCE_DIR})
