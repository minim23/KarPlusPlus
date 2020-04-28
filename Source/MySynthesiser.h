#pragma once
#include "FeedbackDelay.h"
#include "Limiter.h"

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
    std::atomic<float>* roomIn,
    std::atomic<float>* sustainIn,
    std::atomic<float>* tailIn,
    std::atomic<float>* instabilityIn,

    std::atomic<float>* feedbackIn,
    std::atomic<float>* delayTimeIn,
    std::atomic<float>* qIn,
    std::atomic<float>* feedbackAgeIn,

    std::atomic<float>* detuneIn,
    std::atomic<float>* releaseIn)
    {
        karplusVolAmount = karplusVolIn;
        dampAmount = dampIn;
        room = roomIn;
        sustain = sustainIn;
        tailAmount = tailIn;
        instabilityAmount = instabilityIn;

        feedbackAmount = feedbackIn;
        delayTime = delayTimeIn;
        qAmount = qIn;
        feedbackAgeAmount = feedbackAgeIn;

        detuneAmount = detuneIn;
        release = releaseIn;
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
        karplusStrongLeft.setSize(sr * 10);
        karplusStrongRight.setSize(sr * 10);

        smoothKarplusVol.reset(sr, 0.2f);
        smoothKarplusVol.setCurrentAndTargetValue(0.0);

        // ====== RESONANT FEEDBACK SETUP =======
        resFeedbackLeft.setSamplerate(sr);
        resFeedbackLeft.resonatorSetup();
        resFeedbackLeft.setSize(sr * 30);

        resFeedbackRight.setSamplerate(sr);
        resFeedbackRight.resonatorSetup();
        resFeedbackRight.setSize(sr * 30);

        smoothFeedbackVol.reset(sr, 0.2f);
        smoothFeedbackVol.setCurrentAndTargetValue(0.0); 

        // ====== FORMANT COEFFICIENTS =======   
        for (int i = 0; i < formantAmount; i++)
        {
            float maxFreq = 5000; // Maximum random frequency for array of filters

            formantsLeft[i]->setCoefficients(IIRCoefficients::makeBandPass(sr, random.nextInt(maxFreq / 2) + maxFreq / 2 + 20, (random.nextFloat() - 0.01) + 0.01));
            formantsRight[i]->setCoefficients(IIRCoefficients::makeBandPass(sr, random.nextInt(maxFreq / 2) + maxFreq / 2 + 20, (random.nextFloat() - 0.01) + 0.01));
        }
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
            impulseParams.sustain = *sustain * 0.3;
            impulseParams.release = *release;
            impulseEnv.setParameters(impulseParams);

            // ====== DSP LOOP =======
            for (int sampleIndex = startSample; sampleIndex < (startSample + numSamples); sampleIndex++)
            {               
                // ====== ENVELOPES =======
                float envVal = env.getNextSample();
                float impulseVal = impulseEnv.getNextSample();
                float feedbackVal = feedbackEnv.getNextSample();
               
                // ====== IMPULSE =======
                float exciterLeft = random.nextFloat() * impulseVal;
                float exciterRight = random.nextFloat() * impulseVal;

                // ====== APPLY IMPULSE COLOUR =======


                // ====== DECIDE IMPULSE COLOUR =======
                if (*room < 1)
                {
                    exciterLeft = exciterLeft;
                    exciterRight = exciterRight;
                }
                else 
                {
                    float exColourLeft = 0.0f;
                    float exColourRight = 0.0f;

                    for (auto* formant : formantsLeft)
                    {
                        exColourLeft = formant->processSingleSampleRaw(exciterLeft);
                    }
                    for (auto* formant : formantsRight)
                    {
                        exColourRight = formant->processSingleSampleRaw(exciterRight);
                    }
                    
                    exciterLeft = exColourLeft / formantAmount * sqrt(formantAmount);
                    exciterRight = exColourRight / formantAmount * sqrt(formantAmount);
                };
                
                // ====== KARPLUS STRONG =======
                karplusStrongLeft.setPitch(freq, *instabilityAmount);
                karplusStrongRight.setPitch(freq + *detuneAmount, *instabilityAmount);

                karplusStrongLeft.setFeedback(*tailAmount);
                karplusStrongRight.setFeedback(*tailAmount);

                // ====== RESONANT FEEDBACK =======              
                float age = 1 - (*feedbackAgeAmount * random.nextFloat()); // Feedback Colour
                float delayt = (*delayTime * (sr / 20)) * age; // Delaytime

                resFeedbackLeft.setDelayTimeInSamples(delayt + 100);
                resFeedbackLeft.setResonator(freq, *qAmount);
                resFeedbackLeft.setFeedback(*feedbackAmount * feedbackVal);

                resFeedbackRight.setDelayTimeInSamples(delayt + 200);
                resFeedbackRight.setResonator(freq + *detuneAmount, *qAmount);
                resFeedbackRight.setFeedback(*feedbackAmount * feedbackVal);

                // ====== SMOOTH VOLUMES =======
                smoothFeedbackVol.setTargetValue(*feedbackAmount);
                float smoothedFeedbackVol = smoothFeedbackVol.getNextValue();

                smoothKarplusVol.setTargetValue(*karplusVolAmount);
                float smoothedKarplusVol = smoothKarplusVol.getNextValue();
                
                // ====== SAMPLE PROCESSING =======
                float currentSampleLeft = karplusStrongLeft.kSProcess(exciterLeft) * smoothedKarplusVol
                                    + (resFeedbackLeft.process(karplusStrongLeft.process(exciterRight)) * smoothedFeedbackVol)
                                    * gain;                  

                float currentSampleRight = karplusStrongRight.kSProcess(exciterRight) * smoothedKarplusVol
                                    + (resFeedbackRight.process(karplusStrongRight.process(exciterLeft)) * smoothedFeedbackVol)
                                    * gain;
                
                // ====== GLOBAL ENVELOPE =======
                currentSampleLeft = currentSampleLeft
                                    * envVal;
                currentSampleRight = currentSampleRight
                                    * envVal;

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
    OwnedArray<IIRFilter> formantsLeft, formantsRight;
    int formantAmount = 20;

    // ====== GLOBAL VOLUME =======   
    Limiter limiter;
    float gain = 0.3f;

    // ====== UTILITY =======   
    Random random; // for White Noise

    float freq; // Frequency of Synth
    float sr; // Samplerate
};

