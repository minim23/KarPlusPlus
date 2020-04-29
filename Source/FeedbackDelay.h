#pragma once
#include <cmath> // Used for tanh()

class FeedbackDelay
{
public:
    
    // ====== CONSTRUCTOR / DESTRUCTOR =======
    FeedbackDelay() {}
    ~FeedbackDelay() {}

    // ====== STANDART PROCESS THAT CAN BE REPLACED OR RE-USED =======
    virtual float process(float inSamp)
    {
        float outVal = readVal();
        writeVal(inSamp + feedback * outVal); // Feedback scales output back into input
        float floor(outVal); // Calculate interpolation
        return floor;
    }
    
    // ====== SETTER FUNCTIONS =======
    void setSamplerate(float samplerate)
    {
        sr = samplerate;

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
        delayTimeInSamples = smoothDelaytime.getNextValue(); // Smoothed value

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
    
    int writePos = 0;  // Write Position
    int readPos = 0.0f;  // Read Position
    
    int delayTimeInSamples;
    
    float feedback = 0.0f; 

    SmoothedValue<float> smoothDelaytime, smoothFeedback;;

    int sr; // Samplerate
    
};

// ====== KARPLUS STRONG =======
class KarplusStrong : public FeedbackDelay
{
public:
    // ====== PROCESS =======
    float kSProcess(float in)
    {
        in = dampen.processSingleSampleRaw(in); // Dampen Incoming Signal
        in = process(in); // Use original processing

        return in;
    }

    // ====== DAMPENING =======
    void setDampening(float damp) // Takes values between 0-1
    {
        float filterFreq = (damp + 0.01) // Prevent filter hitting 0 Hz
                            * (getSamplerate() / 2)  // Multiplies dampening by Nyquist Frequency
                            * 0.9; // Get practical value   

        dampen.setCoefficients(IIRCoefficients::makeLowPass(getSamplerate(), filterFreq, 1.0f));
    }

    // ====== PITCH WITH INSTABILITY =======
    void setPitch(float freq, float instabilityAmount)
    {
        float delayFreq = getSamplerate() / freq; // Get delaytime from frequency
        instabilityAmount = random.nextFloat() * instabilityAmount; // instability amount equals a frequency value - 20Hz seems practical

        setDelayTimeInSamples(delayFreq + instabilityAmount); // Set delaytime in original function
    }

private:
    IIRFilter dampen;
    Random random;
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
    void setResonator(float freq, float q)  // Q of 0-100 seems useful
    {
        smoothQ.setTargetValue(q);
        float smoothedQ = smoothQ.getNextValue(); // Smoothed resonance value

        resonator.setCoefficients(IIRCoefficients::makeBandPass(getSamplerate(), freq, smoothedQ + 0.01)); // Make sure to have samplerate set

        resGain = 1 + (q / 10); // Define Gainstage to equally increase the Volume with rising Q Value
    }

    // ====== STORE VALUE AND READ CURRENT POSITION =======
    float process(float input) override
    {
        float noise = random.nextFloat() * 0.0001f; // Low level Background noise to excite self oscillation

        float outVal = readVal();
        outVal = resonator.processSingleSampleRaw(input + outVal) * resGain; // Multiply resonant filter by gainstage
        outVal = tanh(outVal); // Limit signal before feedback

        writeVal(getFeedback() * outVal); //Feedback

        outVal = tanh(outVal); // Limit output signal after feedback

        float floor(outVal); // Calculate interpolation
        return floor;    
    }

private:
    IIRFilter resonator;
    SmoothedValue<float> smoothQ;
    float resGain;

    Random random;
};

