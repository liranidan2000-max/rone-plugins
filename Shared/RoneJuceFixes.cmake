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

# ---------------------------------------------------------------------------------------
# 2. macOS standalone: an audio device change (headphones in/out, AirPods) no longer crashes
#    the app (Stucker #142, Sidechain #203, Clipper #207: three reports of one JUCE race).
#    CoreAudio's HAL thread runs AudioIODeviceCombiner::restartAsync, which closed the devices
#    right there while the message thread was inside start() / close(): with #207 it called
#    audioDeviceStopped() on start()'s own stack object. Now it only starts a timer; the close
#    and the reopen run on the message thread. The same text as RoneStucker / RoneSidechain /
#    RoneClipper cmake/JuceCoreAudioCombinerFix.cmake (the marker keeps it from running twice).
#    JUCE 8.0.12 has the same code; JUCE 9 rewrote it.
# ---------------------------------------------------------------------------------------

function(rone_fix_juce_coreaudio_combiner juce_src_dir)
    set(_file "${juce_src_dir}/modules/juce_audio_devices/native/juce_CoreAudio_mac.cpp")
    if(NOT EXISTS "${_file}")
        message(STATUS "RoneJuceFixes: no ${_file} - nothing to fix")
        return()
    endif()

    file(READ "${_file}" _src)

    set(_marker "RONE: combiner restart runs on the message thread")
    string(FIND "${_src}" "${_marker}" _done)
    if(NOT _done EQUAL -1)
        return()
    endif()

    set(_old_restart [=[
    void restartAsync() override
    {
        {
            const ScopedLock sl (closeLock);

            if (active)
            {
                if (callback != nullptr)
                    previousCallback = callback;

                close();
            }
        }

        startTimer (100);
    }
]=])
    set(_new_restart [=[
    // RONE: combiner restart runs on the message thread (crash report #142).
    // CoreAudio calls this on its HAL thread; closing the devices here raced the
    // message thread closing them for a device-list change.
    void restartAsync() override
    {
        startTimer (100);
    }
]=])

    set(_old_timer [=[
    void timerCallback() override
    {
        stopTimer();

        restart (previousCallback);
    }
]=])
    set(_new_timer [=[
    void timerCallback() override
    {
        stopTimer();

        {
            const ScopedLock sl (closeLock);

            if (active)
            {
                if (callback != nullptr)
                    previousCallback = callback;

                close();
            }
        }

        restart (previousCallback);
    }
]=])

    # Git may check out JUCE or this file with CRLF - compare with LF only
    string(REPLACE "\r\n" "\n" _src "${_src}")

    foreach(_part restart timer)
        string(REPLACE "\r\n" "\n" _old_${_part} "${_old_${_part}}")
        string(REPLACE "\r\n" "\n" _new_${_part} "${_new_${_part}}")
        string(FIND "${_src}" "${_old_${_part}}" _at)
        if(_at EQUAL -1)
            message(FATAL_ERROR "RoneJuceFixes: JUCE's AudioIODeviceCombiner changed "
                                "(no match for ${_part}) - re-check the fix for crash report #142 in ${_file}")
        endif()
        string(REPLACE "${_old_${_part}}" "${_new_${_part}}" _src "${_src}")
    endforeach()

    file(WRITE "${_file}" "${_src}")
    message(STATUS "RoneJuceFixes: patched ${_file} (CoreAudio combiner, support #207)")
endfunction()

# ---------------------------------------------------------------------------------------
# 3. macOS AU in Ableton Live: a key pressed while the plugin window is in front no longer
#    takes Live down (support #210: RONE Sidechain 1.2.5 AU, Live 12.4.16, macOS 26.5.2,
#    EXC_BREAKPOINT in libobjc; symbolized from build 451's component, arm64:
#    JuceAU::EditorCompHolder::keyPressed + 0xc8 <- ComponentPeer::handleKeyPress
#    <- NSViewComponentPeer::redirectKeyDown).
#
#    For Live only, JUCE passes every key on to Live's own view (so the space bar still
#    plays). JUCE 8.0.4 reads Live's window once, BEFORE it hands Live the key, and gives
#    the focus back to that same window afterwards. When the key makes Live close or
#    rebuild the plugin window, that window is gone by then: a message to a freed object,
#    which on Apple silicon traps in libobjc. JUCE 8.0.12 reads the window again after
#    the key, and gives the focus back only when there still is one - the same text here.
# ---------------------------------------------------------------------------------------

function(rone_fix_juce_au_live_keys juce_src_dir)
    set(_file "${juce_src_dir}/modules/juce_audio_plugin_client/juce_audio_plugin_client_AU_1.mm")
    if(NOT EXISTS "${_file}")
        message(STATUS "RoneJuceFixes: no ${_file} - nothing to fix")
        return()
    endif()

    file(READ "${_file}" _src)

    set(_marker "RONE: Live may close the window on that key")
    string(FIND "${_src}" "${_marker}" _done)
    if(NOT _done EQUAL -1)
        return()
    endif()

    # JUCE's own fix (8.0.12 has it)
    string(FIND "${_src}" "if ((hostView = [view superview]))" _upstream)
    if(NOT _upstream EQUAL -1)
        message(STATUS "RoneJuceFixes: this JUCE already re-reads Live's window after a key")
        return()
    endif()

    set(_old_keys [=[
                        auto* view = (NSView*) getWindowHandle();
                        auto* hostView = [view superview];
                        auto* hostWindow = [hostView window];

                        [hostWindow makeFirstResponder: hostView];
                        [hostView keyDown: currentEvent];
                        [hostWindow makeFirstResponder: view];
]=])
    set(_new_keys [=[
                        auto* view = (NSView*) getWindowHandle();
                        auto* hostView = [view superview];

                        [[hostView window] makeFirstResponder: hostView];
                        [hostView keyDown: currentEvent];

                        // RONE: Live may close the window on that key (support #210). Read it
                        // again, as JUCE 8.0.12 does: the one read before keyDown can be freed.
                        if ((hostView = [view superview]))
                            if (auto* hostWindow = [hostView window])
                                [hostWindow makeFirstResponder: view];
]=])

    string(REPLACE "\r\n" "\n" _src "${_src}")
    string(REPLACE "\r\n" "\n" _old_keys "${_old_keys}")
    string(REPLACE "\r\n" "\n" _new_keys "${_new_keys}")
    string(FIND "${_src}" "${_old_keys}" _at)
    if(_at EQUAL -1)
        message(FATAL_ERROR "RoneJuceFixes: JUCE's AU keyPressed for Live changed "
                            "- re-check the fix for support #210 in ${_file}")
    endif()
    string(REPLACE "${_old_keys}" "${_new_keys}" _src "${_src}")

    file(WRITE "${_file}" "${_src}")
    message(STATUS "RoneJuceFixes: patched ${_file} (AU keys in Live, support #210)")
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
    rone_fix_juce_coreaudio_combiner("${_dir}")
    rone_fix_juce_au_live_keys("${_dir}")
endfunction()
