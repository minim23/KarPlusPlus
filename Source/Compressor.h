/*
  ==============================================================================

    Compressor.h
    Created: 10 Apr 2020 10:03:02am
    Author:  simon

  ==============================================================================
*/

#pragma once
#include <cmath>

class Compressor
{

};

void compress
(
    float* wav_in,     // signal
    int     n,          // N samples
    double  threshold,  // threshold (percents)
    double  slope,      // slope angle (percents)
    int     sr,         // sample rate (smp/sec)
    double  tla,        // lookahead  (ms)
    double  twnd,       // window time (ms)
    double  tatt,       // attack time  (ms)
    double  trel        // release time (ms)
)
{
    typedef float   stereodata[2];
    stereodata* wav = (stereodata*)wav_in; // our stereo signal
    threshold *= 0.01;          // threshold to unity (0...1)
    slope *= 0.01;              // slope to unity
    tla *= 1e-3;                // lookahead time to seconds
    twnd *= 1e-3;               // window time to seconds
    tatt *= 1e-3;               // attack time to seconds
    trel *= 1e-3;               // release time to seconds

    // attack and release "per sample decay"
    double  att = (tatt == 0.0) ? (0.0) : exp(-1.0 / (sr * tatt));
    double  rel = (trel == 0.0) ? (0.0) : exp(-1.0 / (sr * trel));

    // envelope
    double  env = 0.0;

    // sample offset to lookahead wnd start
    int     lhsmp = (int)(sr * tla);

    // samples count in lookahead window
    int     nrms = (int)(sr * twnd);

    // for each sample...
    for (int i = 0; i < n; ++i)
    {
        // now compute RMS
        double  summ = 0;

        // for each sample in window
        for (int j = 0; j < nrms; ++j)
        {
            int     lki = i + j + lhsmp;
            double  smp;

            // if we in bounds of signal?
            // if so, convert to mono
            if (lki < n)
                smp = 0.5 * wav[lki][0] + 0.5 * wav[lki][1];
            else
                smp = 0.0;      // if we out of bounds we just get zero in smp

            summ += smp * smp;  // square em..
        }

        double  rms = sqrt(summ / nrms);   // root-mean-square

        // dynamic selection: attack or release?
        double  theta = rms > env ? att : rel;

        // smoothing with capacitor, envelope extraction...
        // here be aware of pIV denormal numbers glitch
        env = (1.0 - theta) * rms + theta * env;

        // the very easy hard knee 1:N compressor
        double  gain = 1.0;
        if (env > threshold)
            gain = gain - (env - threshold) * slope;

        // result - two hard kneed compressed channels...
        float  leftchannel = wav[i][0] * gain;
        float  rightchannel = wav[i][1] * gain;
    }
}