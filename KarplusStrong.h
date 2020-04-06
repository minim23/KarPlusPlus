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

        // ====== Q FILTER SMOOTHING FOR RESONATOR =======
        smoothQ.reset(sampleRate, 0.02f); // Set samplerate and smoothing of 20ms
        smoothQ.setCurrentAndTargetValue(0.0); 

        // ====== DELAY SETUP =======
        delay1.setSize(sampleRate * 10); // Set maxium size of Delay to 10 seconds which should be sufficient to play even the lowest frequencies       
        delay2.setSize(sampleRate * 10); // Set maxium size of Delay to 10 seconds which should be sufficient to play even the lowest frequencies       
    }

    void setDelaytime(float freq)
    {
        note = freq;
        float delayFreq = sr / freq; // Get delaytime from freq

        delay1.setDelayTimeInSamples(delayFreq); // Set delaytime to same amount
        delay2.setDelayTimeInSamples(delayFreq); // Set delaytime to same amount
    }

    void setDelaytime(float freq, float instabilityAmount)
    {
        float delayFreq = sr / freq; // Get delaytime from freq
        instabilityAmount = random.nextFloat() * instabilityAmount;

        delay1.setDelayTimeInSamples(delayFreq + instabilityAmount); // Set delaytime to same amount
        delay2.setDelayTimeInSamples(delayFreq - instabilityAmount); // Set delaytime to same amount
    }

    void setTail(float feedAmount) // Takes values between 0-1
    {
        delay1.setFeedback(feedAmount);
        delay2.setFeedback(feedAmount);
    }
     
    void setDampening(float damp) // Takes values between 0-1
    {
        float filterFreq = (damp + 0.01) // Prevent filter hitting 0 Hz
                           * 0.99 // Get relative value
                           * (sr / 2);  // Multiplies dampening by Nyquist Frequency;
        loPass.setCoefficients(IIRCoefficients::makeLowPass(sr, filterFreq, 1.0f));
    }

    void setResonance(float q, float freq)
    {
        resGain = 1 + (q * 0.01); // setting Amplification for gain equal to q
        
        // ====== SMOOTHING Q =======
        smoothQ.setTargetValue(q); // smooth incoming q value
        float smoothedQ = smoothQ.getNextValue();  
        
        resonator.setCoefficients(IIRCoefficients::makeBandPass(sr, freq, smoothedQ)); // set resonator
    }

    float process(float in)
    {
        in = loPass.processSingleSampleRaw(in); // Dampen Incoming Signal
        //in = resonator.processSingleSampleRaw(in) * resGain; // Resonate dampened signal and multiply by Q value
        in = (delay1.process(in) // Apply Feedback Delay
             + delay2.process(in)) // Apply detuned Feedback Delay
             * 0.5f;
        return in;
    }

private:
    BasicDelayLine delay1, delay2;

    IIRFilter loPass, resonator;

    SmoothedValue<float> smoothQ;

    Random random;

    float resGain;
    float note; // Incoming frequency
    int sr; // Samplerate
};
