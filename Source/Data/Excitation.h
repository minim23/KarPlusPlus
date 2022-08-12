#pragma once

class Excitation
{
public:
    // ====== PROCESS AUDIO =======
    float process()
    {
        float noise = (random.nextFloat() - 0.5) * 2.0f;
        float dampenedNoise = dampen.processSingleSampleRaw(noise);
        return dampenedNoise;
    }
    
    // ====== SETTER FUNCTIONS =======
    void setSamplerate(float samplerate)
    {
        sr = samplerate; // Assign to private member variable
    }
    
    // ====== DAMPENING =======
    void setDampening(float damp) // Takes values between 0-1
    {
        float filterFreq = (damp + 0.01) // Prevent filter hitting 0 Hz
                            * (sr / 2)  // Multiplies dampening by Nyquist Frequency
                            * 0.9; // Get practical value

        dampen.setCoefficients(juce::IIRCoefficients::makeLowPass(sr, filterFreq, 1.0f));
    }
    
private:
    juce::Random random;
    juce::IIRFilter dampen;
    
    float sr;
};
