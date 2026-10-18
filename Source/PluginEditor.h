#pragma once
#include "PluginProcessor.h"
#include <vector>

class TempoHelperEditor : public juce::AudioProcessorEditor,
                          private juce::Timer
{
public:
    explicit TempoHelperEditor (TempoHelperProcessor&);
    ~TempoHelperEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override {}
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;

private:
    struct Zone
    {
        juce::Rectangle<float> area;
        juce::String key;
        juce::String value;
    };

    void timerCallback() override;
    float getScale() const;
    const Zone* findZone (juce::Point<float> p) const;
    void drawMeter (juce::Graphics&, juce::Rectangle<float> face, float db, const juce::String& label);

    TempoHelperProcessor& proc;
    double lastBpm = -1.0;
    bool lastHost = false;
    float needleL = -20.0f, needleR = -20.0f;
    float velL = 0.0f, velR = 0.0f;
    juce::String flashKey;
    int flashFrames = 0;
    std::vector<Zone> zones;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TempoHelperEditor)
};