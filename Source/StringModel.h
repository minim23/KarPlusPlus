#pragma once
#include "FeedbackDelay.h"
#include "NonLinAllpass.h"
#include <cmath> // Used for tanh()

// ====== KARPLUS STRONG =======
class KarplusStrong : public Delay
{
public:
    // ====== PROCESS =======
    float process(float inSamp) override
    {
        float outVal = readVal();
        writeVal(inSamp + feedback * outVal); // Feedback scales output back into input
        float floor(outVal); // Calculate interpolation
        float dampString = dampen.processSingleSampleRaw(floor); // process through IRR Filter
        return dampString;
    }

    // ====== DAMPENING =======
    void setDampening(float damp) // Takes values between 0-1
    {
        float filterFreq = (damp + 0.01) // Prevent filter hitting 0 Hz
                            * (getSamplerate() / 2)  // Multiplies dampening by Nyquist Frequency
                            * 0.9; // Get practical value

        dampen.setCoefficients(juce::IIRCoefficients::makeLowPass(getSamplerate(), filterFreq, 1.0f));
    }
    
    // ====== ALLPASS COEFFS =======
    void setAllpass(float coeff1, float coeff2) // Takes values between 0-1
    {
        allpass.setCoefficients(coeff1, coeff2);
    }

    // ====== PITCH WITH INSTABILITY =======
    void setPitch(float freq, float instabilityAmount)
    {
        float delayFreq = getSamplerate() / freq; // Get delaytime from frequency
        instabilityAmount = random.nextFloat() * instabilityAmount; // instability amount equals a frequency value - 20Hz seems practical

        setDelayTimeInSamples(delayFreq + instabilityAmount); // Set delaytime in original function
    }

private:
    juce::IIRFilter dampen;
    juce::Random random;
    
    NonLinearAllpass allpass;
};
