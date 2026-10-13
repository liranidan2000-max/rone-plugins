# licence_sim - offline checks of the Shared licence chain (see licence_sim.cpp).
#   cmake -S Shared/tests/licence_sim -B <build> -G "Visual Studio 17 2022" -A x64 -DJUCE_DIR=<JUCE 8.0.4 source>
#   cmake --build <build> --config Release
#   <build>/licence_sim_artefacts/Release/licence_sim.exe   (exit code 0 = all pass)
#   Run the Debug config too (--config Debug, then <build>/licence_sim_artefacts/Debug/licence_sim.exe):
#   the debug CRT fills freed memory with 0xDD, which exposes a use-after-free that Release hides
#   (R014: localMachineIds() once hashed a freed temporary and still passed in Release).
#   Stricter still: an AddressSanitizer build (clang -fsanitize=address; MSVC /fsanitize=address in Release - it conflicts with Debug's /RTC1).
cmake_minimum_required(VERSION 3.22)
project(LicenceSim VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(JUCE_DIR "" CACHE PATH "JUCE 8.0.4 source (a plugin build's _deps/juce-src)")
add_subdirectory("${JUCE_DIR}" juce)
juce_add_console_app(licence_sim PRODUCT_NAME "licence_sim")
target_sources(licence_sim PRIVATE licence_sim.cpp)
target_compile_definitions(licence_sim PRIVATE JUCE_USE_CURL=0 JUCE_WEB_BROWSER=0)
target_link_libraries(licence_sim PRIVATE
    juce::juce_core juce::juce_cryptography
    juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
if(MSVC)
    target_compile_options(licence_sim PRIVATE /utf-8)
endif()
