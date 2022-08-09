/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
KarPlusPlus2AudioProcessor::KarPlusPlus2AudioProcessor()
: foleys::MagicProcessor  (juce::AudioProcessor::BusesProperties()
                           .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    apvts (*this, nullptr, "ParamTreeID", createParams())
{
    FOLEYS_SET_SOURCE_PATH (__FILE__);
    
//    foleys::MagicProcessorState magicState { *this, apvts };
    
    // ====== HERE YOU CAN ADD THE VISUALISATION =======
    
    
    analyser = magicState.createAndAddObject<foleys::MagicAnalyser>("input");
    
    // ====== CONSTRUCTOR TO SET UP POLYPHONY =======
    for (int i = 0; i < voiceCount; i++)
    {
        synth.addVoice (new MySynthVoice()); //Synth Voice makes the sound
    }

    synth.addSound (new MySynthSound()); // Synth Sound allocates

    // ====== FORMANTS SETUP =======
    for (int i = 0; i < voiceCount; i++)
    {
    MySynthVoice* v = dynamic_cast<MySynthVoice*> (synth.getVoice(i));
    v->formants.setFormants();
    }
}

KarPlusPlus2AudioProcessor::~KarPlusPlus2AudioProcessor()
{
}

//==============================================================================
const juce::String KarPlusPlus2AudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool KarPlusPlus2AudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool KarPlusPlus2AudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool KarPlusPlus2AudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double KarPlusPlus2AudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int KarPlusPlus2AudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int KarPlusPlus2AudioProcessor::getCurrentProgram()
{
    return 0;
}

void KarPlusPlus2AudioProcessor::setCurrentProgram(int index)
{
}

const juce::String KarPlusPlus2AudioProcessor::getProgramName(int index)
{
    return {};
}

void KarPlusPlus2AudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}
 
// =============== PREPARE TO PLAY - SAMPLERATE SETUP ====================
void KarPlusPlus2AudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate(sampleRate);
    
    analyser->prepareToPlay (sampleRate, samplesPerBlock);

    for (int i = 0; i < voiceCount; i++)
    {
        MySynthVoice* v = dynamic_cast<MySynthVoice*>(synth.getVoice(i)); //returns a pointer to synthesiser voice
        v->prepareToPlay(sampleRate);
    }
}

void KarPlusPlus2AudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool KarPlusPlus2AudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    ignoreUnused(layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
#if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}
#endif

// =============== PROCESS BLOCK ====================
void KarPlusPlus2AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    
    magicState.processMidiBuffer (midiMessages, buffer.getNumSamples());
    
    // ====== UPDATE PARAMETERS =======
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        // ====== SYNTH VOICE CAST =======
        if (auto voice = dynamic_cast<MySynthVoice*>(synth.getVoice(i)))
        {
            // ====== PARAMETERS =======
            auto& dampExParam = *apvts.getRawParameterValue("DAMPEXCITATION");
            auto& velToDampExParam = *apvts.getRawParameterValue("VELTODAMPEX");
            auto& sustainParam = *apvts.getRawParameterValue("SUSTAIN");
            auto& velToSustainParam = *apvts.getRawParameterValue("VELTOSUSTAINEX");
            
            auto& formantWidthParam = *apvts.getRawParameterValue("FORMANTSCALING");
            auto& velToFormantWidthParam = *apvts.getRawParameterValue("VELTOFORMANTWIDTH");
            auto& formantQParam = *apvts.getRawParameterValue("FORMANTQ");
            auto& velToFormantQParam = *apvts.getRawParameterValue("VELTOFORMANTQ");
            
            auto& dampStringParam = *apvts.getRawParameterValue("DAMPSTRING");
            auto& velToDampStringParam = *apvts.getRawParameterValue("VELTODAMPENSTRING");

            auto& feedbackParam = *apvts.getRawParameterValue("FEEDBACK");
            auto& velToFeedbackParam = *apvts.getRawParameterValue("VELTOFEEDBACK");
            
            auto& volumeParam = *apvts.getRawParameterValue("VOLUME");
            
            // ====== CONVERT ATOMIC PARAMETERS TO FLOATS =======
            voice->setParameterPointers(
                                        dampExParam.load(),
                                        velToDampExParam.load(),
                                        sustainParam.load(),
                                        velToSustainParam.load(),
                                        
                                        formantWidthParam.load(),
                                        velToFormantWidthParam.load(),
                                        formantQParam.load(),
                                        velToFormantQParam.load(),
                                    
                                        dampStringParam.load(),
                                        velToDampStringParam.load(),
                                    
                                        feedbackParam.load(),
                                        velToFeedbackParam.load(),
            
                                        volumeParam.load()
                );
        }
    }

    // ====== DSP PROCESSING =======
    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
    analyser->pushSamples (buffer);
}

//==============================================================================
//bool KarPlusPlus2AudioProcessor::hasEditor() const
//{
//    return false; // (change this to false if you choose to not supply an editor)
//}
//

//juce::AudioProcessorEditor* KarPlusPlus2AudioProcessor::createEditor()
//{
////    return new juce::GenericAudioProcessorEditor(*this);
////  return new foleys::MagicPluginEditor (magicState);
////    return new foleys::MagicPluginEditor (magicState, BinaryData::magic1_xml, BinaryData::magic1_xmlSize); // THIS SHOULD POINT TO THE CORRECT SIZE
//}

//==============================================================================
void KarPlusPlus2AudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
//    auto state = apvts.copyState();
//    std::unique_ptr<juce::XmlElement> xml(state.createXml());
//    copyXmlToBinary(*xml, destData);
    
    magicState.getStateInformation (destData);
}

void KarPlusPlus2AudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
    {
        if (xmlState->hasTagName(apvts.state.getType()))
        {
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
        }
    }
    
//    magicState.setStateInformation (data, sizeInBytes, getActiveEditor());
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KarPlusPlus2AudioProcessor();

}

// Parameter Layout - DATA Side of the Plugin
juce::AudioProcessorValueTreeState::ParameterLayout KarPlusPlus2AudioProcessor::createParams()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    KarPlusPlus2AudioProcessor::addVelToParams (layout);
    
    // Vector List that returns object type ParameterLayout
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // push_back adds another element to the end of our vector
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID {"DAMPEXCITATION", 1}, "Dampen Excitation", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID {"SUSTAIN", 1}, "Sustain Excitation", 0.0f, 1.0f, 0.8f));


    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID {"FORMANTSCALING", 1}, "Formant Width", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID {"FORMANTQ", 1}, "Formant Q", 0.1f, 50.0f, 0.5f));


    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID {"DAMPSTRING", 1}, "Dampen String", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID {"FEEDBACK", 1}, "Feedback", 0.0f, 1.0f, 0.9f));


    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID {"VOLUME", 1}, "Volume", 0.0f, 1.0f, 0.7f));
//
//    // Vel To Params
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID {"VELTODAMPEX", 1}, "Vel -> Damp Ex", 0.0f, 1.0f, 0.1f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID {"VELTOSUSTAINEX", 1}, "Vel -> Sustain Ex", 0.0f, 1.0f, 0.1f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID {"VELTOFORMANTWIDTH", 1}, "Vel -> Formant W", 0.0f, 1.0f, 0.1f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID {"VELTOFORMANTQ", 1}, "Vel -> Formant Q", 0.0f, 1.0f, 0.1f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID {"VELTODAMPENSTRING", 1}, "Vel -> Dampen St", 0.0f, 1.0f, 0.1f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID {"VELTOFEEDBACK", 1}, "Vel -> Feedback", 0.0f, 1.0f, 0.1f));
//
//
     return { params.begin(), params.end() };
//    return layout;
}

void KarPlusPlus2AudioProcessor::addVelToParams (juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    
    auto velToDampEx = std::make_unique<juce::AudioParameterFloat> (juce::ParameterID {"VELTODAMPEX", 1}, "Vel -> Damp Ex", 0.0f, 1.0f, 0.1f);
    
    auto velToSustainEx = std::make_unique<juce::AudioParameterFloat> (juce::ParameterID {"VELTOSUSTAINEX", 1}, "Vel -> Sustain Ex", 0.0f, 1.0f, 0.1f);
    
    auto velToFormantWidth = std::make_unique<juce::AudioParameterFloat> (juce::ParameterID {"VELTOFORMANTWIDTH", 1}, "Vel -> Formant W", 0.0f, 1.0f, 0.1f);
    auto velToFormantQ = std::make_unique<juce::AudioParameterFloat> (juce::ParameterID {"VELTOFORMANTQ", 1}, "Vel -> Formant Q", 0.0f, 1.0f, 0.1f);
    
    auto velToDampenString = std::make_unique<juce::AudioParameterFloat> (juce::ParameterID {"VELTODAMPENSTRING", 1}, "Vel -> Dampen St", 0.0f, 1.0f, 0.1f);
    auto velToFeedback = std::make_unique<juce::AudioParameterFloat> (juce::ParameterID {"VELTOFEEDBACK", 1}, "Vel -> Feedback", 0.0f, 1.0f, 0.1f);
    
    auto group = std::make_unique<juce::AudioProcessorParameterGroup>("Vel -> Params", "VELTOTEST", "/",
                                                                      std::move (velToDampEx),
                                                                      std::move (velToSustainEx),
                                                                      std::move (velToFormantWidth),
                                                                      std::move (velToFormantQ),
                                                                      std::move (velToDampenString),
                                                                      std::move (velToFeedback)
                                                                      );
    
    layout.add (std::move (group));
    
}
