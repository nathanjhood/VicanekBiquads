/*
  ==============================================================================

    MatchedBiquads.cpp
    Created: 14 Jul 2022 8:13:32pm
    Author:  natha

  ==============================================================================
*/

#include "MatchedBiquads.h"

template <typename SampleType>
MatchedBiquad<SampleType>::MatchedBiquad()
{
    reset();
}

template <typename SampleType>
void MatchedBiquad<SampleType>::setFrequency(SampleType newFreq)
{
    if (f != newFreq)
    {
        f = newFreq;
        coeffs();
    }
}

template <typename SampleType>
void MatchedBiquad<SampleType>::setGain(SampleType newGain)
{
    if (g != newGain)
    {
        g = newGain;
        coeffs();
    }
}

template <typename SampleType>
void MatchedBiquad<SampleType>::setResonance(SampleType newRes)
{

    if (q != newRes)
    {
        q = newRes;
        coeffs();
    };
}

template <typename SampleType>
void MatchedBiquad<SampleType>::setFilterType(FilterType newType)
{
    if (type != newType)
    {
        type = newType;
        reset();
        coeffs();
    }
}

template <typename SampleType>
void MatchedBiquad<SampleType>::setTransformType(TransformationType newTransformType)
{
    if (transformType != newTransformType)
    {
        transformType = newTransformType;
        reset();
        coeffs();
    }
}

template <typename SampleType>
void MatchedBiquad<SampleType>::prepare(juce::dsp::ProcessSpec spec)
{
    jassert(spec.sampleRate > 0);
    jassert(spec.numChannels > 0);

    sampleRate = spec.sampleRate;

    Wn_1.resize(spec.numChannels);
    Wn_2.resize(spec.numChannels);
    Xn_1.resize(spec.numChannels);
    Xn_2.resize(spec.numChannels);
    Yn_1.resize(spec.numChannels);
    Yn_2.resize(spec.numChannels);
}

template <typename SampleType>
void MatchedBiquad<SampleType>::reset(SampleType initValue)
{
    for (auto v : { &Wn_1, &Wn_2, &Xn_1, &Xn_2, &Yn_1, &Yn_2 })
        std::fill(v->begin(), v->end(), initValue);
}

template <typename SampleType>
SampleType MatchedBiquad<SampleType>::processSample(int channel, SampleType inputValue)
{
    jassert(juce::isPositiveAndBelow(channel, Wn_1.size()));
    jassert(juce::isPositiveAndBelow(channel, Wn_2.size()));
    jassert(juce::isPositiveAndBelow(channel, Xn_1.size()));
    jassert(juce::isPositiveAndBelow(channel, Xn_2.size()));
    jassert(juce::isPositiveAndBelow(channel, Yn_1.size()));
    jassert(juce::isPositiveAndBelow(channel, Yn_1.size()));


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
SampleType MatchedBiquad<SampleType>::directFormI(int channel, SampleType inputSample)
{
    auto& Xn1 = Xn_1[(size_t)channel];
    auto& Xn2 = Xn_2[(size_t)channel];
    auto& Yn1 = Yn_1[(size_t)channel];
    auto& Yn2 = Yn_2[(size_t)channel];

    auto& Xn = inputSample;
    auto& Yn = outputSample;

    Yn = ((Xn * b[0]) + (Xn1 * b[1]) + (Xn2 * b[2]) + (Yn1 * a[1]) + (Yn2 * a[2]));

    Xn2 = Xn1;
    Yn2 = Yn1;
    Xn1 = Xn;
    Yn1 = Yn;

    return Yn;
}

template <typename SampleType>
SampleType MatchedBiquad<SampleType>::directFormII(int channel, SampleType inputSample)
{
    auto& Wn1 = Wn_1[(size_t)channel];
    auto& Wn2 = Wn_2[(size_t)channel];

    auto& Wn = loop;
    auto& Xn = inputSample;
    auto& Yn = outputSample;

    Wn = (Xn + ((Wn1 * a[1]) + (Wn2 * a[2])));
    Yn = ((Wn * b[0]) + (Wn1 * b[1]) + (Wn2 * b[2]));

    Wn2 = Wn1;
    Wn1 = Wn;

    return Yn;
}

template <typename SampleType>
SampleType MatchedBiquad<SampleType>::directFormITransposed(int channel, SampleType inputSample)
{
    auto& Wn1 = Wn_1[(size_t)channel];
    auto& Wn2 = Wn_2[(size_t)channel];
    auto& Xn1 = Xn_1[(size_t)channel];
    auto& Xn2 = Xn_2[(size_t)channel];

    auto& Wn = loop;
    auto& Xn = inputSample;
    auto& Yn = outputSample;

    Wn = (Xn + Wn2);
    Yn = ((Wn * b[0]) + Xn2);

    Xn2 = ((Wn * b[1]) + Xn1);
    Wn2 = ((Wn * a[1]) + Wn1);
    Xn1 = (Wn * b[2]);
    Wn1 = (Wn * a[2]);

    return Yn;
}

template <typename SampleType>
SampleType MatchedBiquad<SampleType>::directFormIITransposed(int channel, SampleType inputSample)
{
    auto& Xn1 = Xn_1[(size_t)channel];
    auto& Xn2 = Xn_2[(size_t)channel];

    auto& Xn = inputSample;
    auto& Yn = outputSample;

    Yn = ((Xn * b[0]) + (Xn2));

    Xn2 = ((Xn * b[1]) + (Xn1)+(Yn * a[1]));
    Xn1 = ((Xn * b[2]) + (Yn * a[2]));

    return Yn;
}

template <typename SampleType>
void MatchedBiquad<SampleType>::coeffs()
{
    auto cosh = [&](SampleType z) { return (std::exp(z) + std::exp(-z)) * zeroFive; };
    auto powTwo = [&](SampleType x) { return x * x; };

    f0 = f / (static_cast<SampleType>(sampleRate) / two);
    AA = std::pow(ten, g / twenty);

    switch (type)
    {
    case FilterType::PeakEQ:

        alfa = std::sin(f0 * pi) / (two * std::sqrt(AA) * q);

        // Poles
        
        a_[0] = one;
        a_[1] = minusTwo * std::cos(f0 * pi) / (one + alfa);
        a_[2] = (one - alfa) / (one + alfa);

        // Zeros
        b_[0] = (one + AA * alfa) / (one + alfa);
        b_[1] = a_[1];
        b_[2] = (one - AA * alfa) / (one + alfa);

        break;

    case FilterType::HighPass:

        alfa = std::sin(f0 * pi) / (two * q);

        // Poles
        a_[0] = one;
        a_[1] = minusTwo * std::cos(f0 * pi) / (one + alfa);
        a_[2] = (one - alfa) / (one + alfa);

        // Zeros
        b_[0] = (one - a_[1] + a_[2]) / (two * two);
        b_[1] = minusTwo * b_[0];
        b_[2] = b_[0];

        break;

    case FilterType::LowPass:

        alfa = std::sin(f0 * pi) / (two * q);

        // Poles
        a_[0] = one;
        a_[1] = minusTwo * std::cos(f0 * pi) / (one + alfa);
        a_[2] = (one - alfa) / (one + alfa);

        // # Zeros
        b_[0] = (one + a_[1] + a_[2]) / (two * two);
        b_[1] = two * b_[0];
        b_[2] = b_[0];

        break;

    case FilterType::BandPass:

        alfa = std::sin(f0 * pi) / (two * q);

        // Poles
        a_[0] = one;
        a_[1] = minusTwo * std::cos(f0 * pi) / (one + alfa);
        a_[2] = (one - alfa) / (one + alfa);

        // Zeros
        b_[0] = (one - a_[2]) / two;
        b_[1] = zero;
        b_[2] = -b_[0];

        break;

    case FilterType::MPeakEQ:

        // Poles
        a_[0] = one;
        a_[2] = std::exp((- zeroFive) * pi * f0 / (std::sqrt(AA) * q));
        //_test = four * AA * q * q;

        (four * AA * q * q > one) ?   // complex conjugate poles
            (
                a_[1] = minusTwo * a_[2] * std::cos(std::sqrt(one - one / (four * AA * q * q)) * pi * f0);
        ) :                 // real poles
            (
                a_[1] = minusTwo * a_[2] * cosh(std::sqrt(one / (four * AA * q * q) - one) * pi * f0);
        );
        a_[2] = a_[2] * a_[2];

        // Zeros
        const auto AA0 = powTwo(one + a_[1] + a_[2]);
        const auto AA1 = powTwo(one - a_[1] + a_[2]);
        const auto AA2 = (-four) * a_[2];

        const auto phi1 = powTwo(std::sin(zeroFive * pi * f0));
        const auto phi0 = one - phi1;
        const auto phi2 = four * phi0 * phi1;

        const auto r1 = powTwo((phi0 * AA0 + phi1 * AA1 + phi2 * AA2) * AA);
        const auto r2 = powTwo((AA1 - AA0 + four * (phi0 - phi1) * AA2) * AA);

        const auto BB0 = AA0;
        const auto BB2 = (r1 - phi1 * r2 - BB0) / (powTwo(four * phi1));
        const auto BB1 = r2 + BB0 + four * (phi1 - phi0) * BB2;

        b_[1] = zeroFive * (one + a_[1] + a_[2] - std::sqrt(BB1));
        const auto w = one + a_[1] + a_[2] - b_[1];
        b_[0] = zeroFive * (w + std::sqrt(w ^ 2 + BB2));
        b_[2] = -BB2 / (four * b_[0]);

        break;
    }

    a[0] = (one / a_[0]);
    a[1] = ((-a_[1]) * a[0]);
    a[2] = ((-a_[2]) * a[0]);
    b[0] = (b_[0] * a[0]);
    b[1] = (b_[1] * a[0]);
    b[2] = (b_[2] * a[0]);
}

template <typename SampleType>
void MatchedBiquad<SampleType>::snapToZero() noexcept
{
    for (auto v : { &Wn_1, &Wn_2, &Xn_1, &Xn_2, &Yn_1, &Yn_2 })
        for (auto& element : *v)
            juce::dsp::util::snapToZero(element);
}

//==============================================================================
template class MatchedBiquad<float>;
template class MatchedBiquad<double>;
