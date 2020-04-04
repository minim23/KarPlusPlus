/*
  ==============================================================================

    Limiter.h
    Created: 3 Apr 2020 11:58:19am
    Author:  simon

  ==============================================================================
*/

#pragma once
#include <cmath>
#include <math.h>

class Limiter
{
public:
    float process(float input, float dist)
    {
        if (input > 1)
            tanh(dist * input);
        else if (input <-1)
            tanh(dist * input);

        return input;
    }
private:

};