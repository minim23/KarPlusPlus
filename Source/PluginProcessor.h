/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "MySynthesiser.h"

//==============================================================================
/**
*/
class KarPlusPlus2AudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    KarPlusPlus2AudioProcessor();
    ~KarPlusPlus2AudioProcessor();

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KarPlusPlus2AudioProcessor)

    // Parameter Setup
    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float>* karplusVolParam;

    std::atomic<float>* dampParam;
    std::atomic<float>* sustainParam;
    std::atomic<float>* tailParam;
    std::atomic<float>* instabilityParam;

    std::atomic<float>* detuneParam;

    std::atomic<float>* feedbackParam;
    std::atomic<float>* delaytimeParam;
    std::atomic<float>* qParam;
    std::atomic<float>* feedbackAgeParam;
    std::atomic<float>* offsetParam;

    std::atomic<float>* releaseParam;
    std::atomic<float>* volumeParam;

    // Synthesiser class
    juce::Synthesiser synth;
    int voiceCount = 16;
};
