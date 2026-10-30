#pragma once

// ============================================================================
// WinMidiLoopback - a MIDI port pair from Windows itself (Windows MIDI Services).
//
// Windows 11 with the new MIDI stack (service "midisrv") has a loopback transport in the box: an endpoint pair
// A/B where what is sent to A comes out of B and the other way round, seen by every MIDI 1.0 app (FL, Live,
// REAPER) as ordinary ports named exactly after the endpoints. No driver, no admin rights. loopMIDI's driver
// (teVirtualMIDI) is no use on that stack: a port it makes after midisrv started passes no data at all until
// the service restarts (Microsoft's known issue, microsoft/MIDI #1275; seen on Liran's PC 2026-10-30).
//
// Created the way Microsoft's own SDK does it (microsoft/MIDI, MidiServiceTransportPluginConfigManager.cpp),
// through the in-box COM class Midi2MidiSrvTransport: IMidiTransport::Activate(IMidiTransportConfigurationManager)
// -> Initialize(loopback transport id) -> UpdateConfiguration(json), with "create" or "remove". The pair is
// transient: it lasts until it is removed or the MIDI service restarts. Proven first in RONE VST MANAGER
// (Source/WinMidiLoopback.h, commit 4d6d9e5).
//
// That interface is not a published app contract: everything that touches it lives here, and any failure just
// means "no pair" (the caller falls back to loopMIDI's driver).
// ============================================================================
#if JUCE_WINDOWS

#include <JuceHeader.h>
#include <windows.h>
#include <objbase.h>

namespace rone::winmidi
{
    // from microsoft/MIDI src/in-box/idl/WindowsMidiServices.idl (MIT)
    MIDL_INTERFACE ("EA264200-3328-49E5-8815-73649A8748BE")
    IMidiTransport : public IUnknown
    {
        virtual HRESULT STDMETHODCALLTYPE Activate (REFIID iid, void** activated) = 0;
    };

    MIDL_INTERFACE ("f19dd642-1809-4497-9eee-f230b11bd6fb")
    IMidiTransportConfigurationManager : public IUnknown
    {
        virtual HRESULT STDMETHODCALLTYPE Initialize (GUID transportId, IUnknown* deviceManager, IUnknown* serviceConfigManager) = 0;
        virtual HRESULT STDMETHODCALLTYPE UpdateConfiguration (LPCWSTR configurationJsonSection, LPWSTR* responseJson) = 0;
        virtual HRESULT STDMETHODCALLTYPE Shutdown() = 0;
    };

    // CLSID of the in-box Midi2MidiSrvTransport (C:\Windows\System32\Midi2.MidiSrvTransport.dll)
    static const CLSID kMidiSrvTransport = { 0x2BA15E4E, 0x5417, 0x4A66, { 0x85, 0xB8, 0x2B, 0x22, 0x60, 0xEF, 0xBC, 0x84 } };
    // the MIDI 2.0 loopback transport (Midi2LoopbackMidiTransport)
    static const GUID kLoopbackTransport = { 0x942BF02D, 0x93C0, 0x4EA8, { 0xB0, 0x3E, 0xD5, 0x11, 0x56, 0xCA, 0x75, 0xE1 } };
    static constexpr const char* kLoopbackTransportId = "{942BF02D-93C0-4EA8-B03E-D51156CA75E1}";

    /** True while the new MIDI stack's service is installed. */
    inline bool serviceInstalled()
    {
        bool there = false;
        if (auto scm = OpenSCManagerW (nullptr, nullptr, SC_MANAGER_CONNECT))
        {
            if (auto svc = OpenServiceW (scm, L"midisrv", SERVICE_QUERY_STATUS)) { there = true; CloseServiceHandle (svc); }
            CloseServiceHandle (scm);
        }
        return there;
    }

    /** The MIDI service's process id, 0 when it is not running. A new id = it restarted and every pair is gone. */
    inline DWORD serviceProcessId()
    {
        DWORD pid = 0;
        if (auto scm = OpenSCManagerW (nullptr, nullptr, SC_MANAGER_CONNECT))
        {
            if (auto svc = OpenServiceW (scm, L"midisrv", SERVICE_QUERY_STATUS))
            {
                SERVICE_STATUS_PROCESS s {};
                DWORD needed = 0;
                if (QueryServiceStatusEx (svc, SC_STATUS_PROCESS_INFO, (LPBYTE) &s, sizeof (s), &needed) && s.dwCurrentState == SERVICE_RUNNING)
                    pid = s.dwProcessId;
                CloseServiceHandle (svc);
            }
            CloseServiceHandle (scm);
        }
        return pid;
    }

    /** Sends one loopback-transport section ({"create": ...} or {"remove": [...]}) to the MIDI service.
        Returns the service's answer (or why there was none) in `response`. */
    inline bool configure (const juce::var& transportSection, juce::String& response)
    {
        const bool comHere = SUCCEEDED (CoInitializeEx (nullptr, COINIT_MULTITHREADED));
        bool ok = false;
        IMidiTransport* transport = nullptr;
        if (SUCCEEDED (CoCreateInstance (kMidiSrvTransport, nullptr, CLSCTX_ALL, __uuidof (IMidiTransport), (void**) &transport)) && transport != nullptr)
        {
            IMidiTransportConfigurationManager* config = nullptr;
            if (SUCCEEDED (transport->Activate (__uuidof (IMidiTransportConfigurationManager), (void**) &config)) && config != nullptr)
            {
                if (SUCCEEDED (config->Initialize (kLoopbackTransport, nullptr, nullptr)))
                {
                    juce::DynamicObject::Ptr plugins = new juce::DynamicObject();
                    plugins->setProperty (kLoopbackTransportId, transportSection);
                    juce::DynamicObject::Ptr outer = new juce::DynamicObject();
                    outer->setProperty ("endpointTransportPluginSettings", juce::var (plugins.get()));

                    LPWSTR reply = nullptr;
                    const auto json = juce::JSON::toString (juce::var (outer.get()), true);
                    const HRESULT hr = config->UpdateConfiguration (json.toWideCharPointer(), &reply);
                    if (reply != nullptr)
                    {
                        response = juce::String (reply);
                        CoTaskMemFree (reply);
                    }
                    const auto said = response.removeCharacters (" \t\r\n");
                    ok = SUCCEEDED (hr) && (response.isEmpty() || said.containsIgnoreCase ("\"success\":true"));
                    if (FAILED (hr)) response = "UpdateConfiguration failed " + juce::String::toHexString ((int) hr);
                }
                else
                    response = "no loopback transport";
                config->Shutdown();
                config->Release();
            }
            transport->Release();
        }
        else
            response = "no Windows MIDI Services";
        if (comHere) CoUninitialize();
        return ok;
    }

    /** Asks the MIDI service for the pair A = nameA, B = nameB under `association` (a lower-case {GUID}, the same
        on every computer). True when the service made it or already had it. */
    inline bool createLoopback (const juce::String& association, const juce::String& uniqueId,
                                const juce::String& nameA, const juce::String& descriptionA,
                                const juce::String& nameB, const juce::String& descriptionB, juce::String& response)
    {
        auto endpoint = [&uniqueId] (const juce::String& name, const juce::String& description)
        {
            juce::DynamicObject::Ptr o = new juce::DynamicObject();
            o->setProperty ("name", name);
            o->setProperty ("description", description);
            o->setProperty ("uniqueIdentifier", uniqueId);
            return juce::var (o.get());
        };
        juce::DynamicObject::Ptr pair = new juce::DynamicObject();
        pair->setProperty ("endpointA", endpoint (nameA, descriptionA));
        pair->setProperty ("endpointB", endpoint (nameB, descriptionB));
        juce::DynamicObject::Ptr create = new juce::DynamicObject();
        create->setProperty (juce::Identifier (association), juce::var (pair.get()));
        juce::DynamicObject::Ptr section = new juce::DynamicObject();
        section->setProperty ("create", juce::var (create.get()));
        if (configure (juce::var (section.get()), response)) return true;
        return response.containsIgnoreCase ("already");   // asked again for one that is there: it is there
    }

    /** Takes the pair away again (its ports vanish for every app). */
    inline bool removeLoopback (const juce::String& association, juce::String& response)
    {
        juce::DynamicObject::Ptr section = new juce::DynamicObject();
        section->setProperty ("remove", juce::Array<juce::var> { juce::var (association) });
        return configure (juce::var (section.get()), response);
    }
}

#endif
