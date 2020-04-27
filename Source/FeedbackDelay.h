#pragma once

#include <cmath>

class FeedbackDelay
{
public:
    
    // ====== CONSTRUCTOR / DESTRUCTOR =======
    FeedbackDelay() {}
    ~FeedbackDelay() {}

    virtual float process(float inSamp)
    {
        float outVal = readVal();
        writeVal(inSamp + feedback * outVal);     // note the feedback here, scaling the output back in to the delay
        float floor(outVal); // calculates interpolation
        return floor;
    }
    
    // ====== SETTER FUNCTIONS =======
    void setSamplerate(float samplerate)
    {
        sr = samplerate;

        // ====== SMOOTHED VALUE SETUP =======
        smoothDelaytime.reset(sr, 0.02f); // Set samplerate and smoothing of 20ms
        smoothDelaytime.setCurrentAndTargetValue(0.0); // will be overwritten

        smoothFeedback.reset(sr, 0.05f); // Smoothing of 50ms
        smoothFeedback.setCurrentAndTargetValue(0.0);
    }

    void setSize(float newSize)
    {
        size = newSize;
        buffer = new float[size];
        for (int i = 0; i < size; i++)
        {
            buffer[i] = 0.0f;
        }
    }

    void setDelayTimeInSamples(float delTime)
    {
        smoothDelaytime.setTargetValue(delTime);
        delayTimeInSamples = smoothDelaytime.getNextValue();

        readPos = writePos - delayTimeInSamples;
        while (readPos < 0)
        readPos += size;
    }
    
    void setFeedback(float fb)
    {
        smoothFeedback.setTargetValue(fb);
        float smoothedFeedback = smoothFeedback.getNextValue();
        feedback = smoothedFeedback;

        if (feedback > 10)
            feedback = 10.0f;
        if (feedback < 0)
            feedback = 0.0f;
    }

    // ====== GETTER FUNCTIONS =======
    float getDelayTimeInSamples()
    {
        return delayTimeInSamples;
    }

    float getFeedback()
    {
        return feedback;
    }

    int getSamplerate()
    {
        return sr;
    }
    
    // ====== UTILITY FUNCTIONS =======
    float readVal()
    {
        float outVal = buffer[readPos];
        readPos++;
        readPos %= size;
        return outVal;
    }

    void writeVal(float inSamp)
    {
        buffer[writePos] = inSamp;
        writePos++;
        writePos %= size;
    }
    
private:
    float* buffer; // Buffer
    int size; // Buffer Size
    
    int writePos = 0;  
    int readPos = 0.0f; 
    
    int delayTimeInSamples;
    
    float feedback = 0.0f; 

    SmoothedValue<float> smoothDelaytime, smoothFeedback;;

    int sr; // Samplerate
    
};


// ====== RESONANT FEEDBACK =======
class ResonantFeedback : public FeedbackDelay
{
public:

    // ====== SETUP SAMPLERATE AND SMOOTHED VALUES =======
    void resonatorSetup()
    {
        smoothQ.reset(getSamplerate(), 0.2f); // Set samplerate and smoothing of 200ms
        smoothQ.setCurrentAndTargetValue(0.0); // will be overwritten
    }

    // ====== SETUP RESONATOR =======
    void setResonator(float freq, float q)
    {
        // ====== SMOOTHED Q =======
        smoothQ.setTargetValue(q);
        float smoothedQ = smoothQ.getNextValue();

        resonator.setCoefficients(IIRCoefficients::makeBandPass(getSamplerate(), freq, smoothedQ + 0.01));

        resGain = 1 + (q / 10); // Define Gainstage to equally increase the Volume with rising Q Value
    }

    // ====== STORE VALUE AND READ CURRENT POSITION =======
    float process(float input) override
    {
        // ====== BACKGROUND NOISE =======
        float noise = random.nextFloat() * 0.0001f;

        // ====== PROCESSING =======
        float outVal = readVal();
        outVal = resonator.processSingleSampleRaw(input + outVal) * resGain; // Multiply resonant filter by gainstage
        outVal = tanh(outVal); // Limit signal before feedback

        writeVal(getFeedback() * outVal); //Feedback

        outVal = tanh(outVal); // Limit output signal after feedback

        float floor(outVal); // calculates interpolation
        return floor;
    }

private:
    IIRFilter resonator;
    SmoothedValue<float> smoothQ;
    float resGain;

    Random random;
};


// ====== KARPLUS STRONG =======
class KarplusStrong : public FeedbackDelay
{
public:
    // ====== PROCESS =======
    float kSProcess(float in)
    {
        in = loPass.processSingleSampleRaw(in); // Dampen Incoming Signal
        in = process(in);

        return in;
    }

    // ====== LOW PASS FILTER =======
    void setDampening(float damp) // Takes values between 0-1
    {
        float filterFreq = (damp + 0.01) // Prevent filter hitting 0 Hz
            * 0.5 // Get practical value   
            * (getSamplerate() / 2);  // Multiplies dampening by Nyquist Frequency;
        loPass.setCoefficients(IIRCoefficients::makeLowPass(getSamplerate(), filterFreq, 1.0f));
    }

    // ====== PITCH WITH INSTABILITY FUNCTION =======
    void setPitch(float freq, float instabilityAmount)
    {
        float delayFreq = getSamplerate() / freq; // Get delaytime from freq
        instabilityAmount = random.nextFloat() * instabilityAmount;

        setDelayTimeInSamples(delayFreq + instabilityAmount); // Set delaytime to same amount
    }

private:

    IIRFilter loPass;
    Random random;
};

