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
class Assignment_3AudioProcessor : public AudioProcessor
{
public:
    //==============================================================================
    Assignment_3AudioProcessor();
    ~Assignment_3AudioProcessor();

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(AudioBuffer<float>&, MidiBuffer&) override;

    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const String getProgramName(int index) override;
    void changeProgramName(int index, const String& newName) override;

    //==============================================================================
    void getStateInformation(MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Assignment_3AudioProcessor)

    // Parameter Setup
    AudioProcessorValueTreeState parameters;
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
    Synthesiser synth;
    int voiceCount = 16;
};
