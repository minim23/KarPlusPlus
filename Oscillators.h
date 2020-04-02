//
//  Oscillators.h
//  Week4MoreClasses
//
//  Created by tmudd on 03/02/2020.
//  Copyright © 2020 tmudd. All rights reserved.
//

#include <math.h>
#include <cmath>

#ifndef Oscillators_h
#define Oscillators_h

// BASE class - a basic
class Phasor
{
public:
    
    // Our parent oscillator class does the key things required for most oscillators:
    // -- handles phase
    // -- handles setters and getters for frequency and samplerate
    
    /// update the phase and output the next sample from the oscillator
    float process()
    {
        phase += phaseDelta;
        
        if (phase > 1.0f)
            phase -= 1.0f;
        
        return output(phase);
    }
    
    /// this function is the one that we will replace in the classes that inherit from Phasor
    virtual float output(float p)
    {
        return p;
    }
    
    void setSampleRate(float sr)
    {
        sampleRate = sr;
    }
    
    void setFrequency(float freq)
    {
        frequency = freq;
        phaseDelta = frequency / sampleRate;
    }
    
private:
    float frequency;
    float sampleRate;
    float phase = 0.0f;
    float phaseDelta;
};

// TRIOSC CHILD Class
class TriOsc : public Phasor
{
    // redefine (override) the output function so that we can return a different function of the phase (p)
    float output(float p) override
    {
        return fabsf(p - 0.5f) - 0.5f;
    }
};

// SINEOSC CHILD Class
class SineOsc : public Phasor
{
    // redefine (override) the output function so that we can return a different function of the phase (p)
    float output(float p) override
    {
        return std::sin(p * 2.0f * 3.141592653);
    }
};

// SQUAREOSC CHILD Class
class SquareOsc : public Phasor
{
    // redefine (override) the output function so that we can return a different function of the phase (p)
public:
    float output(float p) override
    {
        float outVal = 0.5;
        if (p > pulseWidth)
            outVal = -5;
        return outVal;
    }
    void setPulseWidth(float pw)
    {
        pulseWidth = pw;
    }
private:
    float pulseWidth = 0.5f;
};

#endif /* Oscillators_h */
