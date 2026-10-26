#pragma once

#include <JuceHeader.h>
#include "ControlCore.h"

// ============================================================================
// The link between the RONE Control engine (the app that owns the camera)
// and every RONE Control instance (used by the offline tests): localhost TCP, one small binary frame
// per camera frame (~430 bytes). All callbacks arrive on the message thread.
// ============================================================================
namespace RoneCtl
{
constexpr juce::uint32 kLinkMagic = 0x3154564C;   // 'LVT1'

class HandLinkServer : private juce::InterprocessConnectionServer
{
public:
    HandLinkServer() = default;
    ~HandLinkServer() override { stop(); }

    bool start (int port = kPort);                 // false = the port is taken (another engine?)
    void stop();
    void broadcast (const HandFrame& f);
    int  numClients() const;
    bool isRunning() const noexcept { return running; }

private:
    class Conn : public juce::InterprocessConnection
    {
    public:
        Conn() : juce::InterprocessConnection (true, kLinkMagic) {}
        ~Conn() override { disconnect(); }
        void connectionMade() override  { alive = true; }
        void connectionLost() override  { alive = false; }
        void messageReceived (const juce::MemoryBlock&) override {}
        std::atomic<bool> alive { true };
        // the server thread creates us, then hands over the socket: never reap a connection that young
        const double born = juce::Time::getMillisecondCounterHiRes();
    };

    juce::InterprocessConnection* createConnectionObject() override;

    mutable juce::CriticalSection lock;
    juce::OwnedArray<Conn> conns;
    bool running = false;
};

class HandLinkClient : private juce::InterprocessConnection,
                       private juce::Timer
{
public:
    explicit HandLinkClient (int port = kPort);
    ~HandLinkClient() override;

    std::function<void (const HandFrame&)> onFrame;
    bool isLinked() const noexcept { return linked.load(); }

private:
    void timerCallback() override;
    void connectionMade() override;
    void connectionLost() override;
    void messageReceived (const juce::MemoryBlock& message) override;

    std::atomic<bool> linked { false };
    const int port;
};
} // namespace RoneCtl
