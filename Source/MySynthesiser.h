#pragma once
#include "Data/StringModel.h"
#include "Data/ADSR.h"
#include "Data/Oscillators.h"
// #include "Data/OscData.h"

#include "Data/Excitation.h" // THIS CAN GO


class MySynthSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote      (int) override      { return true; }
    //--------------------------------------------------------------------------
    bool appliesToChannel   (int) override      { return true; }
};


class MySynthVoice : public juce::SynthesiserVoice
{
public:
    MySynthVoice() {}

    // ====== INITIALIZATE PARAMETER POINTERS =======
    void setParameterPointers(
                              float dampExParam,
                              float velToDampExParam,
                              float sustainParam,
                              float velToSustainParam,
                              
                              float formantWidthParam,
                              float velToFormantWParam,
                              float formantQParam,
                              float velToFormantQParam,
                          
                              float dampStringParam,
                              float velToDampStringParam,
                              float feedbackParam,
                              float velToFeedbackParam,
  
                              float volumeParam
    )
    {
        dampExcitation = dampExParam;
        velToDampExAmount = velToDampExParam;
        sustainExcitation = sustainParam;
        velToSustainExAmount = velToSustainParam;
        
        formantWidth = formantWidthParam;
        velToFormantWAmount = velToFormantWParam;
        formantQ = formantQParam;
        velToFormantQAmount = velToFormantQParam;
        
        dampenString = dampStringParam;
        velToDampenStAmount = velToDampStringParam;
        feedback = feedbackParam;
        velToFeedbackAmount = velToFeedbackParam;

        volume = volumeParam;
    }

    // ====== SAMPLERATE SETUP FOR PREPARE TO PLAY =======
    void prepareToPlay(int sampleRate, int samplesPerBlock, int outputChannels)
    {
        // SET SAMPLERATE
        excitation.setSamplerate (sampleRate);
        karplusStrong.setSamplerate (sampleRate);
        triOsc.setSampleRate (sampleRate);
        
        sr = sampleRate;
        
        karplusStrong.setSize (sampleRate * 1); // Delay size of 1000ms
        
        isPrepared = true;
    }
    
    // ====== PRODUCES PARAMETER VALUES RELATIVE TO INPUT VELOCITY =======
    float velToParam (float parameter, float velocity, float amount)
    {
        float velToParam = parameter;
        velToParam *= amount;
        velToParam *= velocity;
        velToParam += parameter;
        velToParam -= parameter * amount;
        
        return velToParam;
    }
    

    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int /*currentPitchWheelPosition*/) override
    {
        playing = true;
        ending = false;
        
        // ====== RElATIVE VELOCITY VALUES =======
        velToDampenExcitation = velToParam (dampExcitation, velocity, velToDampExAmount);
        velToSustainExcitation = velToParam (sustainExcitation, velocity, velToSustainExAmount);
        velToFormantWidth = velToParam (formantWidth, velocity, velToFormantWAmount);
        velToFormantQ = velToParam (formantQ, velocity, velToFormantQAmount);
        velToDampenString = velToParam (dampenString, velocity, velToDampenStAmount);
        velToFeedback = velToParam (feedback, velocity, velToFeedbackAmount);

        velToVol = velToParam (volume, velocity, 1.0f);

        // ====== SET NOTE PARAMETERS =======
        excitation.setDampening (velToDampenExcitation);
        karplusStrong.setDampening (velToDampenString);
        
        karplusStrong.setFeedback (velToFeedback);
        
        freq = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);
        karplusStrong.setPitch (freq);
        triOsc.setFrequency (freq);

        vol = velToVol;
        
        
        // ====== CALCULATE RELEASE TIME OF ADSR BASED ON FEEDBACK AND FREQUENCY =======
        float hzToMs = 1 / freq;
        relativeSustainTime = velToFeedback * hzToMs * 1000;
        
        // ====== TRIGGER ENVELOPES =======
        generalADSR.reset(); // clear out envelope before re-triggering it
        generalADSR.noteOn(); // start envelope

        impulseADSR.reset();
        impulseADSR.noteOn();

    }

    void stopNote(float /*velocity*/, bool allowTailOff) override
    {
        // ====== TRIGGER OFF ENVELOPES =======
        generalADSR.noteOff();
        impulseADSR.noteOff();

        ending = true;
    }

     // ====== DSP BLOCK =======
    void renderNextBlock(juce::AudioSampleBuffer &outputBuffer, int startSample, int numSamples) override
    {
        jassert (isPrepared);
        
        if (! isVoiceActive())
            return;

        // ====== ADSR =======
        generalADSR.updateADSR (0.1, relativeSustainTime, 1.0f, relativeSustainTime);
        impulseADSR.updateADSR (0.01f, 0.1f, velToSustainExcitation * 0.3, 0.02f);

        // ====== DSP LOOP =======
        for (int sampleIndex = startSample; sampleIndex < (startSample + numSamples); sampleIndex++)
        {
            // ====== ADSR =======
            float globalEnv = generalADSR.getNextSample(); // Global envelope
            float impulseEnv = impulseADSR.getNextSample(); // White Noise envelope
                            
            // ====== CHANNEL ASSIGNMENT ======= THIS COULD BE REFACTORED
            for (int chan = 0; chan < outputBuffer.getNumChannels(); chan++)
            {
                // ====== STEREO DSP =======
                float currentSample = triOsc.process();
                currentSample *= impulseEnv;
                currentSample = karplusStrong.process (currentSample);
                
                currentSample *= globalEnv; // ADSR
                currentSample *= vol; // Volume
                
                outputBuffer.addSample (chan, sampleIndex, currentSample);
                
                if (! generalADSR.isActive())
                    clearCurrentNote();
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
    
    // ====== JASSERTS =======
    bool isPrepared { false };

    // ====== PARAMETER VALUES ======= 
    float dampExcitation;
    float velToDampExAmount;
    float sustainExcitation;
    float velToSustainExAmount;
    
    float formantWidth;
    float velToFormantWAmount;
    float formantQ;
    float velToFormantQAmount;
    
    float dampenString;
    float velToDampenStAmount;
    float feedback;
    float velToFeedbackAmount;
    
    float volume;
    
    // ====== VELOCITY RELATIVE VALUES =======
    float velToDampenExcitation;
    float velToSustainExcitation;
    
    float velToFormantWidth;
    float velToFormantQ;
    
    float velToDampenString;
    float velToFeedback;
    
    float velToVol;
    
    // ====== ENVELOPES =======
    ADSRData generalADSR, impulseADSR;
    float relativeSustainTime;

    Excitation excitation;
    juce::Random random;
    
    KarplusStrong karplusStrong;

    TriOsc triOsc;
    

    float freq; // Frequency of Synth
    float sr; // Samplerate
    float vol;
    juce::SmoothedValue<float> globalVol;
};

