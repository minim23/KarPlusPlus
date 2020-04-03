/*
  ==============================================================================

    MySynthesiser.h
    Created: 7 Mar 2020 4:27:57pm
    Author:  Tom Mudd

  ==============================================================================
*/

#pragma once
#include "Oscillators.h"
#include "BasicDelayLine.h"
#include "Limiter.h"
#include "SubtractiveSynth.h"

// ===========================
// ===========================
// SOUND
class MySynthSound : public SynthesiserSound
{
public:
    bool appliesToNote      (int) override      { return true; }
    //--------------------------------------------------------------------------
    bool appliesToChannel   (int) override      { return true; }
};




// =================================
// =================================
// Synthesiser Voice - My synth code goes in here

/*!
 @class MySynthVoice
 @abstract struct defining the DSP associated with a specific voice.
 @discussion multiple MySynthVoice objects will be created by the Synthesiser so that it can be played polyphicially
 
 @namespace none
 @updated 2019-06-18
 */
class MySynthVoice : public SynthesiserVoice
{
public:
    MySynthVoice() {}

    // ====== INITIALIZATION FOR PREPARE TO PLAY =======
    void init(float sampleRate)
    {
        sr = sampleRate;

        // ====== Q FILTER SMOOTHING =======
        smoothQ.reset(sr, 0.002f); // Set samplerate and smoothing of 20ms
        smoothQ.setCurrentAndTargetValue(*qAmount);

        //limiter.Setup(20, 500, sampleRate);

        delay.setSize(sr * 10); // Set maxium size of Delay to 10 seconds which should be sufficient to play even the lowest frequencies
    }

    // ====== INITIALIZATE PARAMETER POINTERS =======
    void setParameterPointers(
    std::atomic<float>* detuneIn, 
    std::atomic<float>* qIn,
    std::atomic<float>* attackIn,
    std::atomic<float>* decayIn,
    std::atomic<float>* sustainIn,
    std::atomic<float>* releaseIn)
    {
        beatAmount = detuneIn;
        qAmount = qIn;

        attack = attackIn;
        decay = decayIn;
        sustain = sustainIn;
        release = releaseIn;
    }
    
    //--------------------------------------------------------------------------
    /**
     What should be done when a note starts

     @param midiNoteNumber
     @param velocity
     @param SynthesiserSound unused variable
     @param / unused variable
     */
    void startNote(int midiNoteNumber, float velocity, SynthesiserSound*, int /*currentPitchWheelPosition*/) override
    {
        playing = true;
        ending = false;

        // ====== MIDI TO DELAYTIME =======
        freq = MidiMessage::getMidiNoteInHertz(midiNoteNumber); // Get freq from Midi
        float delayFreq = sr / freq; // Get delaytime from freq
        delay.setDelayTimeInSamples(delayFreq); // Set delaytime to same amount


        //osc.setFrequency(freq);

        env.reset(); // clear out envelope before re-triggering it
        env.noteOn(); // start envelope
    }
    //--------------------------------------------------------------------------
    /// Called when a MIDI noteOff message is received
    /**
     What should be done when a note stops

     @param / unused variable
     @param allowTailOff bool to decie if the should be any volume decay
     */
    void stopNote(float /*velocity*/, bool allowTailOff) override
    {
        env.noteOff();
        ending = true;
    }

    //--------------------------------------------------------------------------
    /**
     The Main DSP Block: Put My DSP code in here

     If the sound that the voice is playing finishes during the course of this rendered block, it must call clearCurrentNote(), to tell the synthesiser that it has finished

     @param outputBuffer pointer to output
     @param startSample position of first sample in buffer
     @param numSamples number of smaples in output buffer
     */

     // ====== DSP BLOCK =======
    void renderNextBlock(AudioSampleBuffer& outputBuffer, int startSample, int numSamples) override
    {
        if (playing) // check to see if this voice should be playing
        {
            // ====== SMOOTHING Q =======
            smoothQ.setTargetValue(*qAmount);
            float smoothedQ = smoothQ.getNextValue();

            // ====== RESONATOR SETUP =======
            //resonator.setCoefficients(IIRCoefficients::makeBandPass(sr, freq, smoothedQ));
            //detunedResonator.setCoefficients(IIRCoefficients::makeBandPass(sr, freq + *beatAmount, smoothedQ));
            subSynth.setFilter(sr, freq + *beatAmount, smoothedQ);
            
            // ====== ENVELOPE SETUP =======
            ADSR::Parameters envParams;
            envParams.attack = *attack;
            envParams.decay = *decay;
            envParams.sustain = *sustain;
            envParams.release = *release;
            env.setParameters(envParams);

            // ====== DSP LOOP =======
            for (int sampleIndex = startSample; sampleIndex < (startSample + numSamples); sampleIndex++)
            {
                delay.setFeedback(0.99f);

                float envVal = env.getNextSample(); // Envelope Calculation

                float currentSample = random.nextFloat(); // White Noise

             
                // ====== SAMPLE PROCESSING =======
                currentSample = (
                                 delay.process(currentSample)
                                +(subSynth.process() * smoothedQ) //Adjust Volume relative to Q Amount in Filter
                                 )
                                * 0.5f        // Half the Volume
                                * 0.2f        // Output Volume
                                      
                                * envVal;     // Multiply with Envelope

                currentSample = limiter.process(currentSample); // Waveshaping Limiter
                                
                // ====== CHANNEL ASSIGNMENT =======
                for (int chan = 0; chan < outputBuffer.getNumChannels(); chan++)
                {
                    outputBuffer.addSample(
                                          chan, sampleIndex, 
                                          currentSample 
                                          ); 
                }
                // ====== END SOUND =======
                if (ending)
                {
                    if (envVal < 0.0001f) // Clear note if envelope value is very small
                    {
                        clearCurrentNote();
                        playing = false;
                    }
                }
            }
                    
        }
    }

    //--------------------------------------------------------------------------
    void pitchWheelMoved(int) override {}
    //--------------------------------------------------------------------------
    void controllerMoved(int, int) override {}
    //--------------------------------------------------------------------------
    /**
     Can this voice play a sound. I wouldn't worry about this for the time being

     @param sound a juce::SynthesiserSound* base class pointer
     @return sound cast as a pointer to an instance of MySynthSound
     */
    bool canPlaySound(SynthesiserSound* sound) override
    {
        return dynamic_cast<MySynthSound*> (sound) != nullptr;
    }
    //--------------------------------------------------------------------------
private:
    // ====== SETUP TO INDICATE IF A NOTE IS ON OR OFF =======   
    bool playing = false;
    bool ending = false;

    // ====== PARAMETER VALUES ======= 
    std::atomic<float>* beatAmount;
    std::atomic<float>* qAmount;

    std::atomic<float>* attack;
    std::atomic<float>* decay;
    std::atomic<float>* sustain;
    std::atomic<float>* release;

    // ====== FILTER SMOOTHING ======= 
    SmoothedValue<float, ValueSmoothingTypes::Linear> smoothQ;

    Limiter limiter;


    // ====== SUBTRACTIVE SYNTH =======   
    SubtractiveSynth subSynth;

    IIRFilter resonator, detunedResonator;
    Random random; // for White Noise

    BasicDelayLine delay;

    float freq; // Frequency of Synth

    float sr; // Samplerate

    ADSR env; // JUCE ADSR Envelope



};

