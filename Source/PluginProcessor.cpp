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
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    ),
#endif
    parameters(*this, nullptr, "ParamTreeID", {
    // Parameter Layout
    // id, description, min val, max val, default val
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID {"karplusVol", 1}, "Karplus Vol", 0.0f, 1.0f, 0.7f),

        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID {"damp", 1}, "Dampening", 0.0f, 1.0f, 0.5f),
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID {"sustain", 1}, "Noise Sustain", 0.0f, 1.0f, 0.8f),
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID {"tail", 1}, "Tail", 0.0f, 0.99f, 0.9f),
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID {"instability", 1}, "Instability", 0.0f, 20.0f, 0.0f),

        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID {"feedback", 1}, "Feedback Vol", 0.0f, 1.0f, 0.0f),
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID {"delaytime", 1}, "Delaytime", 0.0f, 10.0f, 5.0f),
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID {"q", 1}, "Q", 0.1f, 100.0f, 50.0f),
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID {"feedbackAge", 1}, "Age", 0.0f, 1.0f, 0.0f),

        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID {"detune", 1}, "Detune", 0.0f, 20.0f, 0.0f),
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID {"offset", 1}, "Phase Offset", 0.0f, 1.0f, 0.0f),

        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID {"release", 1}, "Release", 0.0f, 10.0f, 5.0f),
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID {"volume", 1}, "Volume", 0.0f, 1.0f, 0.7f)
        })
{
    karplusVolParam = parameters.getRawParameterValue("karplusVol");

    dampParam = parameters.getRawParameterValue("damp");
    sustainParam = parameters.getRawParameterValue("sustain");
    tailParam = parameters.getRawParameterValue("tail");
    instabilityParam = parameters.getRawParameterValue("instability");

    feedbackParam = parameters.getRawParameterValue("feedback");
    delaytimeParam = parameters.getRawParameterValue("delaytime");
    qParam = parameters.getRawParameterValue("q");
    feedbackAgeParam = parameters.getRawParameterValue("feedbackAge");
    detuneParam = parameters.getRawParameterValue("detune");
    offsetParam = parameters.getRawParameterValue("offset");

    releaseParam = parameters.getRawParameterValue("release"); 
    volumeParam = parameters.getRawParameterValue("volume");

    // Constructor to set up polyphony
    for (int i = 0; i < voiceCount; i++)
    {
        synth.addVoice(new MySynthVoice()); //Synth Voice makes the sound
    }

    synth.addSound(new MySynthSound()); //Synth Sound allocates

    for (int i = 0; i < voiceCount; i++)
    {
        MySynthVoice* v = dynamic_cast<MySynthVoice*>(synth.getVoice(i)); //returns a pointer to synthesiser voice
        v->setParameterPointers(
            karplusVolParam,

            dampParam,
            sustainParam,
            tailParam,
            instabilityParam,

            feedbackParam,
            delaytimeParam,
            qParam,
            feedbackAgeParam,
            offsetParam,

            detuneParam,           
            releaseParam,
            volumeParam
            );
    }

    // ====== FORMANTS SETUP =======
    for (int i = 0; i < voiceCount; i++)
    {
    MySynthVoice* v = dynamic_cast<MySynthVoice*>(synth.getVoice(i));
    v->setFormants();
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

    for (int i = 0; i < voiceCount; i++)
    {
        MySynthVoice* v = dynamic_cast<MySynthVoice*>(synth.getVoice(i)); //returns a pointer to synthesiser voice
        v->init(sampleRate);
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

    // PROCESSING SYNTH CLASS
    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
}

//==============================================================================
bool KarPlusPlus2AudioProcessor::hasEditor() const
{
    return false; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* KarPlusPlus2AudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void KarPlusPlus2AudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // getStateInformation
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void KarPlusPlus2AudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // setStateInformation
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
    {
        if (xmlState->hasTagName(parameters.state.getType()))
        {
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
        }
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KarPlusPlus2AudioProcessor();

}
