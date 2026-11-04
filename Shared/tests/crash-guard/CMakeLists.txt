# Proves RoneDawCrashGuard.cpp does what its header comment promises. Not part of any release.
#   cmake -S Shared/tests/crash-guard -B build -A x64
#   cmake --build build --config Release
#   build/Release/guard_proof.exe
cmake_minimum_required(VERSION 3.22)
project(CrashGuardProof LANGUAGES CXX ASM_MASM)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/Z7>)
add_link_options(/DEBUG:FULL)

# stands in for a plugin: the same file and the same link a VST3 gets, loaded by a host that is not ours
add_library(guard_plugin MODULE fake_plugin.cpp fake_plugin_asm.asm ../../RoneDawCrashGuard.cpp)
set_property(SOURCE ../../RoneDawCrashGuard.cpp PROPERTY COMPILE_DEFINITIONS "JucePlugin_Name=\"RONE GuardTest\"" "JucePlugin_VersionString=\"9.9.9\"")
target_link_options(guard_plugin PRIVATE /ENTRY:roneGuardEntry)
set_target_properties(guard_plugin PROPERTIES OUTPUT_NAME "RONE GuardTest" SUFFIX ".vst3" PREFIX "" INTERPROCEDURAL_OPTIMIZATION ON)

add_executable(guard_proof guard_proof.cpp guard_proof_asm.asm)
add_dependencies(guard_proof guard_plugin)
target_compile_options(guard_proof PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/EHa>)
