#pragma once
#include "FeedbackDelay.h"

class NonLinearAllpass
{
public:
    // ====== COEFFICIENTS =======
    void setCoefficients (float coeffA, float coeffB) // Takes values between -1 and 1
    {
        coeffA1 = coeffA;
        coeffA2 = coeffB;
    }

    // ====== NON-LINEAR ALLPASS =======
    float process (float input)
    {
        float aIn = input + oldy;
        
        if (aIn >= 0)
            aIn = 0;
        else
            aIn = 1;
        
        coeffA1 = coeffA1 * (aIn * -1);
        coeffA2 = coeffA2 * aIn * -1;
        
        float c1 = coeffA1;
        float c2 = coeffA2;
        
        float output = c1 * input + oldx - c2 * oldy;
        oldx = input;
        oldy = output;
        
        return oldy;
    }
    
private:
    // ====== COEFFICIENTS =======
    float coeffA1 { 0.4 } ;
    float coeffA2 { -0.5 };
    
    Delay sampleDelay;
    
    float oldy = 0.0f;
    float oldx = 0.0f;
};


