#include "PluginEditor.h"

TempoHelperEditor::TempoHelperEditor (TempoHelperProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setSize (560, 520);
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

    const double bpm = juce::jmax (1.0, proc.currentBpm.load());
    const double quarterMs = 60000.0 / bpm;

    auto ms = [] (double v) { return juce::String (v, 1) + " ms"; };
    auto sec = [] (double v) { return juce::String (v, 2) + " s"; };

    int y = 12;

    // Header
    g.setColour (juce::Colours::white);
    g.setFont (22.0f);
    g.drawText ("Tempo Helper", 16, y, 300, 28, juce::Justification::left);

    g.setFont (18.0f);
    g.setColour (juce::Colour (0xff4fc3f7));
    g.drawText (juce::String (bpm, 2) + " BPM", getWidth() - 216, y, 200, 28,
                juce::Justification::right);
    y += 28;

    g.setFont (13.0f);
    g.setColour (juce::Colours::grey);
    g.drawText (proc.hostProvidesBpm.load() ? "Following host tempo"
                                            : "No host tempo found (showing 120 BPM)",
                16, y, 400, 18, juce::Justification::left);
    y += 30;

    // Delay times table
    g.setColour (juce::Colours::white);
    g.setFont (16.0f);
    g.drawText ("Delay times", 16, y, 300, 22, juce::Justification::left);
    y += 26;

    const int colNote = 16, colStraight = 130, colDotted = 270, colTriplet = 410, colW = 130;

    g.setFont (14.0f);
    g.setColour (juce::Colours::grey);
    g.drawText ("Note",     colNote,     y, 100,  20, juce::Justification::left);
    g.drawText ("Straight", colStraight, y, colW, 20, juce::Justification::left);
    g.drawText ("Dotted",   colDotted,   y, colW, 20, juce::Justification::left);
    g.drawText ("Triplet",  colTriplet,  y, colW, 20, juce::Justification::left);
    y += 22;

    const int divisions[] = { 1, 2, 4, 8, 16, 32, 64 };
    g.setColour (juce::Colours::white);
    for (int d : divisions)
    {
        const double straight = 240000.0 / (bpm * d);
        g.drawText ("1/" + juce::String (d), colNote, y, 100, 20, juce::Justification::left);
        g.drawText (ms (straight),           colStraight, y, colW, 20, juce::Justification::left);
        g.drawText (ms (straight * 1.5),     colDotted,   y, colW, 20, juce::Justification::left);
        g.drawText (ms (straight * 2.0 / 3.0), colTriplet, y, colW, 20, juce::Justification::left);
        y += 22;
    }
    y += 14;

    // Pre-delay
    g.setFont (16.0f);
    g.drawText ("Pre-delay suggestions", 16, y, 300, 22, juce::Justification::left);
    y += 26;
    g.setFont (14.0f);
    g.drawText ("Tight: 1/64 = " + ms (240000.0 / (bpm * 64)), 16, y, 520, 20, juce::Justification::left);
    y += 22;
    g.drawText ("Medium: 1/32 = " + ms (240000.0 / (bpm * 32)), 16, y, 520, 20, juce::Justification::left);
    y += 22;
    g.drawText ("Wide: 1/16 = " + ms (240000.0 / (bpm * 16)), 16, y, 520, 20, juce::Justification::left);
    y += 36;

    // Decay
    g.setFont (16.0f);
    g.drawText ("Reverb decay (so the tail ends on the beat)", 16, y, 520, 22, juce::Justification::left);
    y += 26;
    g.setFont (14.0f);
    const char* labels[] = { "1 beat", "2 beats", "1 bar", "2 bars", "4 bars" };
    const double beats[] = { 1, 2, 4, 8, 16 };
    for (int i = 0; i < 5; ++i)
    {
        g.drawText (juce::String (labels[i]) + " = " + sec (beats[i] * quarterMs / 1000.0),
                    16, y, 520, 20, juce::Justification::left);
        y += 22;
    }
}