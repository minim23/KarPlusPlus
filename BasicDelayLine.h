/*
  ==============================================================================

    BasicDelayLine.h
    Created: 28 Feb 2020 3:44:03pm
    Author:  Tom Mudd

  ==============================================================================
*/

#pragma once

#include <cmath>

/// basic delay line with linear interpolation
class BasicDelayLine
{
public:
    
    /// Destructor - called when the object is destroyed
    ~BasicDelayLine()
    {
        delete buffer;      // clean up when the object is destroyed to avoid memory leaks
    }
    
    /// store a value in the delay and retrieve the sample at the current read position
    float process(float inSamp)
    {
        float outVal = readVal();
        writeVal(inSamp + feedback * outVal);     // note the feedback here, scaling the output back in to the delay
        float floor(outVal); // calculates interpolation
        return floor;
    }
    
    /// read a value from the buffer at the read head position, then increment and wrap the readPos
    float readVal()
    {
        float outVal =  buffer[readPos];
        readPos ++;
        readPos %= size;
        return outVal;
    }
    
    /// write a value to the buffer at the write head position, then increment and wrap the writePos
    void writeVal(float inSamp)
    {
        buffer[writePos] = inSamp;
        writePos ++;
        writePos %= size;
    }
    
    /// set the actual delay time in samples
    void setDelayTimeInSamples(int delTime)
    {
        delayTimeInSamples = delTime;
        readPos = writePos - delayTimeInSamples;
        while (readPos < 0)
            readPos += size;
    }

    float getDelayTimeInSamples()
    {
        return delayTimeInSamples;
    }
    
    /// initialise the float array to a given maximum size (your specified delay time must always be less than this)
    void setSize(int newSize)
    {
        size = newSize;
        buffer = new float[size];
        for (int i=0; i<size; i++)
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
    
    
private:
    float* buffer;              // the actual buffer - not yet initialised to a size
    int size;                   // buffer size in samples
    
    int writePos = 0;            // write head location - should always be 0 ≤ writePos < size
    int readPos = 0.0f;        // read head location - should always be 0 ≤ readPos < size
    
    int delayTimeInSamples;
    
    float feedback = 0.0f;       // how much of the delay line to mix with the input?
    
};
