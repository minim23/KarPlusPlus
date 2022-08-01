#pragma once
#include "FeedbackDelay.h"
#include <cmath> // Used for tanh()

// ====== KARPLUS STRONG =======
class KarplusStrong : public FeedbackDelay
{
public:
    // ====== PROCESS =======
    float process(float inSamp) override
    {
        float outVal = readVal();
        writeVal(inSamp + feedback * outVal); // Feedback scales output back into input
        float floor(outVal); // Calculate interpolation
        return floor;
    }

    // ====== DAMPENING =======
    void setDampening(float damp) // Takes values between 0-1
    {
        float filterFreq = (damp + 0.01) // Prevent filter hitting 0 Hz
                            * (getSamplerate() / 2)  // Multiplies dampening by Nyquist Frequency
                            * 0.9; // Get practical value

        dampen.setCoefficients(juce::IIRCoefficients::makeLowPass(getSamplerate(), filterFreq, 1.0f));
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
};
