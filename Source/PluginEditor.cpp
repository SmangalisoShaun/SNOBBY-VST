#include "PluginEditor.h"

namespace
{
    constexpr float designW = 560.0f;
    constexpr float designH = 540.0f;
}

TempoHelperEditor::TempoHelperEditor (TempoHelperProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setResizable (true, true);
    setResizeLimits (380, 366, 1120, 1080);
    getConstrainer()->setFixedAspectRatio ((double) designW / (double) designH);
    setSize ((int) designW, (int) designH);
    startTimerHz (10);
}

TempoHelperEditor::~TempoHelperEditor()
{
    stopTimer();
}

void TempoHelperEditor::timerCallback()
{
    auto bpm = proc.currentBpm.load();
    auto host = proc.hostProvidesBpm.load();

    if (bpm != lastBpm || host != lastHost)
    {
        lastBpm = bpm;
        lastHost = host;
        repaint();
    }
}

void TempoHelperEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1b1d22));

    const float scale = juce::jmin ((float) getWidth() / designW,
                                    (float) getHeight() / designH);
    g.addTransform (juce::AffineTransform::scale (scale));

    const double bpm = juce::jmax (1.0, proc.currentBpm.load());
    const double quarterMs = 60000.0 / bpm;

    const auto white  = juce::Colours::white;
    const auto grey   = juce::Colour (0xff9a9a9a);
    const auto accent = juce::Colour (0xff4fc3f7);
    const auto line   = juce::Colour (0xff343841);

    auto text = [&g] (const juce::String& s, int x, int y, int w, int h,
                      juce::Justification j = juce::Justification::left)
    {
        g.drawText (s, x, y, w, h, j, false);
    };
    auto ms  = [] (double v) { return juce::String (v, 1) + " ms"; };
    auto sec = [] (double v) { return juce::String (v, 2) + " s"; };

    // Header
    g.setColour (white);
    g.setFont (24.0f);
    text ("Tempo Helper", 16, 12, 300, 30);

    g.setColour (accent);
    g.setFont (20.0f);
    text (juce::String (bpm, 2) + " BPM", 344, 12, 200, 30, juce::Justification::right);

    g.setColour (grey);
    g.setFont (13.0f);
    text (proc.hostProvidesBpm.load() ? "Following host tempo"
                                      : "No host tempo found (showing 120 BPM)",
          16, 42, 400, 18);

    // Reverb settings (main focus)
    g.setColour (white);
    g.setFont (19.0f);
    text ("Reverb settings", 16, 72, 300, 26);
    g.setColour (line);
    g.fillRect (16, 102, 528, 1);

    // Pre-delay (left column)
    g.setColour (grey);
    g.setFont (14.0f);
    text ("Pre-delay", 16, 112, 250, 22);

    const char* preLabels[] = { "Tight  1/64", "Medium  1/32", "Wide  1/16" };
    const int preDiv[] = { 64, 32, 16 };
    for (int i = 0; i < 3; ++i)
    {
        const int y = 138 + i * 30;
        g.setFont (16.0f);
        g.setColour (white);
        text (preLabels[i], 16, y, 140, 26);
        g.setColour (accent);
        text (ms (240000.0 / (bpm * preDiv[i])), 156, y, 110, 26, juce::Justification::right);
    }

    // Decay (right column)
    g.setColour (grey);
    g.setFont (14.0f);
    text ("Decay time (tail ends on the beat)", 290, 112, 260, 22);

    const char* decLabels[] = { "1 beat", "2 beats", "1 bar", "2 bars", "4 bars" };
    const double decBeats[] = { 1, 2, 4, 8, 16 };
    for (int i = 0; i < 5; ++i)
    {
        const int y = 138 + i * 30;
        g.setFont (16.0f);
        g.setColour (white);
        text (decLabels[i], 290, y, 110, 26);
        g.setColour (accent);
        text (sec (decBeats[i] * quarterMs / 1000.0), 400, y, 144, 26, juce::Justification::right);
    }

    // Divider
    g.setColour (line);
    g.fillRect (16, 300, 528, 1);

    // Delay times
    g.setColour (white);
    g.setFont (17.0f);
    text ("Delay times", 16, 310, 300, 24);

    const int colNote = 16, colStraight = 130, colDotted = 270, colTriplet = 410, colW = 130;

    g.setFont (13.0f);
    g.setColour (grey);
    text ("Note",     colNote,     340, 100,  20);
    text ("Straight", colStraight, 340, colW, 20);
    text ("Dotted",   colDotted,   340, colW, 20);
    text ("Triplet",  colTriplet,  340, colW, 20);

    const int divisions[] = { 1, 2, 4, 8, 16, 32, 64 };
    int y = 362;
    g.setFont (14.0f);
    g.setColour (white);
    for (int d : divisions)
    {
        const double straight = 240000.0 / (bpm * d);
        text ("1/" + juce::String (d),           colNote,     y, 100,  20);
        text (ms (straight),                     colStraight, y, colW, 20);
        text (ms (straight * 1.5),               colDotted,   y, colW, 20);
        text (ms (straight * 2.0 / 3.0),         colTriplet,  y, colW, 20);
        y += 22;
    }
}