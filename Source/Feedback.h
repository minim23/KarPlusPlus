/*
  ==============================================================================

    Feedback.h
    Created: 6 Apr 2020 5:42:30pm
    Author:  simon

  ==============================================================================
*/

#pragma once
#include "BasicDelayLine.h"
#include "Limiter.h"

class Feedback
{
public:
    void setup(int sampleRate)
    {
        sr = sampleRate; // Store samplerate for delaytime values

        // ====== DELAY SETUP =======
        delay.setSize(sampleRate * 100); // Set maxium size of Delay to 10 seconds which should be sufficient to play even the lowest frequencies     

        // ====== SMOOTHED VALUES SETUP =======
        smoothFeedback.reset(sampleRate, 0.05f); // Set samplerate and smoothing of 20ms
        smoothFeedback.setCurrentAndTargetValue(0.0); // will be overwritten

        smoothDelaytime.reset(sampleRate, 0.05f); // Set samplerate and smoothing of 20ms
        smoothDelaytime.setCurrentAndTargetValue(0.0); // will be overwritten
    }

    void setDelaytime(float delaytime)
    {
        // ====== SMOOTH DELAYTIME =======
        smoothDelaytime.setTargetValue(delaytime); // smooth incoming feedback amount
        float smoothedDelaytime = smoothDelaytime.getNextValue();
        float delayT = delaytime * 500.0f; // Arbitrary sample value

        delay.setDelayTimeInSamples(delayT); // Set delaytime to same amount
    }

    void setFeedback(float feedAmount) // Takes values between 0-1
    {
        // ====== SMOOTH FEEDBACK AMOUNT =======
        smoothFeedback.setTargetValue(feedAmount); // smooth incoming feedback amount
        float smoothedFeedback = smoothFeedback.getNextValue();

        delay.setFeedback(smoothedFeedback);
    }

    void setFilter(float lowPass, float highPass) // Takes values between 0-1
    {
        dC.setCoefficients(IIRCoefficients::makeHighPass(sr, 20.0f, 1.0f));
        
        loPass.setCoefficients(IIRCoefficients::makeLowPass(sr, lowPass, 1.0f));
        hiPass.setCoefficients(IIRCoefficients::makeHighPass(sr, highPass, 1.0f));
    }

    float process(float in)
    {
        in = (delay.process(loPass.processSingleSampleRaw(in) + hiPass.processSingleSampleRaw(in) * 0.5)); // Insert HiPassed and LoPassed Signal into Feedback Chain and divide volume
        in = dC.processSingleSampleRaw(in); // Prevent dC Offset
        in = limiter.foldback(in, 0.99f); // Fold back signal if it exceeds threshold

        return in;
    }

private:
    SmoothedValue<float> smoothFeedback;
    SmoothedValue<float> smoothDelaytime;

    BasicDelayLine delay;
    IIRFilter dC, loPass, hiPass;
    Limiter limiter;

    int sr; // Samplerate
};
