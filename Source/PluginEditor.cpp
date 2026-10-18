#include "PluginEditor.h"
#include <cmath>

namespace
{
    constexpr float designW = 640.0f;
    constexpr float designH = 650.0f;

    const juce::Colour cream     (0xffe6d9b8);
    const juce::Colour dimInk    (0xff9c9078);
    const juce::Colour amber     (0xffffb74d);
    const juce::Colour chassis   (0xff2a2724);
    const juce::Colour edge      (0xff4a443d);
    const juce::Colour screenBg  (0xff120f0c);

    void useFont (juce::Graphics& g, float size, bool bold = false, float kerning = 0.0f)
    {
        g.setFont (size);
        juce::Font f = g.getCurrentFont();
        if (bold)           f = f.boldened();
        if (kerning > 0.0f) f = f.withExtraKerningFactor (kerning);
        g.setFont (f);
    }
}

TempoHelperEditor::TempoHelperEditor (TempoHelperProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setResizable (true, true);
    setResizeLimits (384, 390, 1280, 1300);
    getConstrainer()->setFixedAspectRatio ((double) designW / (double) designH);
    setSize ((int) designW, (int) designH);
    startTimerHz (30);
}

TempoHelperEditor::~TempoHelperEditor()
{
    stopTimer();
}

float TempoHelperEditor::getScale() const
{
    return juce::jmin ((float) getWidth() / designW, (float) getHeight() / designH);
}

const TempoHelperEditor::Zone* TempoHelperEditor::findZone (juce::Point<float> p) const
{
    for (const auto& z : zones)
        if (z.area.contains (p))
            return &z;
    return nullptr;
}

void TempoHelperEditor::mouseDown (const juce::MouseEvent& e)
{
    if (auto* z = findZone (e.position / getScale()))
    {
        juce::SystemClipboard::copyTextToClipboard (z->value);
        flashKey = z->key;
        flashFrames = 25;
        repaint();
    }
}

void TempoHelperEditor::mouseMove (const juce::MouseEvent& e)
{
    setMouseCursor (findZone (e.position / getScale()) != nullptr
                        ? juce::MouseCursor::PointingHandCursor
                        : juce::MouseCursor::NormalCursor);
}

void TempoHelperEditor::timerCallback()
{
    const auto bpm = proc.currentBpm.load();
    const auto host = proc.hostProvidesBpm.load();

    const float b = (float) juce::jmax (1.0, bpm);
    const float tempoPos = juce::jlimit (0.0f, 1.0f, (b - 60.0f) / 140.0f);
    const float barPos   = juce::jlimit (0.0f, 1.0f, ((240.0f / b) - 1.2f) / 2.8f);

    const float targetL = -20.0f + tempoPos * 23.0f;
    const float targetR = -20.0f + barPos * 23.0f;

    auto step = [] (float& pos, float& vel, float target)
    {
        vel = (vel + (target - pos) * 0.12f) * 0.78f;
        pos += vel;
    };
    step (needleL, velL, targetL);
    step (needleR, velR, targetR);

    bool needRepaint = bpm != lastBpm || host != lastHost
                    || std::abs (velL) > 0.005f || std::abs (velR) > 0.005f
                    || std::abs (targetL - needleL) > 0.01f
                    || std::abs (targetR - needleR) > 0.01f;

    lastBpm = bpm;
    lastHost = host;

    if (flashFrames > 0)
    {
        --flashFrames;
        needRepaint = true;
    }

    if (needRepaint)
        repaint();
}

void TempoHelperEditor::drawMeter (juce::Graphics& g, juce::Rectangle<float> face,
                                   float db, const juce::String& label)
{
    g.setColour (juce::Colour (0xff0f0d0b));
    g.fillRoundedRectangle (face.expanded (5.0f), 8.0f);
    g.setColour (juce::Colour (0xffe8d9ab));
    g.fillRoundedRectangle (face, 4.0f);

    const juce::Colour ink (0xff3a2f1c);
    const juce::Colour redInk (0xffb02a20);
    const float px = face.getCentreX();
    const float py = face.getBottom() + 60.0f;
    const float radius = 150.0f;
    const float maxAng = juce::degreesToRadians (44.0f);

    auto ang = [maxAng] (float v) { return -maxAng + (v + 30.0f) / 33.0f * 2.0f * maxAng; };
    auto pt = [px, py] (float a, float r)
    {
        return juce::Point<float> (px + r * std::sin (a), py - r * std::cos (a));
    };

    g.saveState();
    g.reduceClipRegion (face.toNearestInt());

    juce::Path arc;
    arc.addCentredArc (px, py, radius - 10.0f, radius - 10.0f, 0.0f, ang (-30.0f), ang (0.0f), true);
    g.setColour (ink);
    g.strokePath (arc, juce::PathStrokeType (1.4f));

    juce::Path redArc;
    redArc.addCentredArc (px, py, radius - 10.0f, radius - 10.0f, 0.0f, ang (0.0f), ang (3.0f), true);
    g.setColour (redInk);
    g.strokePath (redArc, juce::PathStrokeType (4.0f));

    const float marks[] = { -30.0f, -20.0f, -10.0f, -7.0f, -5.0f, -3.0f, 0.0f, 3.0f };
    for (float m : marks)
    {
        const bool major = (m == -20.0f || m == -10.0f || m == -5.0f || m == 0.0f || m == 3.0f);
        g.setColour (m >= 0.0f ? redInk : ink);
        g.drawLine (juce::Line<float> (pt (ang (m), radius - 10.0f),
                                       pt (ang (m), radius - (major ? 22.0f : 17.0f))), 1.4f);
        if (major)
        {
            useFont (g, 11.0f, true);
            const auto c = pt (ang (m), radius - 34.0f);
            const juce::String s = m > 0.0f ? "+" + juce::String ((int) m) : juce::String ((int) m);
            g.drawText (s, (int) (c.x - 16.0f), (int) (c.y - 7.0f), 32, 14,
                        juce::Justification::centred, false);
        }
    }

    const float clamped = juce::jlimit (-30.0f, 3.0f, db);
    const auto tip = pt (ang (clamped), radius + 4.0f);
    g.setColour (juce::Colours::black.withAlpha (0.22f));
    g.drawLine (px + 3.0f, py + 1.0f, tip.x + 3.0f, tip.y + 1.0f, 2.0f);
    g.setColour (juce::Colour (0xff8a1a14));
    g.drawLine (px, py, tip.x, tip.y, 1.8f);

    g.setColour (ink);
    useFont (g, 16.0f, true);
    g.drawText ("VU", (int) face.getX() + 10, (int) face.getBottom() - 24, 40, 20,
                juce::Justification::left, false);
    useFont (g, 13.0f, true);
    g.drawText (label, (int) face.getRight() - 60, (int) face.getBottom() - 24, 50, 20,
                juce::Justification::right, false);

    g.restoreState();
}

void TempoHelperEditor::paint (juce::Graphics& g)
{
    zones.clear();

    g.fillAll (juce::Colour (0xff120f0c));
    g.addTransform (juce::AffineTransform::scale (getScale()));

    const double bpm = juce::jmax (1.0, proc.currentBpm.load());
    const double quarterMs = 60000.0 / bpm;

    auto text = [&g] (const juce::String& s, float x, float y, float w, float h,
                      juce::Justification j = juce::Justification::left)
    {
        g.drawText (s, juce::Rectangle<float> (x, y, w, h).toNearestInt(), j, false);
    };

    g.setColour (juce::Colour (0xff6b4423));
    g.fillRect (0.0f, 0.0f, 14.0f, designH);
    g.fillRect (designW - 14.0f, 0.0f, 14.0f, designH);
    g.setColour (juce::Colour (0xff4f3219));
    for (float gx : { 3.0f, 7.0f, 10.5f })
    {
        g.fillRect (gx, 0.0f, 1.0f, designH);
        g.fillRect (designW - 14.0f + gx, 0.0f, 1.0f, designH);
    }
    g.setColour (chassis);
    g.fillRect (14.0f, 0.0f, designW - 28.0f, designH);
    g.setColour (juce::Colour (0xff3a3632));
    g.fillRect (14.0f, 0.0f, designW - 28.0f, 2.0f);

    auto screw = [&g] (float cx, float cy, float deg)
    {
        g.setColour (juce::Colour (0xff8a8478));
        g.fillEllipse (cx - 6.0f, cy - 6.0f, 12.0f, 12.0f);
        g.setColour (juce::Colour (0xff3a3630));
        g.drawEllipse (cx - 6.0f, cy - 6.0f, 12.0f, 12.0f, 1.0f);
        const float a = juce::degreesToRadians (deg);
        g.drawLine (cx - std::cos (a) * 4.5f, cy - std::sin (a) * 4.5f,
                    cx + std::cos (a) * 4.5f, cy + std::sin (a) * 4.5f, 1.6f);
    };
    screw (24.0f, 16.0f, 35.0f);
    screw (designW - 24.0f, 16.0f, 110.0f);
    screw (24.0f, designH - 14.0f, 80.0f);
    screw (designW - 24.0f, designH - 14.0f, 10.0f);

    g.setColour (juce::Colour (0xff1d1a17));
    g.fillRoundedRectangle (40.0f, 8.0f, 220.0f, 66.0f, 6.0f);
    g.setColour (cream);
    g.drawRoundedRectangle (40.0f, 8.0f, 220.0f, 66.0f, 6.0f, 1.2f);
    useFont (g, 34.0f, true);
    text ("SNOBBY", 40.0f, 10.0f, 220.0f, 38.0f, juce::Justification::centred);
    g.setColour (amber);
    useFont (g, 13.0f, true, 0.35f);
    text ("TEMPO HELPER", 40.0f, 48.0f, 220.0f, 20.0f, juce::Justification::centred);

    g.setColour (screenBg);
    g.fillRoundedRectangle (360.0f, 8.0f, 240.0f, 66.0f, 6.0f);
    g.setColour (edge);
    g.drawRoundedRectangle (360.0f, 8.0f, 240.0f, 66.0f, 6.0f, 1.2f);
    g.setColour (amber);
    useFont (g, 34.0f, true);
    text (juce::String (bpm, 2), 364.0f, 8.0f, 160.0f, 44.0f, juce::Justification::right);
    useFont (g, 14.0f, true);
    text ("BPM", 530.0f, 24.0f, 60.0f, 20.0f);
    g.setColour (dimInk);
    useFont (g, 11.0f, true, 0.12f);
    text (proc.hostProvidesBpm.load() ? "HOST SYNC" : "NO HOST TEMPO - USING 120",
          364.0f, 52.0f, 232.0f, 18.0f, juce::Justification::centred);

    drawMeter (g, juce::Rectangle<float> (28.0f, 86.0f, 286.0f, 108.0f), needleL, "BPM");
    drawMeter (g, juce::Rectangle<float> (326.0f, 86.0f, 286.0f, 108.0f), needleR, "BAR");

    auto framePanel = [&] (juce::Rectangle<float> r, const juce::String& title)
    {
        g.setColour (edge);
        g.drawRoundedRectangle (r.withTrimmedTop (9.0f), 6.0f, 1.5f);
        const float tw = (float) title.length() * 10.0f + 14.0f;
        g.setColour (chassis);
        g.fillRect (r.getX() + 12.0f, r.getY() + 1.0f, tw, 18.0f);
        g.setColour (cream);
        useFont (g, 13.0f, true, 0.25f);
        text (title, r.getX() + 18.0f, r.getY(), tw, 20.0f);
    };

    auto readout = [&] (juce::Rectangle<float> r, const juce::String& shown,
                        const juce::String& copyValue, const juce::String& key, float fontSize)
    {
        const bool flashing = flashFrames > 0 && flashKey == key;
        g.setColour (screenBg);
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (flashing ? amber : edge);
        g.drawRoundedRectangle (r, 4.0f, 1.0f);
        g.setColour (flashing ? juce::Colour (0xff9ccc65) : amber);
        useFont (g, fontSize, true);
        g.drawText (flashing ? juce::String ("COPIED") : shown, r.toNearestInt(),
                    juce::Justification::centred, false);
        zones.push_back (Zone { r, key, copyValue });
    };

    framePanel (juce::Rectangle<float> (28.0f, 206.0f, 584.0f, 192.0f), "REVERB SETTINGS");

    g.setColour (dimInk);
    useFont (g, 12.0f, true, 0.2f);
    text ("PRE-DELAY", 48.0f, 230.0f, 240.0f, 16.0f);
    text ("DECAY (TAIL ENDS ON THE BEAT)", 330.0f, 230.0f, 270.0f, 16.0f);

    const char* preNames[] = { "Tight  1/64", "Medium  1/32", "Wide  1/16" };
    const int preDiv[] = { 64, 32, 16 };
    for (int i = 0; i < 3; ++i)
    {
        const float y = 252.0f + (float) i * 28.0f;
        const double v = 240000.0 / (bpm * preDiv[i]);
        g.setColour (cream);
        useFont (g, 16.0f);
        text (preNames[i], 48.0f, y, 130.0f, 24.0f);
        readout (juce::Rectangle<float> (182.0f, y, 112.0f, 24.0f),
                 juce::String (v, 1) + " ms", juce::String (v, 1), "pre" + juce::String (i), 16.0f);
    }

    const char* decNames[] = { "1 beat", "2 beats", "1 bar", "2 bars", "4 bars" };
    const double decBeats[] = { 1.0, 2.0, 4.0, 8.0, 16.0 };
    for (int i = 0; i < 5; ++i)
    {
        const float y = 252.0f + (float) i * 28.0f;
        const double v = decBeats[i] * quarterMs / 1000.0;
        g.setColour (cream);
        useFont (g, 16.0f);
        text (decNames[i], 330.0f, y, 100.0f, 24.0f);
        readout (juce::Rectangle<float> (440.0f, y, 130.0f, 24.0f),
                 juce::String (v, 2) + " s", juce::String (v, 2), "dec" + juce::String (i), 16.0f);
    }

    framePanel (juce::Rectangle<float> (28.0f, 408.0f, 584.0f, 204.0f), "DELAY TIMES");

    const float colX[] = { 140.0f, 290.0f, 440.0f };
    const char* colNames[] = { "STRAIGHT", "DOTTED", "TRIPLET" };
    g.setColour (dimInk);
    useFont (g, 12.0f, true, 0.2f);
    text ("NOTE", 48.0f, 430.0f, 70.0f, 16.0f);
    for (int c = 0; c < 3; ++c)
        text (colNames[c], colX[c], 430.0f, 124.0f, 16.0f, juce::Justification::centred);

    const int divisions[] = { 1, 2, 4, 8, 16, 32, 64 };
    for (int r = 0; r < 7; ++r)
    {
        const float y = 450.0f + (float) r * 22.0f;
        const double straight = 240000.0 / (bpm * divisions[r]);
        const double vals[] = { straight, straight * 1.5, straight * 2.0 / 3.0 };

        g.setColour (cream);
        useFont (g, 15.0f, true);
        text ("1/" + juce::String (divisions[r]), 48.0f, y, 70.0f, 19.0f);

        for (int c = 0; c < 3; ++c)
            readout (juce::Rectangle<float> (colX[c], y, 124.0f, 19.0f),
                     juce::String (vals[c], 1) + " ms", juce::String (vals[c], 1),
                     "dl" + juce::String (r) + "_" + juce::String (c), 14.0f);
    }

    g.setColour (dimInk);
    useFont (g, 11.0f, true, 0.15f);
    text ("CLICK ANY VALUE TO COPY", 48.0f, 622.0f, 300.0f, 16.0f);
    g.setColour (cream);
    useFont (g, 12.0f, true, 0.25f);
    text ("BY BOOMHYPE TECH", 340.0f, 620.0f, 250.0f, 20.0f, juce::Justification::right);
}
