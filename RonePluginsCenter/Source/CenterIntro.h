#pragma once

#include <JuceHeader.h>

// ============================================================================
// CenterIntro - the app icon alone on the desktop at launch, before the window
// (Liran 1.11: "like RONE VST MANAGER, a little faster - the Center opens
// quicker"). Same language as RoneVstManager/Source/Intro.h, on the Center's
// own icon (Resources/AppIcon.png): the graphite tile pops in on a soft
// shadow, the white R draws itself with a spark of purple neon at its tip,
// the bar rises, the neon blooms once and a glint crosses the tile. Then it
// breathes until the page is drawn, and zooms out over the window as it
// comes up. About one second; a click skips it; never longer than 10 s.
// ============================================================================
class CenterIntro : public juce::Component, private juce::Timer
{
public:
    std::function<void()> onReveal;   // the exit starts: show the main window now
    std::function<void()> onDone;     // gone from the screen: delete it

    explicit CenterIntro (juce::Point<int> centre)
    {
        tile.addRoundedRectangle (0.0f, 0.0f, 512.0f, 512.0f, 112.0f);
        bar.startNewSubPath (0, 0); bar.lineTo (4.9f, 0); bar.lineTo (4.9f, 16.4f); bar.lineTo (0, 19.2f); bar.closeSubPath();
        line = juce::Drawable::parseSVGPath ("M 0 2.1 H 46.7 A 10.5 10.5 0 0 1 46.7 23.1 H 11.6 A 6.9 6.9 0 0 0 11.6 36.9 H 34 C 46 36.9 47 48.2 59 48.2 H 100");
        lineLength = line.getLength();

        // The shadow once, at full size; each frame only scales and fades it.
        shadow = juce::Image (juce::Image::ARGB, kSide, kSide, true);
        {
            juce::Graphics g (shadow);
            juce::Path p (tile);
            p.applyTransform (juce::AffineTransform::translation (-256.0f, -256.0f).scaled (kIcon / 512.0f)
                                  .translated ((float) kSide * 0.5f, (float) kSide * 0.5f));
            juce::DropShadow (juce::Colours::black.withAlpha (0.6f), 34, { 0, 14 }).drawForPath (g, p);
        }

        setOpaque (false);
        setSize (kSide, kSide);
        setCentrePosition (centre);
        setWantsKeyboardFocus (false);
        addToDesktop (juce::ComponentPeer::windowIsTemporary);   // no frame, no taskbar button
        setAlwaysOnTop (true);
        setVisible (true);
        startTimerHz (60);   // the clock starts on the first tick, once the loop runs
    }

    ~CenterIntro() override { stopTimer(); }

    /** The library is drawn: the intro ends once its entrance has played. */
    void pageReady() { ready = true; }

    /** Straight to the exit (a click, a second launch, a link). */
    void skip() { if (exitAt < 0) beginExit (now()); }

    void mouseDown (const juce::MouseEvent&) override { skip(); }

    void paint (juce::Graphics& g) override
    {
        const double t = now();
        const float side = (float) kSide;

        // The whole icon: pops in, breathes while it waits, zooms out as it goes.
        float scale = 0.0f, alpha = 1.0f;
        {
            const double pop = juce::jlimit (0.0, 1.0, t / 0.42);
            scale = (float) (0.6 + 0.4 * easeOutBack (pop, 1.7));
            alpha = (float) easeOut (juce::jlimit (0.0, 1.0, t / 0.18));
            if (t > kEntrance)
                scale *= 1.0f + 0.01f * (float) std::sin ((t - kEntrance) * juce::MathConstants<double>::twoPi * 0.7);
            if (exitAt >= 0)
            {
                const double x = juce::jlimit (0.0, 1.0, (t - exitAt) / kExit);
                scale *= (float) (1.0 + 0.3 * easeIn (x));
                alpha *= (float) (1.0 - easeIn (x));
            }
        }
        if (alpha <= 0.001f) return;

        const float icon = kIcon / 512.0f * scale;   // icon units (512) -> pixels
        const float tilt = (float) ((1.0 - easeOut (juce::jlimit (0.0, 1.0, t / 0.5))) * -0.09);
        const auto at = juce::AffineTransform::translation (-256.0f, -256.0f)
                            .rotated (tilt).scaled (icon).translated (side * 0.5f, side * 0.5f);

        // 1. The shadow, under the tile.
        g.setOpacity (alpha * (float) easeOut (juce::jlimit (0.0, 1.0, (t - 0.04) / 0.4)));
        g.drawImageTransformed (shadow, juce::AffineTransform::translation (-side * 0.5f, -side * 0.5f)
                                            .rotated (tilt).scaled (scale).translated (side * 0.5f, side * 0.5f));
        g.setOpacity (1.0f);

        // 2. The graphite tile: lighter at the top, a breath of purple at the
        //    foot, darker to the right, and its thin light rim (AppIcon.png).
        juce::Path sq (tile);
        sq.applyTransform (at);
        {
            const auto top = juce::Point<float> (256.0f, 0.0f).transformedBy (at);
            const auto bot = juce::Point<float> (256.0f, 512.0f).transformedBy (at);
            juce::ColourGradient body (juce::Colour (0xff22262b).withMultipliedAlpha (alpha), top,
                                       juce::Colour (0xff26243a).withMultipliedAlpha (alpha), bot, false);
            body.addColour (0.3, juce::Colour (0xff262a30).withMultipliedAlpha (alpha));
            g.setGradientFill (body);
            g.fillPath (sq);

            const auto left  = juce::Point<float> (0.0f, 256.0f).transformedBy (at);
            const auto right = juce::Point<float> (512.0f, 256.0f).transformedBy (at);
            g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.0f), left,
                                                     juce::Colours::black.withAlpha (0.16f * alpha), right, false));
            g.fillPath (sq);

            juce::Path rim;
            rim.addRoundedRectangle (1.5f, 1.5f, 509.0f, 509.0f, 110.5f);
            g.setColour (juce::Colour (0xff3d424b).withMultipliedAlpha (alpha));
            g.strokePath (rim, juce::PathStrokeType (3.0f * icon), at);
        }

        // 3. The R: its line runs with a spark at its tip, its bar rises.
        //    Purple neon under white ink, the glow fuller once the mark lands.
        const auto rAt = juce::AffineTransform::scale (3.4f).translated (88.0f, 166.0f).followedBy (at);
        const float drawn = (float) easeInOut (juce::jlimit (0.0, 1.0, (t - kR) / 0.48));
        const float rise  = (float) easeOutBack (juce::jlimit (0.0, 1.0, (t - kR - 0.28) / 0.26), 1.5);
        float neon = 0.55f;
        {
            const double b = (t - kBloom) / 0.4;   // the bloom: up fast, settles
            if (b > 0.0)
                neon = (float) (b < 1.0 ? 1.0 + 0.6 * std::sin (b * juce::MathConstants<double>::pi) * (1.0 - b * 0.3) : 1.0);
            if (t > kEntrance)
                neon *= 0.92f + 0.08f * (float) std::sin ((t - kEntrance) * 3.6);
        }

        // Placed on screen first: a stroke's width is in pixels, never transformed.
        const float unit = 3.4f * icon;   // one mark unit, in pixels
        juce::Path part = drawn > 0.0f ? partOf (line, drawn * lineLength) : juce::Path();
        part.applyTransform (rAt);
        auto b = bar;
        if (rise > 0.0f)
            b.applyTransform (juce::AffineTransform::scale (1.0f, rise, 0.0f, 19.2f).followedBy (rAt));

        const auto violet = juce::Colour (0xff7b4cdb);
        for (int ring = 7; ring >= 1; --ring)
        {
            const float w = (kInk + (float) ring * 2.2f) * unit;
            g.setColour (violet.withAlpha (juce::jmin (1.0f, alpha * neon * 0.07f * (float) (8 - ring) / 4.0f)));
            if (drawn > 0.0f)
                g.strokePath (part, juce::PathStrokeType (w, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            if (rise > 0.0f)
                g.strokePath (b, juce::PathStrokeType ((float) ring * 2.2f * unit, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        const auto ink = juce::Colour (0xffe8eaed).withMultipliedAlpha (alpha);
        g.setColour (ink);
        if (drawn > 0.0f)
            g.strokePath (part, juce::PathStrokeType (kInk * unit, juce::PathStrokeType::mitered, juce::PathStrokeType::butt));
        if (rise > 0.0f)
            g.fillPath (b);

        // The spark: a small violet-white light riding the tip while it draws.
        if (drawn > 0.0f && drawn < 1.0f)
        {
            const auto tip = part.getCurrentPosition();
            const float r = 9.0f * icon;   // 9 icon units
            const float life = (float) std::sin (juce::jlimit (0.0, 1.0, (double) drawn) * juce::MathConstants<double>::pi);
            juce::ColourGradient spark (juce::Colours::white.withAlpha (0.95f * alpha * life), tip,
                                        violet.withAlpha (0.0f), tip.translated (r * 2.2f, 0.0f), true);
            spark.addColour (0.35, violet.brighter (0.4f).withAlpha (0.55f * alpha * life));
            g.setGradientFill (spark);
            g.fillEllipse (juce::Rectangle<float> (r * 4.4f, r * 4.4f).withCentre (tip));
        }

        // 4. The glint: a band of light crossing the tile once.
        {
            const double gx = (t - kGlint) / 0.45;
            if (gx > 0.0 && gx < 1.0)
            {
                juce::Graphics::ScopedSaveState s (g);
                g.reduceClipRegion (sq);
                const float x = (float) (-200.0 + 920.0 * easeInOut (gx));
                const auto a = juce::Point<float> (x, 0.0f).transformedBy (at);
                const auto c = juce::Point<float> (x + 150.0f, 90.0f).transformedBy (at);
                juce::ColourGradient band (juce::Colours::white.withAlpha (0.0f), a, juce::Colours::white.withAlpha (0.0f), c, false);
                band.addColour (0.5, juce::Colours::white.withAlpha (0.16f * alpha));
                g.setGradientFill (band);
                g.fillPath (sq);
            }
        }
    }

private:
    static constexpr int kSide = 400;                // the window: room for the shadow and the zoom out
    static constexpr float kIcon = 220.0f;           // the icon's size on screen (512 units)
    static constexpr float kInk = 4.4f;              // the R's stroke, in mark units (AppIcon.png)
    static constexpr double kR = 0.10, kBloom = 0.62, kGlint = 0.66;
    static constexpr double kEntrance = 1.05, kExit = 0.3, kLongest = 10.0;

    double now() const { return start > 0.0 ? (juce::Time::getMillisecondCounterHiRes() - start) / 1000.0 : 0.0; }

    void timerCallback() override
    {
        if (start <= 0.0)
            start = juce::Time::getMillisecondCounterHiRes();
        const double t = now();
        if (exitAt < 0 && ((ready && t >= kEntrance) || t > kLongest))
            beginExit (t);
        if (exitAt >= 0 && t >= exitAt + kExit)
        {
            stopTimer();
            setVisible (false);
            if (onDone) onDone();
            return;
        }
        repaint();
    }

    void beginExit (double t)
    {
        exitAt = t;
        if (onReveal) onReveal();
        toFront (false);   // over the window that just came up, while it zooms away
    }

    /** The first `length` of a path (flattened, fine enough for a 2x screen). */
    static juce::Path partOf (const juce::Path& p, float length)
    {
        juce::Path out;
        juce::PathFlatteningIterator it (p, {}, 0.05f);
        float done = 0.0f;
        bool started = false;
        while (it.next())
        {
            const juce::Line<float> seg (it.x1, it.y1, it.x2, it.y2);
            if (! started) { out.startNewSubPath (seg.getStart()); started = true; }
            const float l = seg.getLength();
            if (done + l >= length)
            {
                out.lineTo (seg.getPointAlongLine (juce::jmax (0.0f, length - done)));
                break;
            }
            out.lineTo (seg.getEnd());
            done += l;
        }
        return out;
    }

    static double easeOut (double x)   { return 1.0 - std::pow (1.0 - x, 3.0); }
    static double easeIn (double x)    { return x * x * x; }
    static double easeInOut (double x) { return x < 0.5 ? 4.0 * x * x * x : 1.0 - std::pow (-2.0 * x + 2.0, 3.0) / 2.0; }
    static double easeOutBack (double x, double over)
    {
        const double c3 = over + 1.0;
        return 1.0 + c3 * std::pow (x - 1.0, 3.0) + over * std::pow (x - 1.0, 2.0);
    }

    juce::Path tile, bar, line;
    float lineLength = 1.0f;
    juce::Image shadow;
    double start = 0.0, exitAt = -1.0;
    bool ready = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CenterIntro)
};
