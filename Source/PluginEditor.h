#pragma once
#include "PluginProcessor.h"

class TempoHelperEditor : public juce::AudioProcessorEditor,
                          private juce::Timer
{
public:
    explicit TempoHelperEditor (TempoHelperProcessor&);
    ~TempoHelperEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override {}

private:
    void timerCallback() override;

    TempoHelperProcessor& proc;
    double lastBpm = -1.0;
    bool lastHost = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TempoHelperEditor)
};