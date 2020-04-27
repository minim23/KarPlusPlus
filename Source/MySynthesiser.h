/*
  ==============================================================================

    MySynthesiser.h
    Created: 7 Mar 2020 4:27:57pm
    Author:  Tom Mudd

  ==============================================================================
*/

#pragma once
#include "BasicDelayLine.h"
#include "Limiter.h"
#include "KarplusStrong.h"
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

    // ====== SETUP FORMANTS =======
    void setFormants()
    {
        /*
        for (int i; i < formantAmount; i++)
        {
            formants1.add(new IIRFilter());
            //formants2.add(new IIRFilter());
            //impulseColour.add(new IIRFilter());
        }
        */
    }
    
    // ====== PREPARE TO PLAY =======
    void init(int sampleRate)
    {
        sr = sampleRate;

        // ====== KARPLUS STRONG =======
        karplusStrongLeft.setup(sr);
        karplusStrongRight.setup(sr);

        smoothKarplusVol.reset(sr, 0.2f); // Set samplerate and smoothing of 200ms
        smoothKarplusVol.setCurrentAndTargetValue(0.0); // will be overwritten

        // ====== RESONANT FEEDBACK =======
        resFeedbackLeft.setup(sr);
        resFeedbackLeft.setSize(sr * 30);

        resFeedbackRight.setup(sr);
        resFeedbackRight.setSize(sr * 30);

        smoothFeedbackVol.reset(sr, 0.2f); // Set samplerate and smoothing of 200ms
        smoothFeedbackVol.setCurrentAndTargetValue(0.0); // will be overwritten

        /*
        for (int i = 0; i < formantAmount; i++)
        {
            float formantQ1 = random.nextFloat() + 0.01 * 100;
            //formantQ2 = random.nextFloat() + 0.01;
            //impulseColourQ = random.nextFloat() + 0.01;

            float formantFreq1 = random.nextInt(1000) + 20;
            //formantFreq2 = random.nextInt(*attack) + 20;
            //impulseFreq = random.nextInt(*decay) + 20;

            formants1[i]->setCoefficients(IIRCoefficients::makeBandPass(sr, formantFreq1, formantQ1));
            //formants2[i]->setCoefficients(IIRCoefficients::makeBandPass(sr, formantFreq2, formantQ2));
            //impulseColour[i]->setCoefficients(IIRCoefficients::makeBandPass(sr, impulseFreq, impulseColourQ));
        }
        */
    }

    // ====== INITIALIZATE PARAMETER POINTERS =======
    void setParameterPointers(

        std::atomic<float>* karplusVolIn,
    std::atomic<float>* dampIn,
    std::atomic<float>* tailIn,
    std::atomic<float>* instabilityIn,

    std::atomic<float>* feedbackIn,
    std::atomic<float>* delayTimeIn,
    std::atomic<float>* qIn,
    std::atomic<float>* noiseIn,

    std::atomic<float>* detuneIn,

    std::atomic<float>* attackIn,
    std::atomic<float>* decayIn,
    std::atomic<float>* sustainIn,
    std::atomic<float>* releaseIn)
    {
        karplusVolAmount = karplusVolIn;
        dampAmount = dampIn;
        tailAmount = tailIn;
        instabilityAmount = instabilityIn;

        feedbackAmount = feedbackIn;
        delayTime = delayTimeIn;
        qAmount = qIn;
        noiseAmount = noiseIn;    

        detuneAmount = detuneIn;

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

        // ====== DAMPENING =======
        karplusStrongLeft.setDampening(*dampAmount);
        karplusStrongRight.setDampening(*dampAmount);

        // ====== ENVELOPE RESET =======
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
            // ====== ENVELOPE SETUP =======
            // ====== GLOBAL =======
            ADSR::Parameters envParams;
            envParams.attack = 0.1;
            envParams.decay = 0.25;
            envParams.sustain = 1.0f;
            envParams.release = *release + 1.0; // Make sure to end after feedback release
            env.setParameters(envParams);

            // ====== FEEDBACK =======
            ADSR::Parameters feedbackParams;
            feedbackParams.attack = 0.1;
            feedbackParams.decay = 0.25;
            feedbackParams.sustain = 0.9f;
            feedbackParams.release = *release;
            feedbackEnv.setParameters(feedbackParams);

            // ====== IMPULSE =======
            ADSR::Parameters impulseParams;
            impulseParams.attack = 0.01f;
            impulseParams.decay = 0.1f;
            impulseParams.sustain = 0.1f;
            impulseParams.release = 0.1;
            impulseEnv.setParameters(impulseParams);

            // ====== DSP LOOP =======
            for (int sampleIndex = startSample; sampleIndex < (startSample + numSamples); sampleIndex++)
            {                   
                // ====== ENVELOPES =======
                float envVal = env.getNextSample();
                float impulseVal = impulseEnv.getNextSample();
                float feedbackVal = feedbackEnv.getNextSample();
               
                // ====== IMPULSE =======
                float exciter = random.nextFloat() * impulseVal;
                
                // ====== KARPLUS STRONG =======
                smoothKarplusVol.setTargetValue(*karplusVolAmount);
                float smoothedKarplusVol = smoothKarplusVol.getNextValue();

                karplusStrongLeft.setDelaytime(freq, *instabilityAmount);
                karplusStrongRight.setDelaytime(freq + *detuneAmount, *instabilityAmount);
                karplusStrongLeft.setTail(*tailAmount);
                karplusStrongRight.setTail(*tailAmount);

                // ====== RESONANT FEEDBACK =======
                //float age = 1 - (*noiseAmount * random.nextFloat() * 20);

                smoothFeedbackVol.setTargetValue(*feedbackAmount);
                float smoothedFeedbackVol = smoothFeedbackVol.getNextValue();

                float delayt = (*delayTime * (sr / 10) + 100);

                resFeedbackLeft.setDelayTimeInSamples(delayt);
                resFeedbackLeft.setResonator(freq, *qAmount);
                resFeedbackLeft.setFeedback(*feedbackAmount * feedbackVal);

                resFeedbackRight.setDelayTimeInSamples(delayt + 100);
                resFeedbackRight.setResonator(freq + *detuneAmount, *qAmount);
                resFeedbackRight.setFeedback(*feedbackAmount * feedbackVal);
                
                // ====== SAMPLE PROCESSING =======
                float currentSampleLeft = karplusStrongLeft.process(exciter) * smoothedKarplusVol
                                    + (resFeedbackLeft.process(karplusStrongLeft.process(exciter)) * smoothedFeedbackVol)
                                    * gain;            

                float currentSampleRight = karplusStrongRight.process(exciter) * smoothedKarplusVol
                                    + (resFeedbackRight.process(karplusStrongRight.process(exciter)) * smoothedFeedbackVol)
                                    * gain;            


                // ====== FORMANTS =======
                /*
                for (int i; i < formantAmount; i++)
                {
                    formants1[i]->processSingleSampleRaw(currentSampleLeft);
                }
                
                
                for (auto* formant : formants1)
                {
                    formant->processSingleSampleRaw(currentSampleLeft); 
                }

                
                for (auto* formant : formants2)
                {
                    formant->processSingleSampleRaw(currentSampleRight);
                }*/

                // ====== GLOBAL ENVELOPE =======
                currentSampleLeft = currentSampleLeft * envVal;
                currentSampleRight = currentSampleRight * envVal;

                // ====== LIMIT OUTPUT =======          
                currentSampleLeft = limiter.process(currentSampleLeft, 0.95f);
                currentSampleRight = limiter.process(currentSampleRight, 0.95f);
                                
                // ====== CHANNEL ASSIGNMENT =======
                for (int chan = 0; chan < outputBuffer.getNumChannels(); chan++)
                {
                    outputBuffer.addSample(0, sampleIndex, currentSampleLeft);
                    outputBuffer.addSample(1, sampleIndex, currentSampleRight);
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

    std::atomic<float>* detuneAmount;

    std::atomic<float>* karplusVolAmount;

    std::atomic<float>* attack;
    std::atomic<float>* decay;
    std::atomic<float>* sustain;
    std::atomic<float>* release;

    // ====== ENVELOPES =======   
    ADSR env, impulseEnv, feedbackEnv;

    // ====== KARPLUS STRONG =======   
    SmoothedValue<float> smoothKarplusVol;
    KarplusStrong karplusStrongLeft, karplusStrongRight;

    // ====== FEEDBACK RESONATOR =======   
    SmoothedValue<float> smoothFeedbackVol;
    ResonantFeedback resFeedbackLeft, resFeedbackRight;

    // ====== FORMANTS =======   
    OwnedArray<IIRFilter> formants1;
    int formantAmount = 32;

    // ====== GLOBAL VOLUME =======   
    Limiter limiter;
    float gain = 0.3f;

    // ====== UTILITY =======   
    Random random; // for White Noise

    float freq; // Frequency of Synth
    float sr; // Samplerate
};

