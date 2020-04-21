/*
  ==============================================================================

    ResonantFeedback.h
    Created: 10 Apr 2020 10:47:25am
    Author:  simon

  ==============================================================================
*/

#include <cmath>

class ResonantFeedback
{
public:

    // ====== DESTRUCTOR =======
    ~ResonantFeedback()
    {
    }

    // ====== STORE VALUE AND READ CURRENT POSITION =======
    float process(float input, float noiseLevel)
    {
        // ====== BACKGROUND NOISE =======
        float noise = random.nextFloat() * noiseLevel;
        
        // ====== PROCESSING =======
        float outVal = readVal();
        outVal = resonator.processSingleSampleRaw(input + outVal) * resGain; // Multiply resonant filter by gainstage
        outVal = tanh(outVal); // Limit signal before feedback

        writeVal(feedback * outVal); 

        outVal = tanh(outVal); // Limit output signal after feedback

        float floor(outVal); // calculates interpolation
        return floor;
        
    }

    // ====== READ BUFFER =======
    float readVal()
    {
        float outVal = buffer[readPos];
        readPos++;
        readPos %= size;
        return outVal;
    }

    // ====== WRITE INTO BUFFER =======
    void writeVal(float inSamp)
    {
        buffer[writePos] = inSamp;
        writePos++;
        writePos %= size;
    }

    // ====== DELAY TIME =======
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

    // ====== SET MAXIMUM SIZE =======
    void setSize(float newSize)
    {
        size = newSize;
        buffer = new float[size];
        for (int i = 0; i < size; i++)
        {
            buffer[i] = 0.0f;
        }
    }

    // ====== SET FEEDBACK =======
    void setFeedback(float fb)
    {
        feedback = fb;
        // check we're not going to get crazy positive feedback:
        if (feedback > 1)
            feedback = 1.0f;
        if (feedback < 0)
            feedback = 0.0f;
    }

    // ====== SETUP SAMPLERATE AND SMOOTHED VALUES =======
    void setup(int samplerate)
    {
        // ====== SMOOTHED VALUE SETUP =======
        smoothDelaytime.reset(samplerate, 0.02f); // Set samplerate and smoothing of 20ms
        smoothDelaytime.setCurrentAndTargetValue(0.0); // will be overwritten

        smoothQ.reset(samplerate, 0.02f); // Set samplerate and smoothing of 200ms
        smoothQ.setCurrentAndTargetValue(0.0); // will be overwritten

        sr = samplerate;
    }

    // ====== SETUP RESONATOR =======
    void setResonator(float freq, float q)
    {   
        // ====== SMOOTHED Q =======
        smoothQ.setTargetValue(q);
        float smoothedQ = smoothQ.getNextValue();
        
        resonator.setCoefficients(IIRCoefficients::makeBandPass(sr, freq, smoothedQ + 0.01));

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

    // ====== UTILITY =======
    Random random;
    int sr;
};
