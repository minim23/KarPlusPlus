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
    void init(int sampleRate)
    {
        sr = sampleRate;

        karplusStrong.setup(sr);
        
        feedback.setSize(sr * 10);

    }

    // ====== INITIALIZATE PARAMETER POINTERS =======
    void setParameterPointers(
    std::atomic<float>* detuneIn, 

    std::atomic<float>* dampIn,
    std::atomic<float>* tailIn,
    std::atomic<float>* instabilityIn,

    std::atomic<float>* feedbackIn,

    std::atomic<float>* attackIn,
    std::atomic<float>* decayIn,
    std::atomic<float>* sustainIn,
    std::atomic<float>* releaseIn)
    {
        beatAmount = detuneIn;

        dampAmount = dampIn;
        tailAmount = tailIn;
        instabilityAmount = instabilityIn;

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

        // ====== MIDI TO FREQ =======
        freq = MidiMessage::getMidiNoteInHertz(midiNoteNumber); // Get freq from Midi

        karplusStrong.setDampening(*dampAmount);

        env.reset(); // clear out envelope before re-triggering it
        env.noteOn(); // start envelope

        impulse.reset();
        impulse.noteOn();
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
        impulse.noteOff();

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

            // ====== SYNTHESISER NOTE SETUP =======
            karplusStrong.setDelaytime(freq, *instabilityAmount);

            // ====== FEEDBACK SETUP =======
            feedback.setDelayTimeInSamples(sr / *beatAmount + 1);
            feedback.setFeedback(0.999);
            
            // ====== ENVELOPE SETUP =======
            ADSR::Parameters envParams;
            envParams.attack = *attack;
            envParams.decay = *decay;
            envParams.sustain = *sustain;
            envParams.release = *release;
            env.setParameters(envParams);

            ADSR::Parameters impulseParams;
            impulseParams.attack = 0.01f;
            impulseParams.decay = 0.1f;
            impulseParams.sustain = 0.0f;
            impulseParams.release = 0.1;
            impulse.setParameters(impulseParams);

            // ====== DSP LOOP =======
            for (int sampleIndex = startSample; sampleIndex < (startSample + numSamples); sampleIndex++)
            {                               
                // ====== ENVELOPES =======
                float envVal = env.getNextSample(); // Envelope Calculation
                float impulseVal = impulse.getNextSample(); // Impulse Calculation
                
                // ====== KARPLUS STRONG PARAMERTS =======
                karplusStrong.setTail(*tailAmount);
           
                // ====== IMPULSE =======
                float exciter = random.nextFloat() * impulseVal;

                // ====== SAMPLE PROCESSING =======
                float currentSample = karplusStrong.process(exciter) //Karplus Strong + Feedback
                                      + feedback.process(currentSample * *feedbackAmount)
                                      * 0.5f        // Half the Volume
                                      * 0.1f
                                      * envVal;        // Output Volume        

                // ====== WAVESHAPING LIMITER =======
                //currentSample = tanh(currentSample);                
                //currentSample = limiter.process(currentSample, 0.95f);
                                
                // ====== CHANNEL ASSIGNMENT =======
                for (int chan = 0; chan < outputBuffer.getNumChannels(); chan++)
                {
                    outputBuffer.addSample(chan, sampleIndex, currentSample); 
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
    // ====== NOTE ON/OFF =======   
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
    

    Limiter limiter;

    BasicDelayLine feedback;

    // ====== KARPLUS STRONG =======   
    KarplusStrong karplusStrong;

    Random random; // for White Noise

    BasicDelayLine delay;

    float freq; // Frequency of Synth
    float sr; // Samplerate

    ADSR env; // JUCE ADSR Envelope
    ADSR impulse; // JUCE ADSR Envelope

        //IIRFilter resonator, detunedResonator;
};

