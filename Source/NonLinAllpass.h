#pragma once
#include "FeedbackDelay.h"

class NonLinearAllpass
{
public:
    // ====== NONLINEAR ALLPASS =======
    void setCoefficients(float coeffA1, float coeffA2) // Takes values between -1 and 1
    {
        a1 = coeffA1;
        a2 = coeffA2;
    }
    
private:
    // ====== COEFFICIENTS =======
    float a1;
    float a2;
};


