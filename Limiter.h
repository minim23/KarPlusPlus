/*
  ==============================================================================

    Limiter.h
    Created: 3 Apr 2020 11:58:19am
    Author:  simon

  ==============================================================================
*/

#pragma once
#include <cmath>

class Limiter
{
public:
    float process(float input)
    {
        if (input > 1)
            tanh(input);
        else if (input <-1)
            tanh(input);
        return input;
    }
private:

};

/*

class EnvelopeFollower
{
public:
    EnvelopeFollower();
    {
        envelope = 0;
    }

    void Setup(double attackMs, double releaseMs, int sampleRate);
    {
        a = pow(0.01, 1.0 / (attackMs * sampleRate * 0.001));
        r = pow(0.01, 1.0 / (releaseMs * sampleRate * 0.001));
    }


    /*
    template<class T, int skip>
    void Process(size_t count, const T* src);

    double envelope;

protected:
    double a;
    double r;*/
};

//----------

inline EnvelopeFollower::EnvelopeFollower()


inline void EnvelopeFollower::Setup(double attackMs, double releaseMs, int sampleRate)


template<class T, int skip>
void EnvelopeFollower::Process(size_t count, const T* src)
{
    while (count--)
    {
        double v = ::fabs(*src);
        src += skip;
        if (v > envelope)
            envelope = a * (envelope - v) + v;
        else
            envelope = r * (envelope - v) + v;
    }
}

//----------*/