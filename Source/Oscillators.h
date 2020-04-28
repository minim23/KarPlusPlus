/*
 ==============================================================================
 
 Oscillators.h
 Created: 29 Mar 2019 4:14:43pm
 Author:  tmudd
 
 ==============================================================================
 */

#pragma once

#include <cmath>  // for sin function
#define TWOPI 6.283185307179586476925286766559

/// Base class for oscillators (0-1 phasor)
class Phasor
{
public:
    Phasor()  {};
    
    ~Phasor() {};
    
    /// Note: 0-1
    double process()
    {
        phase += phaseDelta;
        
        if (phase > 1.0)
            phase -= 1.0;
        
        return output(phase);
    }
    
    /// Override for the actual output based on the 0-1 phase
    virtual float output(float p)
    {
        return p;
    }
    
    /// Set the sample rate first
    void setSampleRate(float SR)
    {
        sampleRate = SR;
    }
    /// Make sure the sample rate is set first!
    void setFrequency(float freq)
    {
        frequency = freq;
        phaseDelta = frequency / sampleRate;
    }
    void setPhase(float p)
    {
        phase = p;
    }
    void offsetPhase(float o)
    {
        phase += o;
    }
    
private:
    float frequency;
    float sampleRate;
    double phase = 0.0;
    double phaseDelta;
};







// CHILD Class
class TriOsc : public Phasor
{
    float output(float p) override
    {
        return fabsf(p - 0.5f) - 0.5f;
    }
};


// CHILD Class
class SinOsc : public Phasor
{
    float output(float p) override
    {
        return std::sin(p * TWOPI);
    }
};

// CHILD Class
class SquareOsc : public Phasor
{
public:
    float output(float p) override
    {
        float outVal = 0.5;
        if (p > pulseWidth)
            outVal = -0.5;
        return outVal;
    }
    void setPulseWidth(float pw)
    {
        pulseWidth = pw;
    }
private:
    float pulseWidth = 0.5f;
};

/// LPF a square wave for a smoother LFO without clicks
class SquareLfo : public Phasor
{
public:
    float output(float p) override
    {
        float outVal = 0.5;
        if (p > pulseWidth)
            outVal = -0.5;
        
        return outVal;
    }
    void setPulseWidth(float pw)
    {
        pulseWidth = pw;
    }
    float lowpass(float inVal)
    {
        float outVal = previousVal + (1.0f - smoothing)*(inVal-previousVal);
        previousVal = outVal;
        return outVal;
    }
    
    void setSmoothing(float inSmooth)   {   smoothing = inSmooth;  }
private:
    float pulseWidth = 0.5f;
    float previousVal = 0.0f;
    float smoothing = 0.5f;
};



/// Sample and Hold LFO between 0 and 1
class NoiseLFO : public Phasor
{
    float output(float p) override
    {
        if (p > 0.5f && switchOver)
        {
            outputVal = r.nextFloat();
            switchOver = false;
        }
        if (p < 0.5f)
        {
            switchOver = true;
        }
        return outputVal;
    }
private:
    Random r;
    bool switchOver = false;
    float outputVal=0.0f;
};



// CHILD Class
class HalfSine : public Phasor
{
public:
    float output(float p) override
    {
        float val = 0.0f;
        if (p < pulseWidth)
            val = std::sin(p * 2.0 * TWOPI / pulseWidth);
            return val*val;
    }
    void setPulseWidth(float pw)
    {
        pulseWidth = pw;
    }
private:
    float pulseWidth = 0.5f;
};

class DoubleOsc
{
public:
    void setFrequency(float freq)
    {
        frequency = freq;
        osc1.setFrequency(freq);
        osc2.setFrequency(freq + difference);
    }
    void setSampleRate(float sr)
    {
        osc1.setSampleRate(sr);
        osc2.setSampleRate(sr);
    }
    void setDifference(float diff)
    {
        difference = diff;
        setFrequency(frequency);
    }
    float process()
    {
        float val = osc1.process() + osc2.process();
        return val * 0.5f;
    }
    
private:
    TriOsc osc1;
    TriOsc osc2;
    float difference = 5.0f;
    float frequency;
};



