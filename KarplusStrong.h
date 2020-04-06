/*
  ==============================================================================

    KarplusStrong.h
    Created: 4 Apr 2020 12:39:52pm
    Author:  simon

  ==============================================================================
*/

#pragma once
#include "BasicDelayLine.h"

class KarplusStrong
{
public:
   void setup(int sampleRate)
    {
        sr = sampleRate; // Store samplerate for delaytime values

        // ====== DELAY SETUP =======
        delay.setSize(sampleRate * 10); // Set maxium size of Delay to 10 seconds which should be sufficient to play even the lowest frequencies     
        
        // ====== SMOOTHED VALUES SETUP =======
        smoothFeedback.reset(sampleRate, 0.05f); // Set samplerate and smoothing of 20ms
        smoothFeedback.setCurrentAndTargetValue(0.0); // will be overwritten
    }

    void setDelaytime(float freq, float instabilityAmount)
    {
        float delayFreq = sr / freq; // Get delaytime from freq
        instabilityAmount = random.nextFloat() * instabilityAmount;

        delay.setDelayTimeInSamples(delayFreq + instabilityAmount); // Set delaytime to same amount
    }

    void setTail(float feedAmount) // Takes values between 0-1
    {
        smoothFeedback.setTargetValue(feedAmount); // smooth incoming feedback amount
        float smoothedFeedback = smoothFeedback.getNextValue();

        delay.setFeedback(smoothedFeedback);
    }
     
    void setDampening(float damp) // Takes values between 0-1
    {
        float filterFreq = (damp + 0.01) // Prevent filter hitting 0 Hz
                           * 0.99 // Get relative value
                           * (sr / 2);  // Multiplies dampening by Nyquist Frequency;
        loPass.setCoefficients(IIRCoefficients::makeLowPass(sr, filterFreq, 1.0f));
    }

    float process(float in)
    {
        in = loPass.processSingleSampleRaw(in); // Dampen Incoming Signal
        in = (delay.process(in)); // Apply Feedback Delay

        return in;
    }

private:
    SmoothedValue<float> smoothFeedback;

    BasicDelayLine delay;
    IIRFilter loPass;
    Random random;

    int sr; // Samplerate
};
