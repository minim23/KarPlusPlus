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
#include "Feedback.h"
#include "ResonantFeedback.h"

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
        resFeedback.setup(sr);
        resFeedback.setSize(sr * 30);
    }

    // ====== SETUP FORMANTS =======
    void setFormants()
    {
        for (int i; i < formantAmount; i++)
        {
            formants.add(new IIRFilter()); //SETUP ARRAY OF FILTERS
        }
    }

    // ====== INITIALIZATE PARAMETER POINTERS =======
    void setParameterPointers(


    std::atomic<float>* dampIn,
    std::atomic<float>* tailIn,
    std::atomic<float>* instabilityIn,

    std::atomic<float>* delayTimeIn,
    std::atomic<float>* feedbackIn,
    std::atomic<float>* qIn,
    std::atomic<float>* noiseIn,

    std::atomic<float>* karplusVolIn,

    std::atomic<float>* attackIn,
    std::atomic<float>* decayIn,
    std::atomic<float>* sustainIn,
    std::atomic<float>* releaseIn)
    {
        dampAmount = dampIn;
        tailAmount = tailIn;
        instabilityAmount = instabilityIn;

        feedbackAmount = feedbackIn;
        delayTime = delayTimeIn;
        qAmount = qIn;
        noiseAmount = noiseIn;      

        karplusVolAmount = karplusVolIn;

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

        impulseEnv.reset();
        impulseEnv.noteOn();

        feedbackEnv.reset();
        feedbackEnv.noteOn();
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
        impulseEnv.noteOff();
        feedbackEnv.noteOff();

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
            
            // ====== ENVELOPE SETUP =======
            // ====== GLOBAL =======
            ADSR::Parameters envParams;
            envParams.attack = *attack;
            envParams.decay = *decay;
            envParams.sustain = *sustain;
            envParams.release = *release + 5000; // Make sure to end after feedback release
            env.setParameters(envParams);

            // ====== FEEDBACK =======
            ADSR::Parameters feedbackParams;
            feedbackParams.attack = *attack + 10; // Make sure to beginn after global start
            feedbackParams.decay = *decay;
            feedbackParams.sustain = *sustain - 0.1;
            feedbackParams.release = *release;
            feedbackEnv.setParameters(feedbackParams);

            // ====== IMPULSE =======
            ADSR::Parameters impulseParams;
            impulseParams.attack = 0.01f;
            impulseParams.decay = 0.1f;
            impulseParams.sustain = 0.0f;
            impulseParams.release = 0.1 + *noiseAmount;
            impulseEnv.setParameters(impulseParams);

            // ====== DSP LOOP =======
            for (int sampleIndex = startSample; sampleIndex < (startSample + numSamples); sampleIndex++)
            {   
                /*
                for (int i; i < formantAmount; i++)
                {
                    float q = random.nextFloat() + 0.01;
                    float freq = random.nextInt(6000) + 20;

                    formants[i]->setCoefficients(IIRCoefficients::makeBandPass(sr, freq, q));
                }*/
                
                // ====== ENVELOPES =======
                float envVal = env.getNextSample();
                float impulseVal = impulseEnv.getNextSample();
                float feedbackVal = feedbackEnv.getNextSample();
                
                // ====== IMPULSE =======
                float exciter = random.nextFloat() * impulseVal;
                
                // ====== KARPLUS STRONG PARAMERTS =======
                karplusStrong.setTail(*tailAmount);

                // ====== RESONANT FEEDBACK =======
                resFeedback.setDelayTimeInSamples(*delayTime * (sr/10) + 8000);
                resFeedback.setResonator(freq, *qAmount);
                resFeedback.setFeedback(*feedbackAmount * feedbackVal); // Trigger feedback by custom ASDR

                // ====== SAMPLE PROCESSING =======
                float currentSample = karplusStrong.process(exciter) * *karplusVolAmount
                                      + (resFeedback.process(karplusStrong.process(exciter), 0) * *feedbackAmount)
                                      * 0.3f            // Lower the volume
                                      * envVal;         // Multiply by global envelope 

                /*
                for (auto* formant : formants)
                {
                    formant.process(currentSample);
                }
                */
                /*
                // ====== FORMANT PROCESSING =======
                for (int i = 0; i < formantAmount; i++)
                {
                    currentSample = formants[i]->processSingleSampleRaw(currentSample);
                }           
                
                */

                // ====== LIMIT OUTPUT =======          
                currentSample = limiter.process(currentSample, 0.95f);
                                
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
    std::atomic<float>* delayTime;
    std::atomic<float>* qAmount;
    std::atomic<float>* noiseAmount;
    std::atomic<float>* feedbackAmount;

    std::atomic<float>* dampAmount;
    std::atomic<float>* tailAmount;
    std::atomic<float>* instabilityAmount;

    std::atomic<float>* karplusVolAmount;

    std::atomic<float>* attack;
    std::atomic<float>* decay;
    std::atomic<float>* sustain;
    std::atomic<float>* release;

    // ====== FILTER SMOOTHING ======= 
    

    Limiter limiter;

    // ====== KARPLUS STRONG =======   
    KarplusStrong karplusStrong;

    OwnedArray<IIRFilter> formants;
    int formantAmount = 32;

    Random random; // for White Noise

    BasicDelayLine delay;
    ResonantFeedback resFeedback;

    float freq; // Frequency of Synth
    float sr; // Samplerate

    // ENVELOPE SETUP
    ADSR env;
    ADSR impulseEnv; 
    ADSR feedbackEnv;


        //IIRFilter resonator, detunedResonator;
};

