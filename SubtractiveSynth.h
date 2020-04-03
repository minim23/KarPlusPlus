/*
  ==============================================================================

    SubtractiveSynth.h
    Created: 3 Apr 2020 10:43:13am
    Author:  simon

  ==============================================================================
*/

#pragma once


class SubtractiveSynth
{
public:

    void setFilter(float sampleRate, float freq, float q)
    {
        // Two Bandpass Filter to accumulate higher Q Values
        resonator1.setCoefficients(IIRCoefficients::makeBandPass(sampleRate, freq, q)); 
        resonator2.setCoefficients(IIRCoefficients::makeBandPass(sampleRate, freq, q));
    }

    /*void setFilterAmount(int filterAmount);
    {
        voiceCount = filterAmount;
    }*/

    float process()
    {
        float currentSample = random.nextFloat(); // White Noise
        currentSample = resonator1.processSingleSampleRaw(currentSample) // Filtering White Noise through Bandpass
                        + resonator2.processSingleSampleRaw(currentSample)
                        * 0.5f; // Adjust volume

        return currentSample;
    }

    float process(float input)
    {
        float currentSample = resonator1.processSingleSampleRaw(input) // Processes the input sample
                            + resonator2.processSingleSampleRaw(currentSample)
                            * 0.5f; // Adjust volume

        return currentSample;
    }

private:
    IIRFilter resonator1, resonator2; //
    Random random; // for White Noise

    int voiceCount;
};