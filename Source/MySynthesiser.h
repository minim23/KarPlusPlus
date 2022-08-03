#pragma once
#include "StringModel.h"
#include "Excitation.h"
#include "ADSR.h"
#include "Filterbank.h"

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
                              float dampExIn,
                              float FormantScaleIn,
                              
                              float dampStringIn,
                              float sustainIn,
                              float tailIn,
                              float instabilityIn,

                              float releaseIn,
                              float volumeIn
    )
    {
        dampExAmount = dampExIn;
        formantScaling = FormantScaleIn;
        
        dampStringAmount = dampStringIn;
        sustain = sustainIn;
        tailAmount = tailIn;
        instabilityAmount = instabilityIn;

        release = releaseIn;
        volume = volumeIn;
    }

    // ====== SAMPLERATE SETUP FOR PREPARE TO PLAY =======
    void prepareToPlay(int sampleRate)
    {
        sr = sampleRate;

        // ====== KARPLUS STRONG SETUP =======
        karplusStrong.setSamplerate(sr);
        karplusStrong.setSize(sr * 1); // Delay size of 1000ms
        
        // ====== EXCITATION SETUP =======
        excitation.setSamplerate(sr);
        
        // ====== FORMANTS SETUP =======
        formants.setSamplerate(sr);
        
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
        freq = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);

        // ====== EXCITATION =======
        excitation.setDampening (dampExAmount);
        
        // ====== STRING =======
        karplusStrong.setDampening (dampStringAmount);

        // ====== FORMANT COEFFICIENTS =======
        auto& formantScale = formantScaling; // De-reference pointer
        formants.setCoeff (formantScale); // Insert de-referenced value
        
        // ====== KARPLUS STRONG =======
        karplusStrong.setPitch (freq);
        karplusStrong.setFeedback (tailAmount); // Feedback between 0-1

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
    
//    /// New method to update ADSR values
//    void SynthVoice::update (const float attack, const float decay, const float sustain, const float release)
//    {
//      // Separate method to call ADSR in process block
//    }

     // ====== DSP BLOCK =======
    void renderNextBlock(juce::AudioSampleBuffer& outputBuffer, int startSample, int numSamples) override
    {
        if (playing) // check to see if this voice should be playing
        {
            jassert (isPrepared); // Did you prepare to play?
            
            if (! isVoiceActive()) // If Voice is silent, return without doing anything
                return;
            
            // ====== ADSR =======
            generalADSR.updateADSR (0.1, 0.25, 1.0f, release + 0.1f); // Make sure to end after feedback release
            impulseADSR.updateADSR (0.01f, 0.1f, sustain * 0.3, release + 0.01f);

            // ====== DSP LOOP =======
            for (int sampleIndex = startSample; sampleIndex < (startSample + numSamples); sampleIndex++)
            {
                // ====== SMOOTHED VOLUME =======
                globalVol.setTargetValue(volume); // Output Volume
                float smoothedGlobalVol = globalVol.getNextValue();

                // ====== ENVELOPE VALUES =======
                float envVal = generalADSR.getNextSample(); // Global envelope
                float impulseVal = impulseADSR.getNextSample(); // White Noise envelope
                
                // ====== EXCITATION =======
                float impulse = excitation.process() * impulseVal; // Enveloped White Noise

                // ====== FORMANTS =======
                // formants.process(impulse);
                
                // ====== SAMPLE PROCESSING CHAIN =======
                float currentSample = karplusStrong.process (impulse); // Karplus Strong Volume
                
                // ====== GLOBAL ENVELOPE AND VOLUME =======
                currentSample = currentSample
                                    * envVal // Global envelope
                                    * smoothedGlobalVol * smoothedGlobalVol * smoothedGlobalVol; // Exponential output control for volume adjustment
                                
                // ====== CHANNEL ASSIGNMENT =======
                for (int chan = 0; chan < outputBuffer.getNumChannels(); chan++)
                {
                    outputBuffer.addSample (chan, sampleIndex, currentSample);
                    
                    // DO SOME HAAS MAGIC HERE
                    
                    // if adsr is not active, clear current note
                    if (! generalADSR.isActive())
                        clearCurrentNote();
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
    
    // Needs to be public
    Formants formants;
    
private:
    // ====== NOTE ON/OFF =======   
    bool playing = false;
    bool ending = false;
    
    // ====== JASSERTS =======
    bool isPrepared { false };

    // ====== PARAMETER VALUES ======= 
    float dampExAmount;
    float formantScaling;
    float dampStringAmount;
    float room;
    float sustain;
    float tailAmount;
    float instabilityAmount;

    float release;
    float volume;

    // ====== ENVELOPES =======
    ADSRData generalADSR, impulseADSR;

    // ====== IMPULSE =======
    Excitation excitation;
    
    // ====== FILTERBANK =======
    juce::Random random; // for Filterbank
//    juce::OwnedArray<juce::IIRFilter> formants;
//    float exColour = 0.0f; // Setting up colouration - for some reason it sounded differently when exciter was assigned to formants directly
//    int formantAmount = 6;

    // ====== KARPLUS STRONG =======
    KarplusStrong karplusStrong;

    // ====== GLOBAL VOLUME =======   
    juce::SmoothedValue<float> globalVol;

    // ====== UTILITY =======   
    float freq; // Frequency of Synth
    float sr; // Samplerate
};

