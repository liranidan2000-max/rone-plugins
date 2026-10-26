#include "HandLink.h"

namespace RoneCtl
{
// ============================================================================
// Engine side
// ============================================================================
bool HandLinkServer::start (int port)
{
    if (running) return true;
    running = beginWaitingForSocket (port, "127.0.0.1");
    return running;
}

void HandLinkServer::stop()
{
    if (! running) return;
    InterprocessConnectionServer::stop();
    const juce::ScopedLock sl (lock);
    conns.clear();
    running = false;
}

juce::InterprocessConnection* HandLinkServer::createConnectionObject()
{
    // called on the server thread; the connection's own callbacks go to the message thread
    auto* c = new Conn();
    const juce::ScopedLock sl (lock);
    conns.add (c);
    return c;
}

void HandLinkServer::broadcast (const HandFrame& f)
{
    const auto block = f.encode();
    const auto now = juce::Time::getMillisecondCounterHiRes();
    const juce::ScopedLock sl (lock);
    for (int i = conns.size(); --i >= 0;)
    {
        auto* c = conns.getUnchecked (i);
        if (c->isConnected()) { c->sendMessage (block); continue; }
        if (! c->alive.load() || now - c->born > 2000.0)      // a closed plugin
            conns.remove (i);
    }
}

int HandLinkServer::numClients() const
{
    const juce::ScopedLock sl (lock);
    int n = 0;
    for (auto* c : conns) if (c->alive.load() && c->isConnected()) ++n;
    return n;
}

// ============================================================================
// Plugin side
// ============================================================================
HandLinkClient::HandLinkClient (int p)
    : juce::InterprocessConnection (true, kLinkMagic), port (p)
{
    startTimer (1500);
    timerCallback();
}

HandLinkClient::~HandLinkClient()
{
    stopTimer();
    disconnect();
}

void HandLinkClient::timerCallback()
{
    // Nothing listening on localhost is refused at once, so this never stalls the message thread.
    if (! isConnected())
        connectToSocket ("127.0.0.1", port, 100);
}

void HandLinkClient::connectionMade()  { linked.store (true); }
void HandLinkClient::connectionLost()  { linked.store (false); }

void HandLinkClient::messageReceived (const juce::MemoryBlock& message)
{
    HandFrame f;
    if (HandFrame::decode (message, f) && onFrame)
        onFrame (f);
}
} // namespace RoneCtl
