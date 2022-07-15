/*
  ==============================================================================

    FirstOrderIIRTransforms.cpp
    Created: 15 Jul 2022 2:26:55am
    Author:  natha

  ==============================================================================
*/

#include "FirstOrderIIRTransforms.h"

template <typename SampleType>
FirstOrderIIRTransforms<SampleType>::FirstOrderIIRTransforms()
{

}

template <typename SampleType>
void FirstOrderIIRTransforms<SampleType>::setTransformType(TransformationType newTransformType)
{
    if (transformType != newTransformType)
    {
        transformType = newTransformType;
        reset();
    }
}

template <typename SampleType>
void FirstOrderIIRTransforms<SampleType>::prepare(juce::dsp::ProcessSpec spec)
{
    jassert(spec.sampleRate > 0);
    jassert(spec.numChannels > 0);

    sampleRate = spec.sampleRate;

    Wn_.resize(spec.numChannels);
    Xn_.resize(spec.numChannels);
    Yn_.resize(spec.numChannels);
}

template <typename SampleType>
void FirstOrderIIRTransforms<SampleType>::reset(SampleType initValue)
{
    for (auto v : { &Wn_, &Xn_, &Yn_ })
        std::fill(v->begin(), v->end(), initValue);
}

template <typename SampleType>
SampleType FirstOrderIIRTransforms<SampleType>::processSample(int channel, SampleType inputValue)
{
    jassert(juce::isPositiveAndBelow(channel, Wn_.size()));
    jassert(juce::isPositiveAndBelow(channel, Xn_.size()));
    jassert(juce::isPositiveAndBelow(channel, Yn_.size()));


    switch (transformType)
    {
    case TransformationType::directFormI:
        inputValue = directFormI(channel, inputValue);
        break;
    case TransformationType::directFormII:
        inputValue = directFormII(channel, inputValue);
        break;
    case TransformationType::directFormItransposed:
        inputValue = directFormITransposed(channel, inputValue);
        break;
    case TransformationType::directFormIItransposed:
        inputValue = directFormIITransposed(channel, inputValue);
        break;
    default:
        inputValue = directFormIITransposed(channel, inputValue);
    }

    return inputValue;
}

template <typename SampleType>
SampleType FirstOrderIIRTransforms<SampleType>::directFormI(int channel, SampleType& inputSample)
{
    auto& Xn1 = Xn_[(size_t)channel];
    auto& Yn1 = Yn_[(size_t)channel];

    auto& Xn = inputSample;
    auto& Yn = outputSample;

    Yn = ((Xn * b[0]) + (Xn1 * b[1]) + (Yn1 * a[1]));

    Xn1 = Xn;
    Yn1 = Yn;

    return Yn;
}

template <typename SampleType>
SampleType FirstOrderIIRTransforms<SampleType>::directFormII(int channel, SampleType& inputSample)
{
    auto& Wn1 = Wn_[(size_t)channel];

    auto& Wn = loop;
    auto& Xn = inputSample;
    auto& Yn = outputSample;

    Wn = (Xn + ((Wn1 * a[1])));
    Yn = ((Wn * b[0]) + (Wn1 * b[1]));

    Wn1 = Wn;

    return Yn;
}

template <typename SampleType>
SampleType FirstOrderIIRTransforms<SampleType>::directFormITransposed(int channel, SampleType& inputSample)
{
    auto& Wn1 = Wn_[(size_t)channel];
    auto& Xn1 = Xn_[(size_t)channel];

    auto& Wn = loop;
    auto& Xn = inputSample;
    auto& Yn = outputSample;

    Wn = (Xn + Wn1);
    Yn = ((Wn * b[0]) + Xn1);

    Xn1 = (Wn * b[2]);
    Wn1 = (Wn * a[2]);

    return Yn;
}

template <typename SampleType>
SampleType FirstOrderIIRTransforms<SampleType>::directFormIITransposed(int channel, SampleType& inputSample)
{
    auto& Xn1 = Xn_[(size_t)channel];

    auto& Xn = inputSample;
    auto& Yn = outputSample;

    Yn = ((Xn * b[0]) + (Xn1));

    Xn1 = ((Xn * b[1]) + (Yn * a[1]));

    return Yn;
}

template <typename SampleType>
void FirstOrderIIRTransforms<SampleType>::snapToZero() noexcept
{
    for (auto v : { &Wn_, &Xn_, &Yn_ })
        for (auto& element : *v)
            juce::dsp::util::snapToZero(element);
}

//==============================================================================
template class FirstOrderIIRTransforms<float>;
template class FirstOrderIIRTransforms<double>;
