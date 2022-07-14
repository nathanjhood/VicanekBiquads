/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
VicanekBiquadAudioProcessor::VicanekBiquadAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
#endif
    apvts(*this, &undoManager, "Parameters", createParameterLayout()),
    spec(),
    rmsLeft(), rmsRight(),
    parameters(*this),
    processorFloat(*this),
    processorDouble(*this),
    bypassState(static_cast<juce::AudioParameterBool*>(apvts.getParameter("bypassID"))),
    processingPrecision(ProcessingPrecision::singlePrecision)
{
    jassert(bypassState != nullptr);
}

VicanekBiquadAudioProcessor::~VicanekBiquadAudioProcessor()
{
}

//==============================================================================
juce::AudioProcessorParameter* VicanekBiquadAudioProcessor::getBypassParameter() const
{
    return bypassState;
}

bool VicanekBiquadAudioProcessor::isBypassed() const noexcept
{
    return bypassState->get() == true;
}

void VicanekBiquadAudioProcessor::setBypassParameter(juce::AudioParameterBool* newBypass) noexcept
{
    if (bypassState != newBypass)
    {
        bypassState = newBypass;
        releaseResources();
        reset();
    }
}

bool VicanekBiquadAudioProcessor::supportsDoublePrecisionProcessing() const
{
    return false;
}

juce::AudioProcessor::ProcessingPrecision VicanekBiquadAudioProcessor::getProcessingPrecision() const noexcept
{
    return processingPrecision;
}

bool VicanekBiquadAudioProcessor::isUsingDoublePrecision() const noexcept
{
    return processingPrecision == doublePrecision;
}

void VicanekBiquadAudioProcessor::setProcessingPrecision(ProcessingPrecision newPrecision) noexcept
{
    // If you hit this assertion then you're trying to use double precision
    // processing on a processor which does not support it!
    jassert(newPrecision != doublePrecision || supportsDoublePrecisionProcessing());

    if (processingPrecision != newPrecision)
    {
        processingPrecision = newPrecision;
        releaseResources();
        processorFloat.reset();
        processorDouble.reset();
    }
}

//==============================================================================
const juce::String VicanekBiquadAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool VicanekBiquadAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool VicanekBiquadAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool VicanekBiquadAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double VicanekBiquadAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int VicanekBiquadAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int VicanekBiquadAudioProcessor::getCurrentProgram()
{
    return 0;
}

void VicanekBiquadAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String VicanekBiquadAudioProcessor::getProgramName (int index)
{
    return {};
}

void VicanekBiquadAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void VicanekBiquadAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(sampleRate, samplesPerBlock);

    processingPrecision = getProcessingPrecision();

    spec.sampleRate = getSampleRate();
    spec.maximumBlockSize = getBlockSize();
    spec.numChannels = getTotalNumInputChannels();

    rmsLeft.reset(sampleRate, rampDurationSeconds);
    rmsRight.reset(sampleRate, rampDurationSeconds);

    rmsLeft.setCurrentAndTargetValue(-100.0f);
    rmsRight.setCurrentAndTargetValue(-100.0f);

    processorFloat.prepare(getSpec());
    processorDouble.prepare(getSpec());
}

void VicanekBiquadAudioProcessor::releaseResources()
{
    processorFloat.reset();
    processorDouble.reset();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool VicanekBiquadAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
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

void VicanekBiquadAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    if (bypassState->get())
    {
        processBlockBypassed(buffer, midiMessages);
    }

    else
    {
        juce::ScopedNoDenormals noDenormals;

        processorFloat.process(buffer, midiMessages);

        rmsLeft.skip(buffer.getNumSamples());
        rmsRight.skip(buffer.getNumSamples());

        {
            const auto value = juce::Decibels::gainToDecibels(buffer.getRMSLevel(0, 0, buffer.getNumSamples()));
            if (value < rmsLeft.getCurrentValue())
                rmsLeft.setTargetValue(value);
            else
                rmsLeft.setCurrentAndTargetValue(value);
        }

        {
            const auto value = juce::Decibels::gainToDecibels(buffer.getRMSLevel(1, 0, buffer.getNumSamples()));
            if (value < rmsRight.getCurrentValue())
                rmsRight.setTargetValue(value);
            else
                rmsRight.setCurrentAndTargetValue(value);
        }
    }
}

void VicanekBiquadAudioProcessor::processBlock(juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midiMessages)
{
    if (bypassState->get())
    {
        processBlockBypassed(buffer, midiMessages);
    }

    else
    {
        juce::ScopedNoDenormals noDenormals;

        processorDouble.process(buffer, midiMessages);

        rmsLeft.skip(buffer.getNumSamples());
        rmsRight.skip(buffer.getNumSamples());

        {
            const auto value = static_cast<float>(juce::Decibels::gainToDecibels(buffer.getRMSLevel(0, 0, buffer.getNumSamples())));
            if (value < rmsLeft.getCurrentValue())
                rmsLeft.setTargetValue(value);
            else
                rmsLeft.setCurrentAndTargetValue(value);
        }

        {
            const auto value = static_cast<float>(juce::Decibels::gainToDecibels(buffer.getRMSLevel(1, 0, buffer.getNumSamples())));
            if (value < rmsRight.getCurrentValue())
                rmsRight.setTargetValue(value);
            else
                rmsRight.setCurrentAndTargetValue(value);
        }
    }
}

void VicanekBiquadAudioProcessor::processBlockBypassed(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    midiMessages.clear();

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing context(block);

    const auto& inputBlock = context.getInputBlock();
    auto& outputBlock = context.getOutputBlock();

    outputBlock.copyFrom(inputBlock);
}

void VicanekBiquadAudioProcessor::processBlockBypassed(juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midiMessages)
{
    midiMessages.clear();

    juce::dsp::AudioBlock<double> block(buffer);
    juce::dsp::ProcessContextReplacing context(block);

    const auto& inputBlock = context.getInputBlock();
    auto& outputBlock = context.getOutputBlock();

    outputBlock.copyFrom(inputBlock);
}

//==============================================================================
bool VicanekBiquadAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* VicanekBiquadAudioProcessor::createEditor()
{
    return new VicanekBiquadAudioProcessorEditor (*this);
}

juce::AudioProcessorValueTreeState::ParameterLayout VicanekBiquadAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout params;

    params.add(std::make_unique<juce::AudioParameterBool>("bypassID", "Bypass", false));

    Parameters::setParameterLayout(params);

    return params;
}

//==============================================================================
void VicanekBiquadAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void VicanekBiquadAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

float VicanekBiquadAudioProcessor::getRMSLevel(const int channel) const
{
    jassert(channel == 0 || channel == 1);

    if (channel == 0)
        return rmsLeft.getCurrentValue();
    if (channel == 1)
        return rmsRight.getCurrentValue();
    return 0.0f;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VicanekBiquadAudioProcessor();
}
