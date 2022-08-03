#pragma once
#include "FeedbackDelay.h"
#include "NonLinAllpass.h"
#include <cmath> // Used for tanh()

// ====== KARPLUS STRONG =======
class KarplusStrong : public Delay
{
public:
    // ====== DAMPENING =======
    void setSamplerate (float samplerate)
    {
        sr = samplerate;
    }
    
    // ====== DAMPENING =======
    void setDampening (float damp) // Takes values between 0-1
    {
        float filterFreq = (damp + 0.01) // Prevent filter hitting 0 Hz
                            * (sr / 2)  // Multiplies dampening by Nyquist Frequency
                            * 0.9; // Get practical value

        dampen.setCoefficients(juce::IIRCoefficients::makeLowPass(sr, filterFreq, 1.0f));
    }
    
    // ====== PITCH WITH INSTABILITY =======
    void setPitch (float freq)
    {
        float delayFreq = sr / freq; // Get delaytime from frequency
        setDelayTimeInSamples (delayFreq); // Set delaytime in original function
        
        float noise = (random.nextFloat() - 0.5) * 2;
        allpass.setCoefficients (noise, noise);
    }
    
    // ====== PROCESS =======
    float process (float inSamp) override
    {
        float outVal = readVal();
        float nonLinearAllpass = allpass.process (outVal);
        float dampString = dampen.processSingleSampleRaw (nonLinearAllpass); // process through IRR Filter
        writeVal (inSamp + feedback * dampString); // Feedback scales output back into input
        float floor (outVal); // Calculate interpolation
        return floor;
    }

private:
    juce::IIRFilter dampen;
    juce::Random random;
    
    // ====== ALLPASS =======
    NonLinearAllpass allpass;
};
