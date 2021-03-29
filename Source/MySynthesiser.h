#pragma once
#include "FeedbackDelay.h"

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

    // ====== INITIALIZATE PARAMETER POINTERS =======
    void setParameterPointers(

    std::atomic<float>* karplusVolIn,
    std::atomic<float>* dampIn,
    std::atomic<float>* sustainIn,
    std::atomic<float>* tailIn,
    std::atomic<float>* instabilityIn,

    std::atomic<float>* feedbackIn,
    std::atomic<float>* delayTimeIn,
    std::atomic<float>* qIn,
    std::atomic<float>* feedbackAgeIn,
    std::atomic<float>* detuneIn,
    std::atomic<float>* offsetIn,

    std::atomic<float>* releaseIn,
    std::atomic<float>* volumeIn
    )
    {
        karplusVolAmount = karplusVolIn;
        dampAmount = dampIn;
        sustain = sustainIn;
        tailAmount = tailIn;
        instabilityAmount = instabilityIn;

        feedbackAmount = feedbackIn;
        delayTime = delayTimeIn;
        qAmount = qIn;
        feedbackAgeAmount = feedbackAgeIn;
        detuneAmount = detuneIn;
        offsetAmount = offsetIn;

        release = releaseIn;
        volume = volumeIn;
    }

    // ====== SETUP FORMANTS =======
    void setFormants()
    {       
        for (int i = 0; i < formantAmount; i++)
        {
            formantsLeft.add(new IIRFilter());      
            formantsRight.add(new IIRFilter());
        }
    }

    // ====== SAMPLERATE SETUP FOR PREPARE TO PLAY =======
    void init(int sampleRate)
    {
        sr = sampleRate;

        // ====== KARPLUS STRONG SETUP =======
        karplusStrongLeft.setSamplerate(sr);
        karplusStrongRight.setSamplerate(sr);
        karplusStrongLeft.setSize(sr * 10); // Practical delay size - possibly too big
        karplusStrongRight.setSize(sr * 10);

        // ====== RESONANT FEEDBACK SETUP =======
        resFeedbackLeft.setSamplerate(sr);
        resFeedbackLeft.resonatorSetup(); // Gets samplerate from function above - could possibly be simplified or improved
        resFeedbackLeft.setSize(sr * 30); // Practical delay size

        resFeedbackRight.setSamplerate(sr);
        resFeedbackRight.resonatorSetup();
        resFeedbackRight.setSize(sr * 30);

        // ====== VOLUME SMOOTHING SETUP =======
        smoothKarplusVol.reset(sr, 0.2f); // Smoothed value of 200ms
        smoothKarplusVol.setCurrentAndTargetValue(0.0);

        smoothFeedbackVol.reset(sr, 0.2f); // Smoothed value of 200ms
        smoothFeedbackVol.setCurrentAndTargetValue(0.0);

        globalVol.reset(sr, 0.02f); // Smoothed value of 20ms
        globalVol.setCurrentAndTargetValue(0.0);
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
        freq = MidiMessage::getMidiNoteInHertz(midiNoteNumber);

        // ====== DAMPENING =======
        karplusStrongLeft.setDampening(*dampAmount);
        karplusStrongRight.setDampening(*dampAmount);

        // ====== FORMANT COEFFICIENTS =======   
        for (int i = 0; i < formantAmount; i++)
        {
            float maxRandFreq = *dampAmount // Get relative value
                                * 1000 // Max. Freq for ranomization of filters
                                + 1; // Next Int can not be 0

            float maxFreq = *dampAmount // Get relative value
                            * 4000 // Rough max. Freq
                            + 100; // Absolute min. Freq

            formantsLeft[i]->setCoefficients(IIRCoefficients::makeBandPass(sr, random.nextInt(maxRandFreq) + maxFreq, (random.nextFloat() - 0.01) + 0.01));
            formantsRight[i]->setCoefficients(IIRCoefficients::makeBandPass(sr, random.nextInt(maxRandFreq) + maxFreq, (random.nextFloat() - 0.01) + 0.01));
        }

        // ====== TRIGGER ENVELOPES =======
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
        // ====== TRIGGER OFF ENVELOPES =======
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
            envParams.release = *release + 0.1f; // Make sure to end after feedback release
            env.setParameters(envParams);

            // ====== FEEDBACK =======
            ADSR::Parameters feedbackParams;
            feedbackParams.attack = 0.1;
            feedbackParams.decay = 0.25;
            feedbackParams.sustain = 0.9f;
            feedbackParams.release = *release + 0.01f;
            feedbackEnv.setParameters(feedbackParams);

            // ====== IMPULSE =======
            ADSR::Parameters impulseParams;
            impulseParams.attack = 0.01f;
            impulseParams.decay = 0.1f;
            impulseParams.sustain = *sustain * 0.3;
            impulseParams.release = *release + 0.01f;
            impulseEnv.setParameters(impulseParams);

            // ====== DSP LOOP =======
            for (int sampleIndex = startSample; sampleIndex < (startSample + numSamples); sampleIndex++)
            {               
                // ====== SMOOTHED VOLUMES =======
                smoothFeedbackVol.setTargetValue(*feedbackAmount); // Resonant Feedback Volume
                float smoothedFeedbackVol = smoothFeedbackVol.getNextValue();

                smoothKarplusVol.setTargetValue(*karplusVolAmount); // Karplus Strong Volume
                float smoothedKarplusVol = smoothKarplusVol.getNextValue();

                globalVol.setTargetValue(*volume); // Output Volume
                float smoothedGlobalVol = globalVol.getNextValue();

                // ====== ENVELOPE VALUES =======
                float envVal = env.getNextSample(); // Global envelope
                float impulseVal = impulseEnv.getNextSample(); // White Noise envelope
                float feedbackVal = feedbackEnv.getNextSample(); // Resonant Feedback envelope
               
                // ====== IMPULSE =======
                float exciterLeft = random.nextFloat() * impulseVal; // Enveloped White Noise
                float exciterRight = random.nextFloat() * impulseVal;

                // ====== FORMANTS =======
                float exColourLeft = 0.0f; // Setting up colouration - for some reason it sounded differently when exciter was assigned to formants directly
                float exColourRight = 0.0f;

                for (auto* formant : formantsLeft) // Process Impulse through array of randomized Bandpass Filters on left and right channel
                {
                     exColourLeft = formant->processSingleSampleRaw(exciterLeft);
                }
                for (auto* formant : formantsRight)
                {
                    exColourRight = formant->processSingleSampleRaw(exciterRight);
                }
                    
                exciterLeft = exColourLeft // Take formanted impulse
                              / formantAmount // Adjust volume relative to formant amount
                              * sqrt(formantAmount); // Adjust volume relative to resonance of filter - normally this should be square root of Q, but this did not seem practical
                                 
                exciterRight = exColourRight
                              / formantAmount
                              * sqrt(formantAmount);
                
                // ====== KARPLUS STRONG =======
                karplusStrongLeft.setPitch(freq, *instabilityAmount);
                karplusStrongRight.setPitch(freq + *detuneAmount, *instabilityAmount); // Detuning between left and right channel

                karplusStrongLeft.setFeedback(*tailAmount); // Feedback between 0-1
                karplusStrongRight.setFeedback(*tailAmount);

                // ====== RESONANT FEEDBACK =======              
                float age = 1 - (*feedbackAgeAmount * random.nextFloat()); // Feedback Colour (Randomization of Delaytime)
                float delayt = (*delayTime * (sr / 20)) * age; // Delaytime set to an arbitrary, functional value - initially this was set to get ms - multiplied by Feedback Colour 
                float offset = *offsetAmount * (freq / 2); // Phasing up to 180*

                resFeedbackLeft.setDelayTimeInSamples(delayt + 100 + offset); // Slight offset of left and right channel with control of phasing
                resFeedbackLeft.setResonator(freq, *qAmount);
                resFeedbackLeft.setFeedback(*feedbackAmount * feedbackVal); // Envelope controlling Feedback amount relative to maximum volume

                resFeedbackRight.setDelayTimeInSamples(delayt + 100);
                resFeedbackRight.setResonator(freq + *detuneAmount, *qAmount); // Detuning between left and right channel
                resFeedbackRight.setFeedback(*feedbackAmount * feedbackVal);
                
                // ====== SAMPLE PROCESSING CHAIN =======
                float currentSampleLeft = karplusStrongLeft.kSProcess(exciterLeft) * smoothedKarplusVol // Karplus Strong Volume
                                        + (resFeedbackLeft.process(karplusStrongLeft.process(exciterRight)) * smoothedFeedbackVol); // Resonant Feedback Volume - KS inserted

                float currentSampleRight = karplusStrongRight.kSProcess(exciterRight) * smoothedKarplusVol
                                           + (resFeedbackRight.process(karplusStrongRight.process(exciterLeft)) * smoothedFeedbackVol);
                
                // ====== GLOBAL ENVELOPE AND VOLUME =======
                currentSampleLeft = currentSampleLeft
                                    * envVal // Global envelope
                                    * smoothedGlobalVol * smoothedGlobalVol * smoothedGlobalVol; // Exponential output control for volume adjustment
                currentSampleRight = currentSampleRight
                                    * envVal
                                    * smoothedGlobalVol * smoothedGlobalVol * smoothedGlobalVol;
                                
                // ====== CHANNEL ASSIGNMENT =======
                for (int chan = 0; chan < outputBuffer.getNumChannels(); chan++)
                {
                    outputBuffer.addSample(0, sampleIndex, currentSampleLeft); // Left Channel
                    outputBuffer.addSample(1, sampleIndex, currentSampleRight); // Right Channel
                }

                // ====== END SOUND =======
                if (ending)
                {
                    if (envVal < 0.0001f) // Clear note if envelope value is very low
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
    std::atomic<float>* karplusVolAmount;

    std::atomic<float>* dampAmount;
    std::atomic<float>* room;
    std::atomic<float>* sustain;
    std::atomic<float>* tailAmount;
    std::atomic<float>* instabilityAmount;

    std::atomic<float>* feedbackAmount;
    std::atomic<float>* delayTime;
    std::atomic<float>* qAmount;
    std::atomic<float>* feedbackAgeAmount;
    std::atomic<float>* detuneAmount;
    std::atomic<float>* offsetAmount;

    std::atomic<float>* release;
    std::atomic<float>* volume;

    // ====== ENVELOPES =======   
    ADSR env, impulseEnv, feedbackEnv;

    // ====== IMPULSE =======   
    Random random; // for White Noise
    OwnedArray<IIRFilter> formantsLeft, formantsRight;
    int formantAmount = 20;

    // ====== KARPLUS STRONG =======   
    SmoothedValue<float> smoothKarplusVol;
    KarplusStrong karplusStrongLeft, karplusStrongRight;

    // ====== FEEDBACK RESONATOR =======   
    SmoothedValue<float> smoothFeedbackVol;
    ResonantFeedback resFeedbackLeft, resFeedbackRight;

    // ====== GLOBAL VOLUME =======   
    SmoothedValue<float> globalVol;

    // ====== UTILITY =======   
    float freq; // Frequency of Synth
    float sr; // Samplerate
};

