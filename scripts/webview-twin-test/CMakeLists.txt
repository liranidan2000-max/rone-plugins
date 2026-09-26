cmake_minimum_required(VERSION 3.22)
project(TwinEditorTest VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#   cmake -S . -B <build> -DJUCE_DIR=<JUCE 8.0.4 tree>   then   cmake --build <build> --config Release
add_subdirectory(${JUCE_DIR} ${CMAKE_BINARY_DIR}/juce)

juce_add_console_app(TwinEditorHost PRODUCT_NAME "TwinEditorHost")
juce_generate_juce_header(TwinEditorHost)
target_sources(TwinEditorHost PRIVATE twin_host.cpp)
target_compile_definitions(TwinEditorHost PRIVATE
    JUCE_PLUGINHOST_VST3=1
    JUCE_MODAL_LOOPS_PERMITTED=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)
target_link_libraries(TwinEditorHost PRIVATE
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_events
    juce::juce_core
    juce::juce_data_structures
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)

add_executable(HiddenDesktopRun hidden_run.cpp)
target_compile_definitions(HiddenDesktopRun PRIVATE UNICODE _UNICODE)
target_link_libraries(HiddenDesktopRun PRIVATE user32)
