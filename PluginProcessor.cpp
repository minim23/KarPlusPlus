/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
Assignment_3AudioProcessor::Assignment_3AudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", AudioChannelSet::stereo(), true)
#endif
    ),
#endif
    parameters (*this, nullptr, "ParamTreeID", {
    // Parameter Layout
    // id, description, min val, max val, default val
    std::make_unique<AudioParameterFloat>("detune", "Detune (Hz)", 0.0f, 20.0f, 2.0f),
    std::make_unique<AudioParameterFloat>("q", "Q", 1.0f, 2000.0f, 20.0f),
    std::make_unique<AudioParameterFloat>("attack", "Attack", 0.001f, 5.0f, 0.5f),
    std::make_unique<AudioParameterFloat>("decay", "Decay", 0.0f, 0.25f, 0.25f),
    std::make_unique<AudioParameterFloat>("sustain", "Sustain", 0.0f, 1.0f, 0.5f),
    std::make_unique<AudioParameterFloat>("release", "Release", 0.1f, 5.0f, 1.0f),
        })
{
    detuneParam = parameters.getRawParameterValue("detune");
    qParam = parameters.getRawParameterValue("q");
    attackParam = parameters.getRawParameterValue("attack");
    decayParam = parameters.getRawParameterValue("decay");
    sustainParam = parameters.getRawParameterValue("sustain");
    releaseParam = parameters.getRawParameterValue("release");

    // Constructor to set up polyphony
    for (int i = 0; i < voiceCount; i++)
    {
        synth.addVoice(new MySynthVoice()); //Synth Voice makes the sound
    }

    synth.addSound(new MySynthSound()); //Synth Sound allocates

    for (int i = 0; i < voiceCount; i++)
    {
        MySynthVoice* v = dynamic_cast<MySynthVoice*>(synth.getVoice(i)); //returns a pointer to synthesiser voice
        v->setParameterPointers(detuneParam, qParam, attackParam, decayParam, sustainParam, releaseParam);
    }
}

Assignment_3AudioProcessor::~Assignment_3AudioProcessor()
{
}

//==============================================================================
const String Assignment_3AudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool Assignment_3AudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool Assignment_3AudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool Assignment_3AudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double Assignment_3AudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int Assignment_3AudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int Assignment_3AudioProcessor::getCurrentProgram()
{
    return 0;
}

void Assignment_3AudioProcessor::setCurrentProgram(int index)
{
}

const String Assignment_3AudioProcessor::getProgramName(int index)
{
    return {};
}

void Assignment_3AudioProcessor::changeProgramName(int index, const String& newName)
{
}
 
// =============== PREPARE TO PLAY ====================
void Assignment_3AudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate(sampleRate);

    for (int i = 0; i < voiceCount; i++)
    {
        MySynthVoice* v = dynamic_cast<MySynthVoice*>(synth.getVoice(i)); //returns a pointer to synthesiser voice
        v->init(sampleRate);
    }
}

void Assignment_3AudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool Assignment_3AudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    ignoreUnused(layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
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
void Assignment_3AudioProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;

    // PROCESSING SYNTH CLASS
    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
}

//==============================================================================
bool Assignment_3AudioProcessor::hasEditor() const
{
    return false; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* Assignment_3AudioProcessor::createEditor()
{
    return new GenericAudioProcessorEditor(*this);
}

//==============================================================================
void Assignment_3AudioProcessor::getStateInformation(MemoryBlock& destData)
{
    // getStateInformation
    auto state = parameters.copyState();
    std::unique_ptr<XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void Assignment_3AudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // setStateInformation
    std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
    {
        if (xmlState->hasTagName(parameters.state.getType()))
        {
            parameters.replaceState(ValueTree::fromXml(*xmlState));
        }
    }
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Assignment_3AudioProcessor();

}