/*
  ==============================================================================

    FirstOrderIIRTransforms.h
    Created: 15 Jul 2022 2:26:55am
    Author:  natha

  ==============================================================================
*/

#pragma once

#ifndef FIRSTORDERIIRTRANSFORMS_H_INCLUDED
#define FIRSTORDERIIRTRANSFORMS_H_INCLUDED

#include <JuceHeader.h>
#include "Coefficient.h"

enum struct TransformationType
{
    directFormI = 0,
    directFormII = 1,
    directFormItransposed = 2,
    directFormIItransposed = 3
};

template <typename SampleType>
class FirstOrderIIRTransforms
{
    FirstOrderIIRTransforms();

    /** Sets the BiLinear Transform for the filter to use. See enum for available types. */
    void setTransformType(TransformationType newTransformType);

    //==========================================================================
    /** Initialises the processor. */
    void prepare(juce::dsp::ProcessSpec spec);

    /** Resets the internal state variables of the processor. */
    void reset(SampleType initialValue = { 0.0 });

    /** Ensure that the state variables are rounded to zero if the state
    variables are denormals. This is only needed if you are doing sample
    by sample processing.*/
    void snapToZero() noexcept;

    //==========================================================================
    /** Processes the input and output samples supplied in the processing context. */
    template <typename ProcessContext>
    void process(const ProcessContext& context) noexcept
    {
        const auto& inputBlock = context.getInputBlock();
        auto& outputBlock = context.getOutputBlock();
        const auto numChannels = outputBlock.getNumChannels();
        const auto numSamples = outputBlock.getNumSamples();

        jassert(inputBlock.getNumChannels() == numChannels);
        jassert(inputBlock.getNumSamples() == numSamples);

        if (context.isBypassed)
        {
            outputBlock.copyFrom(inputBlock);
            return;
        }

        for (size_t channel = 0; channel < numChannels; ++channel)
        {
            auto* inputSamples = inputBlock.getChannelPointer(channel);
            auto* outputSamples = outputBlock.getChannelPointer(channel);

            for (size_t i = 0; i < numSamples; ++i)
                outputSamples[i] = processSample((int)channel, inputSamples[i]);
        }

#if JUCE_DSP_ENABLE_SNAP_TO_ZERO
        snapToZero();
#endif
    }

private:
    //==========================================================================
    /** Processes one sample at a time on a given channel. */
    SampleType processSample(int channel, SampleType inputSample);

    //==========================================================================
    SampleType directFormI(int channel, SampleType& inputValue);
    SampleType directFormII(int channel, SampleType& inputValue);
    SampleType directFormITransposed(int channel, SampleType& inputValue);
    SampleType directFormIITransposed(int channel, SampleType& inputValue);

    //==========================================================================
    /** Coefficient gain */
    Coefficient<SampleType> a[2], b[2];

    std::vector<SampleType> Wn_, Xn_, Yn_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FirstOrderIIRTransforms)
};

#endif //FIRSTORDERIIRTRANSFORMS_H_INCLUDED
