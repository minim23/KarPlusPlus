#pragma once
#include "StringModel.h"
#include "Excitation.h"
#include "ADSR.h"

// ===========================
// ===========================
// SOUND
class MySynthSound : public juce::SynthesiserSound
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
class MySynthVoice : public juce::SynthesiserVoice
{
public:
    MySynthVoice() {}

    // ====== INITIALIZATE PARAMETER POINTERS =======
    void setParameterPointers(

    std::atomic<float>* dampExIn,
                              
    std::atomic<float>* dampStringIn,
    std::atomic<float>* sustainIn,
    std::atomic<float>* tailIn,
    std::atomic<float>* instabilityIn,

    std::atomic<float>* releaseIn,
    std::atomic<float>* volumeIn
    )
    {
        dampExAmount = dampExIn;
        
        dampStringAmount = dampStringIn;
        sustain = sustainIn;
        tailAmount = tailIn;
        instabilityAmount = instabilityIn;

        release = releaseIn;
        volume = volumeIn;
    }

    // ====== SETUP FORMANTS =======
    void setFormants()
    {       
        for (int i = 0; i < formantAmount; i++)
        {
            formantsLeft.add(new juce::IIRFilter());
            formantsRight.add(new juce::IIRFilter());
        }
    }

    // ====== SAMPLERATE SETUP FOR PREPARE TO PLAY =======
    void prepareToPlay(int sampleRate)
    {
        sr = sampleRate;

        // ====== KARPLUS STRONG SETUP =======
        karplusStrongLeft.setSamplerate(sr);
        karplusStrongRight.setSamplerate(sr);
        karplusStrongLeft.setSize(sr * 1); // Delay size of 1000ms
        karplusStrongRight.setSize(sr * 1);
        
        // ====== EXCITATION SETUP =======
        excitation.setSamplerate(sr);
        
        globalVol.reset(sr, 0.02f); // Smoothed value of 20ms
        globalVol.setCurrentAndTargetValue(0.0);
        
        isPrepared = true;
    }
    
    //--------------------------------------------------------------------------
    /**
     What should be done when a note starts

     @param midiNoteNumber
     @param velocity
     @param SynthesiserSound unused variable
     @param / unused variable
     */
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int /*currentPitchWheelPosition*/) override
    {
        playing = true;
        ending = false;

        // ====== MIDI TO FREQ =======
        freq = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);

        // ====== EXCITATION =======
        excitation.setDampening(*dampExAmount);
        excitation.setDampening(*dampExAmount);
        
        // ====== STRING =======
        karplusStrongLeft.setAllpass(random.nextFloat(), random.nextFloat());
        karplusStrongRight.setAllpass(random.nextFloat(), random.nextFloat());
        
        karplusStrongLeft.setDampening(*dampStringAmount);
        karplusStrongRight.setDampening(*dampStringAmount);
        


        // ====== FORMANT COEFFICIENTS =======   
        for (int i = 0; i < formantAmount; i++)
        {
            float maxRandFreq = *dampExAmount // Get relative value
                                * 1000 // Max. Freq for ranomization of filters
                                + 1; // Next Int can not be 0

            float maxFreq = *dampExAmount // Get relative value
                            * 4000 // Rough max. Freq
                            + 100; // Absolute min. Freq

            formantsLeft[i]->setCoefficients(juce::IIRCoefficients::makeBandPass(sr, random.nextInt(maxRandFreq) + maxFreq, (random.nextFloat() - 0.01) + 0.01));
            formantsRight[i]->setCoefficients(juce::IIRCoefficients::makeBandPass(sr, random.nextInt(maxRandFreq) + maxFreq, (random.nextFloat() - 0.01) + 0.01));
        }

        // ====== TRIGGER ENVELOPES =======
        generalADSR.reset(); // clear out envelope before re-triggering it
        generalADSR.noteOn(); // start envelope

        impulseADSR.reset();
        impulseADSR.noteOn();

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
        generalADSR.noteOff();
        impulseADSR.noteOff();

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
    void renderNextBlock(juce::AudioSampleBuffer& outputBuffer, int startSample, int numSamples) override
    {
        if (playing) // check to see if this voice should be playing
        {
            jassert (isPrepared); // Did you prepare to play?
            
            if (! isVoiceActive()) // If Voice is silent, return without doing anything
                return;
            
            
            // ====== ADSR =======
            generalADSR.updateADSR (0.1, 0.25, 1.0f, *release + 0.1f); // Make sure to end after feedback release
            impulseADSR.updateADSR (0.01f, 0.1f, *sustain * 0.3, *release + 0.01f);

            // ====== DSP LOOP =======
            for (int sampleIndex = startSample; sampleIndex < (startSample + numSamples); sampleIndex++)
            {
                
                // ====== SMOOTHED VOLUME =======
                globalVol.setTargetValue(*volume); // Output Volume
                float smoothedGlobalVol = globalVol.getNextValue();

                // ====== ENVELOPE VALUES =======
                float envVal = generalADSR.getNextSample(); // Global envelope
                float impulseVal = impulseADSR.getNextSample(); // White Noise envelope
                
                // ====== EXCITATION =======
                float impulse = excitation.process();
               
                // ====== IMPULSE =======
                float exciterLeft = impulse * impulseVal; // Enveloped White Noise
                float exciterRight = impulse * impulseVal;

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
                karplusStrongRight.setPitch(freq, *instabilityAmount); // Detuning between left and right channel

                karplusStrongLeft.setFeedback(*tailAmount); // Feedback between 0-1
                karplusStrongRight.setFeedback(*tailAmount);
                
                // ====== SAMPLE PROCESSING CHAIN =======
                float currentSampleLeft = karplusStrongLeft.process(exciterLeft); // Karplus Strong Volume

                float currentSampleRight = karplusStrongRight.process(exciterRight);
                
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
    bool canPlaySound(juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<MySynthSound*> (sound) != nullptr;
    }
    //--------------------------------------------------------------------------
private:
    // ====== NOTE ON/OFF =======   
    bool playing = false;
    bool ending = false;

    // ====== PARAMETER VALUES ======= 
    std::atomic<float>* dampExAmount;
    std::atomic<float>* dampStringAmount;
    std::atomic<float>* room;
    std::atomic<float>* sustain;
    std::atomic<float>* tailAmount;
    std::atomic<float>* instabilityAmount;

    std::atomic<float>* release;
    std::atomic<float>* volume;

    // ====== ENVELOPES =======
    ADSRData generalADSR, impulseADSR;

    // ====== IMPULSE =======   
    juce::Random random; // for Filterbank
    juce::OwnedArray<juce::IIRFilter> formantsLeft, formantsRight;
    int formantAmount = 20;
    
    Excitation excitation;

    // ====== KARPLUS STRONG =======
    KarplusStrong karplusStrongLeft, karplusStrongRight;

    // ====== GLOBAL VOLUME =======   
    juce::SmoothedValue<float> globalVol;
    
    bool isPrepared { false }; 

    // ====== UTILITY =======   
    float freq; // Frequency of Synth
    float sr; // Samplerate
};

