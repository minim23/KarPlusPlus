#pragma once

class Formants
{
public:
    void setSamplerate(float sampleRate)
    {
        sr = sampleRate;
    }
    
    // ====== SETUP FORMANTS =======
    void setFormants()
    {
        for (int i = 0; i < formantAmount; i++)
        {
            formants.add(new juce::IIRFilter());
        }
    }
    
    // ====== FORMANT COEFFICIENTS =======
    void setCoeff(float formantScaling, float resonance)
    {
        for (int i = 0; i < formantAmount; i++)
        {
            auto maxRandFreq = formantScaling // Get relative value
                                * 1000 // Max. Freq for ranomization of filters
                                + 1; // Next Int can not be 0

            auto maxFreq = formantScaling // Get relative value
                            * 4000 // Rough max. Freq
                            + 100; // Absolute min. Freq
            
            auto freq = random.nextInt (maxRandFreq) + maxFreq;
            
            float variation = random.nextFloat() * resonance;

            formants[i]->setCoefficients (juce::IIRCoefficients::makePeakFilter (sr, freq , variation, variation));
            DBG (freq);
        }
    }
    
    float process (float source)
    {
        for (auto* formant : formants) // Process Impulse through array of randomized Bandpass Filters on left and right channel
        {
             exColour = formant->processSingleSampleRaw(source);
        }
            
        source = exColour // Take formanted impulse
                      / formantAmount // Adjust volume relative to formant amount
                      * sqrt(formantAmount); // Adjust volume relative to resonance of filter - normally this should be square root of Q, but this did not seem practical
        
        return source;
    }
    
private:
    juce::Random random; // for Filterbank
    juce::OwnedArray<juce::IIRFilter> formants;
    float exColour = 0.0f; // Setting up colouration - for some reason it sounded differently when exciter was assigned to formants directly
    int formantAmount = 6;
    
    float sr;
    
};
