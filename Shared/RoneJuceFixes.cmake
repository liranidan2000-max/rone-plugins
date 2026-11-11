# Fixes to the JUCE a product fetches, applied to the fetched sources at configure time.
# Shared/RoneBuildHooks.cmake calls rone_apply_juce_fixes() at the end of the configure of
# every macOS release build, so no plugin's own CMakeLists changes. Each fix is idempotent
# (a marker line), does nothing on a JUCE that already carries it, and fails the configure
# when JUCE's code no longer matches - a JUCE bump can never silently drop a fix. (Same way
# as RoneStucker/cmake/JuceCoreAudioCombinerFix.cmake.)
#
# ---------------------------------------------------------------------------------------
# 1. macOS: a WebView plugin no longer takes the DAW down when its window closes while the
#    page is still asking for a file (support #133: RONE Clipper 1.0.12 in FL Studio,
#    macOS 26, EXC_CRASH SIGABRT; resolved to std::__throw_bad_function_call inside
#    JUCE's webView:startURLSchemeTask:).
#
#    JUCE 8.0.4: the WKWebView's delegate holds a raw pointer to the DelegateConnector that
#    lives in WKWebViewImpl. The delegate is also the page's script-message handler and its
#    juce:// scheme handler, both retained by the WKWebViewConfiguration, so it outlives
#    the impl - and ~WKWebViewImpl never clears the pointer. A resource request (or a
#    message) that WebKit delivers after the editor died calls through the dead connector:
#    a destroyed std::function, std::bad_function_call, nothing catches it, abort() in the
#    host. Every RONE plugin on JUCE 8.0.4 has it; the Center, the Analyzer, the Flanger
#    and VST MANAGER are on 8.0.12, where JUCE fixed it the same way: the destructor
#    clears the pointer and the handlers check it.
# ---------------------------------------------------------------------------------------

function(rone_fix_juce_webview_teardown juce_src_dir)
    set(_file "${juce_src_dir}/modules/juce_gui_extra/native/juce_WebBrowserComponent_mac.mm")
    if(NOT EXISTS "${_file}")
        message(STATUS "RoneJuceFixes: no ${_file} - nothing to fix")
        return()
    endif()

    file(READ "${_file}" _src)

    set(_marker "RONE: the delegate lets go of the connector")
    string(FIND "${_src}" "${_marker}" _done)
    if(NOT _done EQUAL -1)
        return()
    endif()

    # JUCE's own fix (8.0.12 has it)
    string(FIND "${_src}" "WebViewDelegateClass::setConnector (webViewDelegate.get(), nullptr)" _upstream)
    if(NOT _upstream EQUAL -1)
        message(STATUS "RoneJuceFixes: this JUCE already clears the WebView delegate's connector")
        return()
    endif()

    set(_old_destructor [=[
    ~WKWebViewImpl() override
    {
        setView (nil);
]=])
    set(_new_destructor [=[
    ~WKWebViewImpl() override
    {
        // RONE: the delegate lets go of the connector (support #133). WebKit keeps the
        // delegate alive as the page's message and juce:// handler, and it called
        // through this object after it was gone. As in JUCE 8.0.12.
        WebViewDelegateClass::setConnector (webViewDelegate.get(), nullptr);

        setView (nil);
]=])

    set(_old_message [=[
                   [] (id self, SEL, id, id message)
                   {
                       const auto object = fromObject ([message body]);
]=])
    set(_new_message [=[
                   [] (id self, SEL, id, id message)
                   {
                       if (getConnector (self) == nullptr)   // RONE: the editor is gone (support #133)
                           return;

                       const auto object = fromObject ([message body]);
]=])

    set(_old_resource [=[
                   [] (id self, SEL, id, id urlSchemeTask)
                   {
                       const auto request = [urlSchemeTask request];
]=])
    set(_new_resource [=[
                   [] (id self, SEL, id, id urlSchemeTask)
                   {
                       if (getConnector (self) == nullptr)   // RONE: the editor is gone (support #133)
                       {
                           [urlSchemeTask didFailWithError: [NSError errorWithDomain: NSURLErrorDomain
                                                                                code: NSURLErrorCancelled
                                                                            userInfo: nil]];
                           return;
                       }

                       const auto request = [urlSchemeTask request];
]=])

    # JUCE keeps this file with CRLF; compare and write with LF only
    string(REPLACE "\r\n" "\n" _src "${_src}")

    foreach(_part destructor message resource)
        string(REPLACE "\r\n" "\n" _old_${_part} "${_old_${_part}}")
        string(REPLACE "\r\n" "\n" _new_${_part} "${_new_${_part}}")

        string(FIND "${_src}" "${_old_${_part}}" _at)
        string(FIND "${_src}" "${_old_${_part}}" _last REVERSE)
        if(_at EQUAL -1 OR NOT _at EQUAL _last)
            message(FATAL_ERROR "RoneJuceFixes: JUCE's WKWebView code changed (the ${_part} part matches "
                                "no place, or more than one) - re-check the fix for support #133 in ${_file}")
        endif()
        string(REPLACE "${_old_${_part}}" "${_new_${_part}}" _src "${_src}")
    endforeach()

    file(WRITE "${_file}" "${_src}")
    message(STATUS "RoneJuceFixes: patched ${_file} (WebView teardown, support #133)")
endfunction()

# Every fix, on whichever JUCE this project brought in (FetchContent or its own folder).
function(rone_apply_juce_fixes)
    set(_dir "")
    if(DEFINED JUCE_SOURCE_DIR AND EXISTS "${JUCE_SOURCE_DIR}/modules")          # set by JUCE's own project()
        set(_dir "${JUCE_SOURCE_DIR}")
    elseif(DEFINED juce_SOURCE_DIR AND EXISTS "${juce_SOURCE_DIR}/modules")      # FetchContent_MakeAvailable(JUCE)
        set(_dir "${juce_SOURCE_DIR}")
    endif()
    if(_dir STREQUAL "")
        message(STATUS "RoneJuceFixes: this project has no JUCE - nothing to fix")
        return()
    endif()

    rone_fix_juce_webview_teardown("${_dir}")
endfunction()
