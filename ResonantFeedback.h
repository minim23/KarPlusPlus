/*
  ==============================================================================

    ResonantFeedback.h
    Created: 10 Apr 2020 10:47:25am
    Author:  simon

  ==============================================================================
*/

#include <cmath>
#include "Limiter.h"

/// basic delay line with linear interpolation
class ResonantFeedback
{
public:

    /// Destructor - called when the object is destroyed
    ~ResonantFeedback()
    {
    }

    /// store a value in the delay and retrieve the sample at the current read position
    float process(float input, float noiseLevel)
    {
        float noise = random.nextFloat() * noiseLevel;
        
        float outVal = readVal();
        
        outVal = resonator.processSingleSampleRaw(input + outVal) * resGain;
        outVal = tanh(outVal);

        writeVal(feedback * outVal);     // note the feedback here, scaling the output back in to the delay

        outVal = tanh(outVal);

        float floor(outVal); // calculates interpolation
        return floor;
        
    }

    /// read a value from the buffer at the read head position, then increment and wrap the readPos
    float readVal()
    {
        float outVal = buffer[readPos];
        readPos++;
        readPos %= size;
        return outVal;
    }

    /// write a value to the buffer at the write head position, then increment and wrap the writePos
    void writeVal(float inSamp)
    {
        buffer[writePos] = inSamp;
        writePos++;
        writePos %= size;
    }

    /// set the actual delay time in samples
    void setDelayTimeInSamples(float delTime)
    {
        smoothDelaytime.setTargetValue(delTime);

        delayTimeInSamples = smoothDelaytime.getNextValue();
        readPos = writePos - delayTimeInSamples;
        while (readPos < 0)
            readPos += size;
    }

    float getDelayTimeInSamples()
    {
        return delayTimeInSamples;
    }

    /// initialise the float array to a given maximum size (your specified delay time must always be less than this)
    void setSize(float newSize)
    {
        size = newSize;
        buffer = new float[size];
        for (int i = 0; i < size; i++)
        {
            buffer[i] = 0.0f;
        }
    }

    void setFeedback(float fb)
    {
        feedback = fb;
        // check we're not going to get crazy positive feedback:
        if (feedback > 1)
            feedback = 1.0f;
        if (feedback < 0)
            feedback = 0.0f;
    }

    void setFormants(int samplerate, float maxFreq)
    {
        sr = samplerate;
       
        //formants.setCoefficients(IIRCoefficients::makeBandPass(samplerate, freq, q));

    }

    void setResonator(int samplerate, float freq, float q)
    {
        // ====== SMOOTHED VALUE SETUP =======
        smoothDelaytime.reset(samplerate, 0.02f); // Set samplerate and smoothing of 20ms
        smoothDelaytime.setCurrentAndTargetValue(0.0); // will be overwritten

        smoothQ.reset(samplerate, 0.02f); // Set samplerate and smoothing of 200ms
        smoothQ.setCurrentAndTargetValue(0.0); // will be overwritten
        
        smoothQ.setTargetValue(q);
        float smoothedQ = smoothQ.getNextValue();
        
        // ====== RESONATOR SETUP =======
        resonator.setCoefficients(IIRCoefficients::makeBandPass(samplerate, freq, smoothedQ + 0.01));

        resGain = 1 + (q / 10); // Define Gainstage to equally increase the Volume with rising Q Value
    }


private:
    
    // ====== DELAY SPECIFICS =======
    float* buffer;              // the actual buffer - not yet initialised to a size
    int size;                   // buffer size in samples

    int writePos = 0;            // write head location - should always be 0 ≤ writePos < size
    int readPos = 0.0f;        // read head location - should always be 0 ≤ readPos < size

    int delayTimeInSamples;

    float feedback = 0.0f;       // how much of the delay line to mix with the input?

    // ====== SMOOTHED VALUES =======
    SmoothedValue<float> smoothDelaytime;
    SmoothedValue<float> smoothQ;
    
    // ====== RESONATOR =======
    IIRFilter resonator;
    int resonatorAmount = 1;

    float resGain;

    Limiter limiter;

    Random random;
    int sr;
};
