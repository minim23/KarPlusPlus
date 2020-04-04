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
#include "KarplusStrong.h"

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

        karplusStrong.setup(sr); // Send samplerate values to Karplus Strong

        feedbacker1.setSize(sr * 10);
        feedbacker2.setSize(sr * 10);

    }

    // ====== INITIALIZATE PARAMETER POINTERS =======
    void setParameterPointers(
    std::atomic<float>* detuneIn, 

    std::atomic<float>* dampIn,
    std::atomic<float>* tailIn,
    std::atomic<float>* instabilityIn,

    std::atomic<float>* qIn,

    std::atomic<float>* feedbackIn,

    std::atomic<float>* attackIn,
    std::atomic<float>* decayIn,
    std::atomic<float>* sustainIn,
    std::atomic<float>* releaseIn)
    {
        beatAmount = detuneIn;

        dampAmount = dampIn,
        tailAmount = tailIn,
        instabilityAmount = instabilityIn,

        qAmount = qIn;

        feedbackAmount = feedbackIn;

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

            // ====== SYNTHESISER NOTE SETUP =======
            subSynth.setFilter(sr, freq + /**beatAmount,*/ *instabilityAmount, smoothedQ);
            karplusStrong.setDelaytime(freq, /**beatAmount,*/ *instabilityAmount);

            // ====== FEEDBACK SETUP =======
            feedbacker1.setDelayTimeInSamples(8000 + (sr / *beatAmount));
            feedbacker2.setDelayTimeInSamples(8000 + (sr / *beatAmount));

            feedbacker1.setFeedback(*feedbackAmount);
            feedbacker2.setFeedback(*feedbackAmount);
            
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
                float envVal = env.getNextSample(); // Envelope Calculation
                
                // ====== KARPLUS STRONG PARAMERTS =======
                karplusStrong.setTail(*tailAmount);
                karplusStrong.setDampening(*dampAmount);
           
                // ====== WHITE NOISE =======
                float currentSample = random.nextFloat();

                // ====== SAMPLE PROCESSING =======
                currentSample = (karplusStrong.process(currentSample + tanh(feedbacker2.process((subSynth.process() * smoothedQ) * *feedbackAmount))) //Karplus Strong
                                +(subSynth.process(currentSample + tanh(feedbacker1.process(karplusStrong.process(currentSample) * *feedbackAmount)) ) * smoothedQ)) //Subtractive Synth with adjusted Volume relative to Q Amount in Filter
                                * 0.5f        // Half the Volume
                                * 0.1f        // Output Volume                                     
                                * envVal;     // Multiply with Envelope

                // ====== WAVESHAPING LIMITER =======
                currentSample = limiter.process(currentSample, 1.0f);
                                
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

    std::atomic<float>* dampAmount;
    std::atomic<float>* tailAmount;
    std::atomic<float>* instabilityAmount;

    std::atomic<float>* qAmount;

    std::atomic<float>* feedbackAmount;

    std::atomic<float>* attack;
    std::atomic<float>* decay;
    std::atomic<float>* sustain;
    std::atomic<float>* release;

    // ====== FILTER SMOOTHING ======= 
    SmoothedValue<float, ValueSmoothingTypes::Linear> smoothQ;

    Limiter limiter;

    BasicDelayLine feedbacker1, feedbacker2;

    // ====== SUBTRACTIVE SYNTH =======   
    SubtractiveSynth subSynth;

    // ====== KARPLUS STRONG =======   
    KarplusStrong karplusStrong;


    Random random; // for White Noise

    BasicDelayLine delay;

    float freq; // Frequency of Synth
    float sr; // Samplerate

    ADSR env; // JUCE ADSR Envelope


        //IIRFilter resonator, detunedResonator;
};

