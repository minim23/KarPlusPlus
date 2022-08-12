/*
  ==============================================================================

    OscData.h
    Created: 26 Jul 2022 11:01:58am
    Author:  Simon Weins

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class OscData : public juce::dsp::Oscillator<float>
{
public:
    void setWaveType(const int choice);
    void prepareToPlay (juce::dsp::ProcessSpec& spec);
    void setWaveFrequency (const int midiNoteNumber);
    void getNextAudioBlock (juce::dsp::AudioBlock<float>& block);
    void setFmParams (const float depth, const float freq);
    
private:
    juce::dsp::Oscillator<float> osc { [](float x) { return std::sin (x); }}; /// Sine Wave function
    juce::dsp::Oscillator<float> fmOsc { [](float x) { return std::sin (x); }};
    float fmMod { 0.0f };
    float fmDepth { 0.0f };
    
    int lastMidiNote;
    
    /// return std::sin (x); // SINE WAVE
    /// return x / juce::MathConstants<float>::pi; // SAW WAVE
    /// return x < 0.0f ? -1.0f : 1.0f; // SQUARE
};
